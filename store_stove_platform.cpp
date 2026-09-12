#include "store_stove/store_stove_platform.h"

#include "core/foundation/diagnostics/log.h"

#include <GameSupportSDK.h>
#include <IAPSDK.h>
#include <OwnershipSDK.h>
#include <PCBangSDK.h>

#include <Windows.h>

namespace nxm::store_stove {
namespace {

const nx::log::Category log_store_stove = nx::log::category("store_stove");

} // namespace

nx::vector<wchar_t> wide_from_utf8(const nx::string_view text) {
  if (text.empty())
    return nx::vector<wchar_t>(1, L'\0');
  const int need =
      ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                             static_cast<int>(text.size()), nullptr, 0);
  if (need <= 0)
    return nx::vector<wchar_t>(1, L'\0');
  nx::vector<wchar_t> out(static_cast<usize>(need) + 1, L'\0');
  ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                        static_cast<int>(text.size()), out.data(), need);
  return out;
}

nx::string utf8_from_wide(const wchar_t *const text) {
  if (text == nullptr || *text == L'\0')
    return {};
  const int wide_len = static_cast<int>(::wcslen(text));
  const int need = ::WideCharToMultiByte(CP_UTF8, 0, text, wide_len, nullptr,
                                         0, nullptr, nullptr);
  if (need <= 0)
    return {};
  nx::string out;
  out.resize(static_cast<usize>(need));
  ::WideCharToMultiByte(CP_UTF8, 0, text, wide_len, out.data(), need, nullptr,
                        nullptr);
  return out;
}

StovePlatform *StovePlatform::s_instance = nullptr;

StovePlatform::StovePlatform() noexcept = default;

StovePlatform::~StovePlatform() {
  if (s_instance == this)
    s_instance = nullptr;
}

bool StovePlatform::initialize(const PlatformConfig &config) {
  namespace Base = Stove::PCSDK::Base;

  const nx::vector<wchar_t> environment = wide_from_utf8(config.environment);
  const nx::vector<wchar_t> game_id = wide_from_utf8(config.game_id);
  const nx::vector<wchar_t> application_key =
      wide_from_utf8(config.application_key);

  Base::StovePCInitializeParam param;
  param.SetEnvironment(environment.data());
  param.SetGameID(game_id.data());
  param.SetApplicationKey(application_key.data());

  m_shop_key = config.shop_key;
  s_instance = this;
  m_initialize_called = true;
  Base::Base_Initialize(&param, &StovePlatform::on_initialize_finished);
  return true;
}

void StovePlatform::tick() {
  if (m_initialize_called)
    Stove::PCSDK::Base::Base_RunCallback();
}

void StovePlatform::shutdown() {
  namespace Base = Stove::PCSDK::Base;
  namespace Ownership = Stove::PCSDK::Ownership;
  namespace IAP = Stove::PCSDK::IAP;
  namespace GameSupport = Stove::PCSDK::GameSupport;
  namespace PCBang = Stove::PCSDK::PCBang;

  if (m_pcbang_ready) {
    PCBang::PCBang_UnInitialize();
    m_pcbang_ready = false;
  }
  if (m_game_support_ready) {
    GameSupport::GameSupport_UnInitialize();
    m_game_support_ready = false;
  }
  if (m_iap_ready) {
    IAP::IAP_UnInitialize();
    m_iap_ready = false;
  }
  if (m_ownership_ready) {
    Ownership::Ownership_UnInitialize();
    m_ownership_ready = false;
  }
  if (m_base_ready) {
    Base::Base_UnInitialize();
    m_base_ready = false;
  }
  m_initialize_called = false;
}

void __cdecl
StovePlatform::on_initialize_finished(const Stove::PCSDK::CallbackResult result) {
  if (s_instance != nullptr)
    s_instance->on_initialize_result(result);
}

void StovePlatform::on_initialize_result(
    const Stove::PCSDK::CallbackResult &result) {
  namespace Ownership = Stove::PCSDK::Ownership;
  namespace IAP = Stove::PCSDK::IAP;
  namespace GameSupport = Stove::PCSDK::GameSupport;
  namespace PCBang = Stove::PCSDK::PCBang;

  if (!result.GetResult().IsSuccessful()) {
    // Expected whenever the process wasn't launched through (or alongside)
    // the STOVE launcher - not an error, the same honest-failure shape
    // store_steam's SteamAPI_Init() already has when no Steam client is
    // running.
    nx::logi(log_store_stove, "Base_Initialize failed ({}) - staying idle",
              result.GetResult().GetResultCode());
    return;
  }
  m_base_ready = true;

  const Stove::PCSDK::Result ownership_result =
      Ownership::Ownership_Initialize();
  m_ownership_ready = ownership_result.IsSuccessful();

  const nx::vector<wchar_t> shop_key = wide_from_utf8(m_shop_key);
  const Stove::PCSDK::Result iap_result = IAP::IAP_Initialize(shop_key.data());
  m_iap_ready = iap_result.IsSuccessful();

  const Stove::PCSDK::Result game_support_result =
      GameSupport::GameSupport_Initialize();
  m_game_support_ready = game_support_result.IsSuccessful();

  const Stove::PCSDK::Result pcbang_result = PCBang::PCBang_Initialize();
  m_pcbang_ready = pcbang_result.IsSuccessful();

  nx::logi(log_store_stove, "Base_Initialize succeeded");
}

}
