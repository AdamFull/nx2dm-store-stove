#include "framework/nxtest.h"

#include "store_stove/store_stove_platform.h"
#include "store_stove/store_stove_services.h"

// Like store_egs's/store_gog's own platform init, StovePlatform::initialize()
// drives a real Base_Initialize() call, which assumes the process was
// launched through (or alongside) the STOVE launcher - not something to
// fire in a unit test. Every service method here checks the relevant
// StovePlatform::*_ready() gate before touching any STOVE interface at
// all, so a never-initialized StovePlatform (all gates false) proves the
// same guard-path bar store_steam's/store_egs's/store_gog's own tests
// already established, without ever making a real STOVE call.

using namespace nxm::store_stove;

TEST_CASE("store_stove services: a never-initialized platform reports "
          "correctly") {
  const StovePlatform platform;
  CHECK_FALSE(platform.ready());
  CHECK_FALSE(platform.ownership_ready());
  CHECK_FALSE(platform.iap_ready());
  CHECK_FALSE(platform.game_support_ready());
}

TEST_CASE("store_stove services: StoveCore refuses safely with no platform") {
  StovePlatform platform;
  StoveCore core(platform);
  CHECK_FALSE(core.is_owned());
  CHECK_FALSE(core.is_owned("12345"));
  CHECK(core.owned_dlc_ids().empty());
  CHECK(core.store_name() == "stove");
  core.refresh_ownership();
  CHECK(core.owned_dlc_ids().empty());
}

TEST_CASE("store_stove services: StoveIap refuses safely with no platform") {
  StovePlatform platform;
  StoveIap iap(platform);
  CHECK(iap.products().empty());
  CHECK_FALSE(iap.purchase("12345"));
  CHECK_FALSE(iap.purchase_pending());
  CHECK(iap.purchase_error().empty());
  iap.refresh_products();
  CHECK(iap.products().empty());
}

TEST_CASE(
    "store_stove services: StoveAchievements refuses safely with no platform") {
  StovePlatform platform;
  StoveAchievements achievements(platform);
  // unlock() has no honest implementation against this SDK at all (see
  // store_stove_services.h) - it must refuse unconditionally, platform
  // state or not.
  CHECK_FALSE(achievements.unlock("first_win"));
  CHECK_FALSE(achievements.is_unlocked("first_win"));
  CHECK(achievements.achievement_ids().empty());
  CHECK_FALSE(achievements.set_stat("enemies_killed", 5.0));
  CHECK(achievements.stat("enemies_killed") == 0.0);
  achievements.refresh_achievements();
  achievements.refresh_stat("enemies_killed");
  CHECK(achievements.achievement_ids().empty());
  achievements.refresh({"enemies_killed"});
  CHECK(achievements.achievement_ids().empty());
  CHECK(achievements.stat("enemies_killed") == 0.0);
}
