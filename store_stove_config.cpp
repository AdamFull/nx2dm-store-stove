#include "store_stove/store_stove_config.h"

#include "core/foundation/diagnostics/log.h"
#include "core/foundation/serialization/ini.h"
#include "core/foundation/vfs/vfs.h"

namespace nxm::store_stove {
namespace {

const nx::log::Category log_store_stove = nx::log::category("store_stove");

[[nodiscard]] const nx::string *
require(const nx::ini::Document &doc, const nx::string_view key,
        const nx::string_view path, bool &ok) {
  const nx::string *const value = doc.find("store_stove", key);
  if (value == nullptr) {
    nx::logw(log_store_stove, "config: '{}' is missing [store_stove] {}",
              path, key);
    ok = false;
  }
  return value;
}

} // namespace

std::optional<ServiceConfig> load_project_config(const nx::string_view path) {
  const auto text = nx::vfs::read_text(path);
  if (!text) {
    if (text.error().kind != nx::fs::io_error::NotFound)
      nx::logw(log_store_stove, "config: could not read '{}': {}", path,
                nx::fs::to_string(text.error().kind));
    return {};
  }

  const auto parsed = nx::ini::parse(text->view());
  if (!parsed) {
    const nx::ini::ParseError &error = parsed.error();
    nx::logw(log_store_stove, "config: {}:{}:{}: {}", path, error.line,
              error.column, error.message());
    return {};
  }

  bool ok = true;
  const nx::string *const environment = require(*parsed, "environment", path, ok);
  const nx::string *const game_id = require(*parsed, "game_id", path, ok);
  const nx::string *const application_key =
      require(*parsed, "application_key", path, ok);
  const nx::string *const shop_key = require(*parsed, "shop_key", path, ok);
  if (!ok)
    return {};

  ServiceConfig config;
  config.environment = *environment;
  config.game_id = *game_id;
  config.application_key = *application_key;
  config.shop_key = *shop_key;
  return config;
}

}
