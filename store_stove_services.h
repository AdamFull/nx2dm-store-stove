#pragma once

#include "store_stove/store_stove_platform.h"

#include "store/store_service.h"

#include "core/foundation/containers/hash_map.h"

#include <GameSupportSDK.h>
#include <IAPSDK.h>
#include <OwnershipSDK.h>

namespace nxm::store_stove {

/// Three of the five neutral services (store_service.h), backed by the real
/// STOVE PC SDK - store.cloud_saves and store.presence are never registered
/// (confirmed absent: no file-storage API beyond a bare
/// Base_GetCloudSavingPath() path string with no read/write/list calls
/// attached, and zero friends/presence hits anywhere in the SDK), the same
/// "simply doesn't provide it" shape used elsewhere in this module family
/// for a backend whose SDK genuinely lacks a subsystem.
///
/// store.achievements *is* registered here, unlike originally assumed -
/// GameSupportSDK turns out to have a real stats+achievements+leaderboard
/// subsystem. But its achievements are entirely server-computed from a Stat
/// crossing a goal value the SDK's own StovePCAchievementCondition never
/// names (no statId field), so there is no honest way to implement a
/// generic unlock(id) here - see StoveAchievements::unlock() below.
///
/// Every call in this SDK is asynchronous via a callback - unlike Steam or
/// GOG Galaxy, there is no synchronous local-cache read anywhere - and
/// every one of those callbacks is a **plain C function pointer with no
/// userdata parameter** (see store_stove_platform.h's class comment for
/// why this forces a static "current instance" pointer per class below).
/// Every class here checks the relevant StovePlatform::*_ready() gate
/// before touching its SDK and degrades to a safe default when it's false -
/// true whenever the process wasn't launched through the STOVE launcher,
/// the same guard-path convention store_steam's tests already established.

class StoveCore final : public store::StoreCore {
public:
  explicit StoveCore(StovePlatform &platform) noexcept;
  ~StoveCore() override;

  [[nodiscard]] bool is_owned(nx::string_view dlc_id = {}) const override;
  [[nodiscard]] nx::vector<nx::string> owned_dlc_ids() const override {
    return m_owned_dlc_ids;
  }
  [[nodiscard]] nx::string_view store_name() const noexcept override {
    return "stove";
  }

  /// Fires Ownership_OwnershipList() - the SDK has no per-id query, only a
  /// full list of the base game + every DLC's ownership at once, refreshing
  /// both is_owned()'s base-game flag and owned_dlc_ids().
  void refresh_ownership();

private:
  static void __cdecl on_ownership_list_finished(
      Stove::PCSDK::CallbackResult result,
      Stove::PCSDK::Ownership::StovePCOwnership *ownerships, uint32_t count);

  StovePlatform &m_platform;
  bool m_base_owned = false;
  nx::vector<nx::string> m_owned_dlc_ids;

  static StoveCore *s_instance;
};

class StoveIap final : public store::StoreIap {
public:
  explicit StoveIap(StovePlatform &platform) noexcept;
  ~StoveIap() override;

  [[nodiscard]] nx::vector<store::StoreProduct> products() const override {
    return m_products;
  }
  bool purchase(nx::string_view product_id) override;
  [[nodiscard]] bool purchase_pending() const override { return m_purchase_pending; }
  [[nodiscard]] nx::string_view purchase_error() const override {
    return m_purchase_error.view();
  }

  /// Fires IAP_FetchProducts() (default category/page), refreshing
  /// products() and the per-id sale-price cache purchase() needs -
  /// IAP_StartPurchase()'s order must echo back the exact sale price a
  /// prior fetch returned, so purchase() can't be called for an id this
  /// hasn't cached yet.
  void refresh_products();

private:
  static void __cdecl on_fetch_products_finished(
      Stove::PCSDK::CallbackResult result,
      Stove::PCSDK::IAP::StovePCProduct *products, uint32_t count);
  static void __cdecl on_start_purchase_finished(
      Stove::PCSDK::CallbackResult result,
      Stove::PCSDK::IAP::StovePCPurchaseResult purchase_result);

  StovePlatform &m_platform;
  nx::vector<store::StoreProduct> m_products;
  nx::hash_map<nx::string, f64> m_sale_price_by_id;
  bool m_purchase_pending = false;
  nx::string m_purchase_error;

  static StoveIap *s_instance;
};

class StoveAchievements final : public store::StoreAchievements {
public:
  explicit StoveAchievements(StovePlatform &platform) noexcept;
  ~StoveAchievements() override;

  /// STOVE has no direct "unlock" primitive - achievements are computed
  /// server-side from a Stat crossing a goal value configured on STOVE's
  /// own backend, and StovePCAchievementCondition (the SDK's own
  /// description of that condition) never names which stat id drives it,
  /// so there is no honest way to implement a generic unlock(id) here.
  /// Always refuses; drive the backing stat via set_stat() instead (per
  /// whatever condition was configured for this achievement in the STOVE
  /// partner console) and poll is_unlocked().
  bool unlock(nx::string_view id) override;
  [[nodiscard]] bool is_unlocked(nx::string_view id) const override;
  [[nodiscard]] nx::vector<nx::string> achievement_ids() const override {
    return m_achievement_ids;
  }
  bool set_stat(nx::string_view id, f64 value) override;
  [[nodiscard]] f64 stat(nx::string_view id) const override;

  /// Fires GameSupport_AllAchievement(), refreshing achievement_ids() and
  /// the per-id unlocked cache is_unlocked() reads.
  void refresh_achievements();
  /// Fires GameSupport_Stat(id), refreshing the cached value stat() reads
  /// back for that one id.
  void refresh_stat(nx::string_view id);

private:
  static void __cdecl on_all_achievement_finished(
      Stove::PCSDK::CallbackResult result,
      Stove::PCSDK::GameSupport::StovePCAchievement *achievements,
      uint32_t count);
  static void __cdecl on_modify_stat_finished(
      Stove::PCSDK::CallbackResult result,
      Stove::PCSDK::GameSupport::StovePCModifyStatValue value);
  static void __cdecl
  on_stat_finished(Stove::PCSDK::CallbackResult result,
                   Stove::PCSDK::GameSupport::StovePCStat stat);

  StovePlatform &m_platform;
  nx::vector<nx::string> m_achievement_ids;
  nx::hash_map<nx::string, bool> m_unlocked;
  nx::hash_map<nx::string, f64> m_stats;
  // set_stat()'s single in-flight target id - StovePCModifyStatValue's
  // response carries no id of its own to key m_stats on, unlike
  // StovePCStat's GetStatFullId() (see on_stat_finished), so the write path
  // has to track it the same shared-slot way store_steam's own
  // purchase_pending()/purchase_error() track one in-flight IAP call.
  nx::string m_pending_stat_id;

  static StoveAchievements *s_instance;
};

}
