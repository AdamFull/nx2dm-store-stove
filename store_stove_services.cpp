#include "store_stove/store_stove_services.h"

#include "core/foundation/diagnostics/log.h"
#include "core/foundation/strings/format.h"

#include <cwchar>
#include <utility>

namespace nxm::store_stove {
namespace {

const nx::log::Category log_store_stove = nx::log::category("store_stove");

[[nodiscard]] bool parse_i64(const nx::string_view text, int64_t &out) {
  if (text.empty())
    return false;
  i64 value = 0;
  for (const char c : text) {
    if (c < '0' || c > '9')
      return false;
    value = value * 10 + (c - '0');
  }
  out = value;
  return true;
}

} // namespace

// -- StoveCore --------------------------------------------------------------

StoveCore *StoveCore::s_instance = nullptr;

StoveCore::StoveCore(StovePlatform &platform) noexcept : m_platform(platform) {
  s_instance = this;
}

StoveCore::~StoveCore() {
  if (s_instance == this)
    s_instance = nullptr;
}

bool StoveCore::is_owned(const nx::string_view dlc_id) const {
  if (!m_platform.ownership_ready())
    return false;
  if (dlc_id.empty())
    return m_base_owned;
  for (const nx::string &id : m_owned_dlc_ids)
    if (id.view() == dlc_id)
      return true;
  return false;
}

void StoveCore::refresh_ownership(const nx::string_view) {
  if (!m_platform.ownership_ready())
    return;
  Stove::PCSDK::Ownership::Ownership_OwnershipList(
      &StoveCore::on_ownership_list_finished);
}

void __cdecl StoveCore::on_ownership_list_finished(
    const Stove::PCSDK::CallbackResult result,
    Stove::PCSDK::Ownership::StovePCOwnership *const ownerships,
    const uint32_t count) {
  if (s_instance == nullptr || !result.GetResult().IsSuccessful())
    return;
  using Stove::PCSDK::Ownership::OwnershipCode;
  using Stove::PCSDK::Ownership::OwnershipGameCode;

  s_instance->m_base_owned = false;
  s_instance->m_owned_dlc_ids.clear();
  for (uint32_t i = 0; i < count; ++i) {
    const bool owned = ownerships[i].GetOwnershipCode() == OwnershipCode::ACQUIRE;
    if (ownerships[i].GetGameCode() == OwnershipGameCode::DLC) {
      if (owned)
        s_instance->m_owned_dlc_ids.push_back(
            utf8_from_wide(ownerships[i].GetGameId()));
    } else if (ownerships[i].GetGameCode() == OwnershipGameCode::BASIC) {
      s_instance->m_base_owned = s_instance->m_base_owned || owned;
    }
  }
}

// -- StoveIap -----------------------------------------------------------

StoveIap *StoveIap::s_instance = nullptr;

StoveIap::StoveIap(StovePlatform &platform) noexcept : m_platform(platform) {
  s_instance = this;
}

StoveIap::~StoveIap() {
  if (s_instance == this)
    s_instance = nullptr;
}

bool StoveIap::purchase(const nx::string_view product_id) {
  if (!m_platform.iap_ready())
    return false;
  int64_t numeric_id = 0;
  if (!parse_i64(product_id, numeric_id))
    return false;
  const auto it = m_sale_price_by_id.find(nx::string(product_id));
  if (it == m_sale_price_by_id.end())
    // Unknown until refresh_products() has cached this id's sale price -
    // IAP_StartPurchase()'s order must echo back the exact price a prior
    // fetch returned.
    return false;

  namespace IAP = Stove::PCSDK::IAP;

  IAP::StovePCOrderProduct order;
  order.SetProductId(numeric_id);
  order.SetSalePrice(it->second);
  order.SetQuantity(1);

  IAP::StovePCPurchaseOption option;
  option.SetOperation(IAP::StovePCPurchaseOperation::WITH_WEBVIEW_AND_CONFIRM_RESULT);

  IAP::StovePCStartPurchaseParam params;
  params.CreateOrderProduct(1);
  params.SetOrderProduct(0, &order);
  params.SetPurchaseOption(option);

  m_purchase_pending = true;
  m_purchase_error = nx::string{};
  IAP::IAP_StartPurchase(&params, &StoveIap::on_start_purchase_finished);
  return true;
}

void StoveIap::refresh_products(const nx::vector<nx::string> &) {
  if (!m_platform.iap_ready())
    return;
  const Stove::PCSDK::IAP::StovePCFetchProductParam params;
  Stove::PCSDK::IAP::IAP_FetchProducts(&params,
                                       &StoveIap::on_fetch_products_finished);
}

void __cdecl StoveIap::on_fetch_products_finished(
    const Stove::PCSDK::CallbackResult result,
    Stove::PCSDK::IAP::StovePCProduct *const products, const uint32_t count) {
  if (s_instance == nullptr || !result.GetResult().IsSuccessful())
    return;
  s_instance->m_products.clear();
  s_instance->m_sale_price_by_id.clear();
  for (uint32_t i = 0; i < count; ++i) {
    store::StoreProduct product;
    product.id = nx::format("{}", products[i].GetProductId());
    product.title = utf8_from_wide(products[i].GetName());
    product.price_display = utf8_from_wide(products[i].GetDisplaySalePriceString());
    s_instance->m_sale_price_by_id.insert_or_assign(product.id,
                                                     products[i].GetSalePrice());
    s_instance->m_products.push_back(std::move(product));
  }
}

void __cdecl StoveIap::on_start_purchase_finished(
    const Stove::PCSDK::CallbackResult result,
    const Stove::PCSDK::IAP::StovePCPurchaseResult purchase_result) {
  if (s_instance == nullptr)
    return;
  s_instance->m_purchase_pending = false;
  if (!result.GetResult().IsSuccessful()) {
    s_instance->m_purchase_error =
        nx::format("Stove error {}", result.GetResult().GetResultCode());
    return;
  }
  if (!purchase_result.IsPurchased())
    s_instance->m_purchase_error = nx::string("purchase did not complete");
}

// -- StoveAchievements --------------------------------------------------

StoveAchievements *StoveAchievements::s_instance = nullptr;

StoveAchievements::StoveAchievements(StovePlatform &platform) noexcept
    : m_platform(platform) {
  s_instance = this;
}

StoveAchievements::~StoveAchievements() {
  if (s_instance == this)
    s_instance = nullptr;
}

bool StoveAchievements::unlock(const nx::string_view) { return false; }

bool StoveAchievements::is_unlocked(const nx::string_view id) const {
  const auto it = m_unlocked.find(nx::string(id));
  return it != m_unlocked.end() && it->second;
}

bool StoveAchievements::set_stat(const nx::string_view id, const f64 value) {
  if (!m_platform.game_support_ready())
    return false;
  const nx::vector<wchar_t> wide_id = wide_from_utf8(id);
  m_pending_stat_id = nx::string(id);
  Stove::PCSDK::GameSupport::GameSupport_ModifyStat(
      wide_id.data(), static_cast<int32_t>(value),
      &StoveAchievements::on_modify_stat_finished);
  return true;
}

f64 StoveAchievements::stat(const nx::string_view id) const {
  const auto it = m_stats.find(nx::string(id));
  return it == m_stats.end() ? 0.0 : it->second;
}

void StoveAchievements::refresh_achievements() {
  if (!m_platform.game_support_ready())
    return;
  Stove::PCSDK::GameSupport::GameSupport_AllAchievement(
      &StoveAchievements::on_all_achievement_finished);
}

void StoveAchievements::refresh_stat(const nx::string_view id) {
  if (!m_platform.game_support_ready())
    return;
  const nx::vector<wchar_t> wide_id = wide_from_utf8(id);
  Stove::PCSDK::GameSupport::GameSupport_Stat(
      wide_id.data(), &StoveAchievements::on_stat_finished);
}

void __cdecl StoveAchievements::on_all_achievement_finished(
    const Stove::PCSDK::CallbackResult result,
    Stove::PCSDK::GameSupport::StovePCAchievement *const achievements,
    const uint32_t count) {
  if (s_instance == nullptr || !result.GetResult().IsSuccessful())
    return;
  s_instance->m_achievement_ids.clear();
  s_instance->m_unlocked.clear();
  for (uint32_t i = 0; i < count; ++i) {
    nx::string id = utf8_from_wide(achievements[i].GetAchievementId());
    const wchar_t *const status = achievements[i].GetStatus();
    const bool unlocked = status != nullptr && std::wcscmp(status, L"ACHIEVED") == 0;
    s_instance->m_unlocked.insert_or_assign(id, unlocked);
    s_instance->m_achievement_ids.push_back(std::move(id));
  }
}

void __cdecl StoveAchievements::on_modify_stat_finished(
    const Stove::PCSDK::CallbackResult result,
    const Stove::PCSDK::GameSupport::StovePCModifyStatValue value) {
  if (s_instance == nullptr || !result.GetResult().IsSuccessful())
    return;
  s_instance->m_stats.insert_or_assign(s_instance->m_pending_stat_id,
                                       static_cast<f64>(value.GetCurrentValue()));
}

void __cdecl StoveAchievements::on_stat_finished(
    const Stove::PCSDK::CallbackResult result,
    const Stove::PCSDK::GameSupport::StovePCStat stat) {
  if (s_instance == nullptr || !result.GetResult().IsSuccessful())
    return;
  const Stove::PCSDK::GameSupport::StovePCStatFullId *const full_id =
      stat.GetStatFullId();
  if (full_id == nullptr)
    return;
  s_instance->m_stats.insert_or_assign(utf8_from_wide(full_id->GetStatId()),
                                       static_cast<f64>(stat.GetCurrentValue()));
}

}
