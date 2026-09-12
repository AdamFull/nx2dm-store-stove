#pragma once

#include "core/foundation/core/foundation.h"
#include "core/foundation/strings/utf8_string.h"

#include <BaseSDK.h>

namespace nxm::store_stove {

/// Converts UTF-8 to a null-terminated UTF-16 buffer - the entire STOVE SDK
/// takes/returns `wchar_t*`, but the neutral interface (and nx::string/
/// nx::string_view) are always UTF-8, so every call in this module crosses
/// this boundary at least once. Windows-only, matching the module's own
/// platform restriction.
[[nodiscard]] nx::vector<wchar_t> wide_from_utf8(nx::string_view text);
[[nodiscard]] nx::string utf8_from_wide(const wchar_t *text);

struct PlatformConfig {
  nx::string environment;
  nx::string game_id;
  nx::string application_key;
  nx::string shop_key;
};

/// Owns the four STOVE sub-SDKs this module uses (Base/Ownership/IAP/
/// GameSupport) and their init/tick/shutdown sequencing.
///
/// Two structural quirks drive this class's shape, both confirmed directly
/// from the vendored headers rather than assumed:
///
/// - Every STOVE callback is a **plain C function pointer with no userdata
///   parameter at all** (`typedef void(__cdecl*)(CallbackResult, ...)` -
///   contrast EOS's trailing `void* ClientData` or GOG's listener-object
///   dispatch). There is therefore no way to route a callback back to a
///   particular instance except through a static "current instance"
///   pointer per class - safe here because exactly one store backend
///   module (hence one instance of each service class) is ever active in a
///   process at a time, the same invariant `order_modules()` already
///   enforces via ServiceRegistry for "only one store active".
/// - Base_Initialize() is asynchronous (`OnInitializeFinished`), but
///   Ownership_Initialize()/IAP_Initialize()/GameSupport_Initialize() are
///   all synchronous (`Result`, returned directly) - so this class only
///   starts them once BaseSDK's own callback confirms success, storing
///   `shop_key` as a member since IAP_Initialize() needs it there, not at
///   the point PlatformConfig was passed to initialize().
///
/// STOVE also assumes a launcher already authenticated the user before the
/// game process started - there is no Login call anywhere in BaseSDK, only
/// read-only accessors for the already-signed-in session (Base_GetUser()
/// etc, not used by this module). `Base_RestartAppIfNecessary*()` - a
/// launcher-relaunch gate confirming the process was started through the
/// STOVE launcher - is deliberately not called here, the same choice
/// store_steam already makes for Steam's equivalent
/// `SteamAPI_RestartAppIfNecessary()`: packaging/distribution concern, not
/// this module's.
class StovePlatform {
public:
  StovePlatform() noexcept;
  ~StovePlatform();

  bool initialize(const PlatformConfig &config);
  void tick();
  void shutdown();

  [[nodiscard]] bool ready() const noexcept { return m_base_ready; }
  [[nodiscard]] bool ownership_ready() const noexcept { return m_ownership_ready; }
  [[nodiscard]] bool iap_ready() const noexcept { return m_iap_ready; }
  [[nodiscard]] bool game_support_ready() const noexcept {
    return m_game_support_ready;
  }

private:
  static void __cdecl on_initialize_finished(Stove::PCSDK::CallbackResult result);
  void on_initialize_result(const Stove::PCSDK::CallbackResult &result);

  bool m_initialize_called = false;
  bool m_base_ready = false;
  bool m_ownership_ready = false;
  bool m_iap_ready = false;
  bool m_game_support_ready = false;
  nx::string m_shop_key;

  static StovePlatform *s_instance;
};

}
