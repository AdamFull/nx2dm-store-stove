#include "framework/nxtest.h"

#include "app/engine.h"
#include "script/script_host.h"
#include "store_stove/store_stove_pcbang.h"
#include "store_stove/store_stove_platform.h"
#include "store_stove/store_stove_scripting.h"

namespace {

using namespace nxm::store_stove;
namespace script = nxe::script;

struct Exposed {
  StovePlatform platform;
  StovePCBang pcbang{platform};
  script::Host host;
  nx::vector<script::Host::ServiceInfo> services;

  Exposed() {
    expose_store_stove_extras(host, pcbang);
    services = host.services();
  }

  [[nodiscard]] const script::Host::ServiceInfo *
  find(const nx::string_view name) const {
    for (const script::Host::ServiceInfo &one : services)
      if (one.name == name)
        return &one;
    return nullptr;
  }
};

} // namespace

// This is the one place a mismatch between what store_stove_scripting.cpp
// actually registers and what modules/store_stove/script-services.json
// declares to Luau would show up - see test_modio_scripting.cpp's identical
// role for modio. No backend is registered in this harness (no live STOVE
// session runs), so every callable here just exercises its own "no session"
// refusal path, not real STOVE behavior.
TEST_CASE("store_stove scripting: every service is exposed with the shape "
          "a script is told about") {
  const Exposed exposed;

  static constexpr struct {
    nx::string_view name;
    nx::string_view signature;
  } WANT[] = {
      {"store_stove_pcbang_login", "()->(boolean)"},
      {"store_stove_pcbang_login_pending", "()->(boolean)"},
      {"store_stove_pcbang_logged_in", "()->(boolean)"},
      {"store_stove_pcbang_logout", "()->(boolean)"},
      {"store_stove_pcbang_logout_pending", "()->(boolean)"},
      {"store_stove_pcbang_check_status", "()->(boolean)"},
      {"store_stove_pcbang_check_status_pending", "()->(boolean)"},
      {"store_stove_pcbang_is_pc_bang", "()->(boolean)"},
      {"store_stove_pcbang_is_premium", "()->(boolean)"},
      {"store_stove_pcbang_serial_number", "()->(number)"},
      {"store_stove_pcbang_remain_time", "()->(number)"},
      {"store_stove_pcbang_product_code", "()->(number)"},
  };

  CHECK(exposed.services.size() == nx::array_size(WANT));
  for (const auto &want : WANT) {
    const script::Host::ServiceInfo *const found = exposed.find(want.name);
    REQUIRE(found != nullptr);
    CHECK(found->signature == want.signature);
  }
}

TEST_CASE("store_stove scripting: the module hands them over on its own") {
  std::unique_ptr<nxe::Module> found;
  for (const nxe::ModuleFactory factory : nxe::enabled_module_factories()) {
    std::unique_ptr<nxe::Module> module = factory();
    if (module != nullptr && module->name() == "store_stove")
      found = std::move(module);
  }
  REQUIRE(found != nullptr);

  nxe::Engine engine{nxe::Game{}};
  nxe::ModuleContext ctx{engine};
  script::Host host;
  found->on_expose_scripts(host, ctx);

  script::Host direct;
  StovePlatform platform;
  StovePCBang pcbang{platform};
  expose_store_stove_extras(direct, pcbang);
  CHECK(host.exposed_count() == direct.exposed_count());
  CHECK(host.exposed_count() > 0u);
}
