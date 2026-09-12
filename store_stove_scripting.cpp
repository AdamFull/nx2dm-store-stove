#include "store_stove/store_stove_scripting.h"

#include "store_stove/store_stove_pcbang.h"

#include "core/script/script_host.h"

namespace nxm::store_stove {

void expose_store_stove_extras(nxe::script::Host &host, StovePCBang &pcbang) {
  host.expose_as("store_stove_pcbang_login",
                 [&pcbang]() { return pcbang.login(); });
  host.expose_as("store_stove_pcbang_login_pending",
                 [&pcbang]() { return pcbang.login_pending(); });
  host.expose_as("store_stove_pcbang_logged_in",
                 [&pcbang]() { return pcbang.logged_in(); });

  host.expose_as("store_stove_pcbang_logout",
                 [&pcbang]() { return pcbang.logout(); });
  host.expose_as("store_stove_pcbang_logout_pending",
                 [&pcbang]() { return pcbang.logout_pending(); });

  host.expose_as("store_stove_pcbang_check_status",
                 [&pcbang]() { return pcbang.check_status(); });
  host.expose_as("store_stove_pcbang_check_status_pending",
                 [&pcbang]() { return pcbang.check_status_pending(); });

  host.expose_as("store_stove_pcbang_is_pc_bang",
                 [&pcbang]() { return pcbang.is_pc_bang(); });
  host.expose_as("store_stove_pcbang_is_premium",
                 [&pcbang]() { return pcbang.is_premium(); });
  host.expose_as("store_stove_pcbang_serial_number", [&pcbang]() {
    return static_cast<f64>(pcbang.serial_number());
  });
  host.expose_as("store_stove_pcbang_remain_time", [&pcbang]() {
    return static_cast<f64>(pcbang.remain_time());
  });
  host.expose_as("store_stove_pcbang_product_code", [&pcbang]() {
    return static_cast<f64>(pcbang.product_code());
  });
}

}
