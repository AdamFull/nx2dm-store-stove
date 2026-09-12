#include "framework/nxtest.h"

#include "store_stove/store_stove_config.h"

#include "core/foundation/vfs/vfs.h"

namespace {

using nxm::store_stove::load_project_config;

struct VfsScope {
  VfsScope() { initialized = nx::vfs::initialize(); }
  ~VfsScope() { nx::vfs::shutdown(); }
  bool initialized = false;
};

[[nodiscard]] nx::blob<u8> as_bytes(const nx::string_view text) {
  return nx::blob<u8>(
      {reinterpret_cast<const u8 *>(text.data()), text.size()});
}

constexpr nx::string_view kValid = R"(
[store_stove]
environment = real
game_id = game-1
application_key = app-key-1
shop_key = shop-key-1
)";

} // namespace

TEST_CASE("store_stove config: no file present is not an error") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config().has_value());

  nx::vfs::unmount(mount);
}

TEST_CASE("store_stove config: a valid file is parsed in full") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/config/store_stove.ini", as_bytes(kValid));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  const std::optional<nxm::store_stove::ServiceConfig> config =
      load_project_config();
  REQUIRE(config.has_value());
  CHECK(config->environment.view() == "real");
  CHECK(config->game_id.view() == "game-1");
  CHECK(config->application_key.view() == "app-key-1");
  CHECK(config->shop_key.view() == "shop-key-1");

  nx::vfs::unmount(mount);
}

TEST_CASE("store_stove config: any missing required field is refused") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/config/store_stove.ini",
             as_bytes("[store_stove]\ngame_id = game-1\n"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config().has_value());

  nx::vfs::unmount(mount);
}

TEST_CASE("store_stove config: malformed ini does not crash") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/config/store_stove.ini", as_bytes("this is not [ini at all"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config().has_value());

  nx::vfs::unmount(mount);
}

TEST_CASE("store_stove config: an explicit path overrides the default") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/somewhere/else.ini", as_bytes(kValid));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config().has_value());
  const std::optional<nxm::store_stove::ServiceConfig> config =
      load_project_config("/somewhere/else.ini");
  REQUIRE(config.has_value());
  CHECK(config->game_id.view() == "game-1");

  nx::vfs::unmount(mount);
}
