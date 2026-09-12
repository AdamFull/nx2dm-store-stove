#include "framework/nxtest.h"

#include "store_stove/store_stove_pcbang.h"
#include "store_stove/store_stove_platform.h"

// Same guard-path bar as test_store_stove_services.cpp: no live STOVE
// session runs here, so StovePlatform::pcbang_ready() is false throughout
// this binary and every operation below must refuse safely.

using namespace nxm::store_stove;

TEST_CASE("store_stove extras: PCBang refuses safely with no session") {
  StovePlatform platform;
  StovePCBang pcbang(platform);

  CHECK_FALSE(pcbang.login());
  CHECK_FALSE(pcbang.login_pending());
  CHECK_FALSE(pcbang.logged_in());

  CHECK_FALSE(pcbang.logout());
  CHECK_FALSE(pcbang.logout_pending());

  CHECK_FALSE(pcbang.check_status());
  CHECK_FALSE(pcbang.check_status_pending());

  CHECK_FALSE(pcbang.is_pc_bang());
  CHECK_FALSE(pcbang.is_premium());
  CHECK(pcbang.serial_number() == 0);
  CHECK(pcbang.remain_time() == 0);
  CHECK(pcbang.product_code() == 0);
}
