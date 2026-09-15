#include "store_stove/store_stove_config.h"
#include "store_stove/store_stove_pcbang.h"
#include "store_stove/store_stove_platform.h"
#include "store_stove/store_stove_scripting.h"
#include "store_stove/store_stove_services.h"

#include "store/store_service.h"

#include "core/app/engine.h"
#include "core/app/module_system/module.h"
#include "core/app/module_system/module_context.h"

#include "core/foundation/diagnostics/log.h"

namespace nxm::store_stove {
namespace {

constexpr nx::string_view PUMP_SYSTEM = "store_stove.pump";
const nx::log::Category log_store_stove = nx::log::category("store_stove");

// store.cloud_saves and store.presence are deliberately absent - the
// vendored STOVE SDK has no file-storage or friends/presence API at all
// (confirmed absent from every header), the same "simply doesn't provide
// it" shape a missing subsystem already has elsewhere in this module
// family - see store_stove_services.h.
constexpr nxe::ModuleService PROVIDED_SERVICES[] = {
    {.id = store::kCoreService, .version = {1, 0, 0}},
    {.id = store::kIapService, .version = {1, 0, 0}},
    {.id = store::kAchievementsService, .version = {1, 0, 0}},
};

class StoreStoveModule final : public nxe::Module {
public:
  StoreStoveModule()
      : m_core(m_platform), m_iap(m_platform), m_achievements(m_platform),
        m_pcbang(m_platform) {}

  [[nodiscard]] nxe::ModuleDescriptor descriptor() const noexcept override {
    nxe::ModuleDescriptor out{};
    out.id = "store_stove";
    out.version = {1, 0, 0};
    out.provided_services = PROVIDED_SERVICES;
    // The vendored STOVE SDK ships Windows only.
    out.platforms = nxe::ModulePlatform::Windows;
    return out;
  }

  bool on_register(nxe::ModuleContext &ctx) override {
    nxe::ServiceRegistrar registrar = ctx.service_registrar();
    store::StoreCore &core = m_core;
    store::StoreIap &iap = m_iap;
    store::StoreAchievements &achievements = m_achievements;
    return registrar.provide(store::kCoreService, PROVIDED_SERVICES[0].version, core) &&
           registrar.provide(store::kIapService, PROVIDED_SERVICES[1].version, iap) &&
           registrar.provide(store::kAchievementsService,
                              PROVIDED_SERVICES[2].version, achievements);
  }

  bool on_attach(nxe::ModuleContext &ctx) override {
    if (!ctx.schedule().try_define(
            PUMP_SYSTEM, nxe::sys::SystemFn([this](const nxe::sys::Context &) {
              m_platform.tick();
            }))) {
      nx::logw(log_store_stove, "system '{}' is already owned by another module",
                PUMP_SYSTEM);
      return false;
    }
    ctx.schedule().add(nxe::sys::Stage::Update, PUMP_SYSTEM);

    if (const std::optional<ServiceConfig> config = load_project_config();
        config.has_value()) {
      PlatformConfig platform_config;
      platform_config.environment = config->environment;
      platform_config.game_id = config->game_id;
      platform_config.application_key = config->application_key;
      platform_config.shop_key = config->shop_key;
      if (m_platform.initialize(platform_config))
        nx::logi(log_store_stove, "attached, Game ID {}", config->game_id);
      else
        nx::logw(log_store_stove, "STOVE platform initialization failed");
    } else {
      nx::logi(log_store_stove, "no {} found; staying idle", kDefaultConfigPath);
    }

    return true;
  }

  void on_expose_scripts(nxe::script::Host &host, nxe::ModuleContext &) override {
    expose_store_stove_extras(host, m_pcbang);
  }

  void on_detach(nxe::ModuleContext &) override { m_platform.shutdown(); }

private:
  StovePlatform m_platform;
  StoveCore m_core;
  StoveIap m_iap;
  StoveAchievements m_achievements;

  // Stove-specific extra (PC Bang detection) - never part of
  // store_service.h's neutral interface, never registered through
  // ServiceRegistry (see store_stove_scripting.h).
  StovePCBang m_pcbang;
};

} // namespace
} // namespace nxm::store_stove

NX_DECLARE_MODULE(store_stove, nxm::store_stove::StoreStoveModule)
