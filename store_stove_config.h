#pragma once

#include "core/foundation/strings/utf8_string.h"

#include <optional>

namespace nxm::store_stove {

/// VFS path StoreStoveModule::on_attach() checks automatically. A project
/// enables the module by cooking a file to this path (author it at
/// assets/config/store_stove.ini) with its own STOVE partner credentials -
/// e.g.:
///
///   [store_stove]
///   environment = ...
///   game_id = ...
///   application_key = ...
///   shop_key = ...
///
/// `environment`/`game_id`/`application_key` configure BaseSDK
/// (Base_Initialize); `shop_key` is a separate credential IAPSDK needs
/// (IAP_Initialize) - all four are required. `application_key` and
/// `shop_key` are real secrets - this file likely shouldn't be committed to
/// a public repository.
inline constexpr nx::string_view kDefaultConfigPath = "/config/store_stove.ini";

struct ServiceConfig {
  nx::string environment;
  nx::string game_id;
  nx::string application_key;
  nx::string shop_key;
};

/// Reads and validates an INI file at @p path. Empty if the file does not
/// exist, or (with a logged warning) if it exists but fails to parse or is
/// missing a required field - callers should treat both the same way, as
/// "nothing to auto-configure with".
[[nodiscard]] std::optional<ServiceConfig>
load_project_config(nx::string_view path = kDefaultConfigPath);

}
