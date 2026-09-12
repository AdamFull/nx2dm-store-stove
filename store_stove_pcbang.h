#pragma once

#include "store_stove/store_stove_platform.h"

#include "core/foundation/core/foundation.h"

#include <PCBangSDK.h>

namespace nxm::store_stove {

/// STOVE's PC Bang detection (`Stove::PCSDK::PCBang`) - a Stove-only extra,
/// a real Korean-market storefront concept with no equivalent elsewhere in
/// this module family: Korean internet-cafe ("PC bang") venues get special
/// in-game benefits when the SDK detects the machine is one.
///
/// `PCBang_UserLogin` registers two callbacks at once - one fires once with
/// the login result, the other fires repeatedly on the SDK's own 4-minute
/// timer with refreshed benefits - there is no separate "refresh" call from
/// the game side, it's push-based. `PCBang_CheckPCBangStatus` is an
/// independent query, not requiring a prior login. All three calls share
/// one cache below, each callback only touching the fields it actually
/// received.
///
/// Same plain-C-function-pointer-with-no-userdata callback shape every
/// other STOVE call already has - follows the identical static
/// current-instance dispatch `StoveCore`/`StoveIap`/`StoveAchievements`
/// already use (store_stove_services.h), guarding on
/// `StovePlatform::pcbang_ready()`.
class StovePCBang {
public:
  explicit StovePCBang(StovePlatform &platform) noexcept;
  ~StovePCBang();

  /// Fires PCBang_UserLogin(), registering both the one-shot login result
  /// and the recurring benefits-refresh callback.
  bool login();
  [[nodiscard]] bool login_pending() const noexcept { return m_login_pending; }
  [[nodiscard]] bool logged_in() const noexcept { return m_logged_in; }

  /// Fires PCBang_UserLogout(); clears the cache below on success.
  bool logout();
  [[nodiscard]] bool logout_pending() const noexcept { return m_logout_pending; }

  /// Fires PCBang_CheckPCBangStatus() - independent of login().
  bool check_status();
  [[nodiscard]] bool check_status_pending() const noexcept {
    return m_check_status_pending;
  }

  // Shared cache, populated by whichever call above most recently landed.
  [[nodiscard]] bool is_pc_bang() const noexcept;
  [[nodiscard]] bool is_premium() const noexcept;
  [[nodiscard]] i32 serial_number() const noexcept { return m_serial_number; }
  [[nodiscard]] i32 remain_time() const noexcept { return m_remain_time; }
  [[nodiscard]] i32 product_code() const noexcept { return m_product_code; }

private:
  static void __cdecl login_callback(
      Stove::PCSDK::CallbackResult result,
      Stove::PCSDK::PCBang::StovePCBangUserLogin userLogin);
  static void __cdecl refresh_benefits_callback(
      Stove::PCSDK::CallbackResult result,
      Stove::PCSDK::PCBang::StovePCRefreshUserBenefits benefits);
  static void __cdecl logout_callback(Stove::PCSDK::CallbackResult result);
  static void __cdecl check_status_callback(
      Stove::PCSDK::CallbackResult result,
      Stove::PCSDK::PCBang::StovePCBangStatus status);

  StovePlatform &m_platform;
  bool m_login_pending = false;
  bool m_logged_in = false;
  bool m_logout_pending = false;
  bool m_check_status_pending = false;
  Stove::PCSDK::PCBang::PCBangPremium m_premium_status =
      Stove::PCSDK::PCBang::PCBangPremium::PCBANG_ERROR;
  i32 m_serial_number = 0;
  i32 m_remain_time = 0;
  i32 m_product_code = 0;

  static StovePCBang *s_instance;
};

}
