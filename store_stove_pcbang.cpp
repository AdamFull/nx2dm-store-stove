#include "store_stove/store_stove_pcbang.h"

#include "core/foundation/diagnostics/log.h"

namespace nxm::store_stove {
namespace {

const nx::log::Category log_store_stove = nx::log::category("store_stove");

}

StovePCBang *StovePCBang::s_instance = nullptr;

StovePCBang::StovePCBang(StovePlatform &platform) noexcept
    : m_platform(platform) {
  s_instance = this;
}

StovePCBang::~StovePCBang() {
  if (s_instance == this)
    s_instance = nullptr;
}

bool StovePCBang::login() {
  if (!m_platform.pcbang_ready())
    return false;
  m_login_pending = true;
  Stove::PCSDK::PCBang::PCBang_UserLogin(
      &StovePCBang::login_callback, &StovePCBang::refresh_benefits_callback);
  return true;
}

void __cdecl StovePCBang::login_callback(
    const Stove::PCSDK::CallbackResult result,
    const Stove::PCSDK::PCBang::StovePCBangUserLogin userLogin) {
  if (s_instance == nullptr)
    return;
  s_instance->m_login_pending = false;
  if (!result.GetResult().IsSuccessful()) {
    nx::logw(log_store_stove, "PCBang_UserLogin failed: {}",
              result.GetResult().GetResultCode());
    return;
  }
  s_instance->m_logged_in = true;
  s_instance->m_premium_status = userLogin.GetPremiumStatus();
  s_instance->m_serial_number = userLogin.GetPCBangSerialNumber();
  s_instance->m_remain_time = userLogin.GetRemainTime();
}

void __cdecl StovePCBang::refresh_benefits_callback(
    const Stove::PCSDK::CallbackResult result,
    const Stove::PCSDK::PCBang::StovePCRefreshUserBenefits benefits) {
  if (s_instance == nullptr || !result.GetResult().IsSuccessful())
    return;
  s_instance->m_premium_status = benefits.GetPremiumStatus();
  s_instance->m_remain_time = benefits.GetRemainTime();
}

bool StovePCBang::logout() {
  if (!m_platform.pcbang_ready())
    return false;
  m_logout_pending = true;
  Stove::PCSDK::PCBang::PCBang_UserLogout(&StovePCBang::logout_callback);
  return true;
}

void __cdecl
StovePCBang::logout_callback(const Stove::PCSDK::CallbackResult result) {
  if (s_instance == nullptr)
    return;
  s_instance->m_logout_pending = false;
  if (!result.GetResult().IsSuccessful()) {
    nx::logw(log_store_stove, "PCBang_UserLogout failed: {}",
              result.GetResult().GetResultCode());
    return;
  }
  s_instance->m_logged_in = false;
  s_instance->m_premium_status = Stove::PCSDK::PCBang::PCBangPremium::PCBANG_ERROR;
  s_instance->m_serial_number = 0;
  s_instance->m_remain_time = 0;
  s_instance->m_product_code = 0;
}

bool StovePCBang::check_status() {
  if (!m_platform.pcbang_ready())
    return false;
  m_check_status_pending = true;
  Stove::PCSDK::PCBang::PCBang_CheckPCBangStatus(
      &StovePCBang::check_status_callback);
  return true;
}

void __cdecl StovePCBang::check_status_callback(
    const Stove::PCSDK::CallbackResult result,
    const Stove::PCSDK::PCBang::StovePCBangStatus status) {
  if (s_instance == nullptr)
    return;
  s_instance->m_check_status_pending = false;
  if (!result.GetResult().IsSuccessful()) {
    nx::logw(log_store_stove, "PCBang_CheckPCBangStatus failed: {}",
              result.GetResult().GetResultCode());
    return;
  }
  s_instance->m_premium_status = status.GetPremiumStatus();
  s_instance->m_serial_number = status.GetPCBangSerialNumber();
  s_instance->m_product_code = status.GetProductCode();
}

bool StovePCBang::is_pc_bang() const noexcept {
  using Stove::PCSDK::PCBang::PCBangPremium;
  return m_premium_status == PCBangPremium::PCBANG_PREMIUM ||
         m_premium_status == PCBangPremium::PCBANG_FREE;
}

bool StovePCBang::is_premium() const noexcept {
  return m_premium_status == Stove::PCSDK::PCBang::PCBangPremium::PCBANG_PREMIUM;
}

}
