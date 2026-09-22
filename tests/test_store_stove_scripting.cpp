#include "framework/nxtest.h"

#include "app/engine.h"
#include "core/foundation/platform/filesystem.h"
#include "script/luau/luau_bindings.h"
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
};

} // namespace

TEST_CASE("store_stove scripting: every service is exposed as script-services.json "
          "declares it") {
  const Exposed exposed;
  const auto manifest = nx::fs::file_read_text(
      nx::fs::path_view(NX_MODULE_SERVICES_MANIFEST));
  REQUIRE(manifest);

  nx::string error;
  if (!script::luau_manifest_agrees(manifest.value(), exposed.services, error))
    FAIL(error.c_str());
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
