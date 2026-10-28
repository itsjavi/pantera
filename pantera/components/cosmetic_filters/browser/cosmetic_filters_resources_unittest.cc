// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/components/cosmetic_filters/browser/cosmetic_filters_resources.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "pantera/components/adblock/ad_block_engine_wrapper.h"
#include "pantera/components/shields/shields_settings.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace pantera {
namespace {

constexpr char kNews[] = "https://news.example/";

rust::Box<adblock::FilterSet> MakeFilterSet(std::string_view rules) {
  rust::Box<adblock::FilterSet> filter_set =
      adblock::new_filter_set(/*debug=*/false);
  filter_set->add_filter_list(std::vector<uint8_t>(rules.begin(), rules.end()));
  return filter_set;
}

class CosmeticFiltersResourcesTest : public ::testing::Test {
 protected:
  void SetUp() override {
    HostContentSettingsMap::RegisterProfilePrefs(prefs_.registry());
    map_ = base::MakeRefCounted<HostContentSettingsMap>(
        &prefs_, /*is_off_the_record=*/false, /*store_last_modified=*/false,
        /*restore_session=*/false, /*should_record_metrics=*/false);

    // Once per process; later calls return false.
    adblock::set_domain_resolver();
    wrapper_ = AdBlockEngineWrapper::Create();
    // The default engine's lists have first-party protections; the other
    // engine's don't.
    ASSERT_TRUE(
        wrapper_->Load(true,
                       MakeFilterSet("news.example##.ad\n"
                                     "news.example##div:has(> .sponsored)\n"
                                     "youtube.com##.video-ad\n"),
                       *storage_));
    ASSERT_TRUE(wrapper_->Load(
        false,
        MakeFilterSet("news.example##.promo\n"
                      "news.example##.box:has-text(Sponsored)\n"),
        *storage_));
    resources_ =
        std::make_unique<CosmeticFiltersResources>(wrapper_.get(), map_);
  }

  void TearDown() override { map_->ShutdownOnUIThread(); }

  std::optional<base::DictValue> Get(const std::string& url,
                                     const std::string& top_frame_url,
                                     bool cross_origin = false) {
    base::test::TestFuture<std::optional<base::DictValue>> future;
    resources_->UrlCosmeticResources(GURL(url), GURL(top_frame_url),
                                     cross_origin, future.GetCallback());
    return future.Take();
  }

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  scoped_refptr<HostContentSettingsMap> map_;
  rust::Box<adblock::ResourceStorage> storage_ =
      adblock::new_empty_resource_storage();
  std::unique_ptr<AdBlockEngineWrapper> wrapper_;
  std::unique_ptr<CosmeticFiltersResources> resources_;
};

TEST_F(CosmeticFiltersResourcesTest, StandardModeSparesFirstPartyContent) {
  std::optional<base::DictValue> resources = Get(kNews, kNews);
  ASSERT_TRUE(resources);
  EXPECT_EQ(resources->FindBool("hide_first_party"), false);
  // The default engine's `:has()` and procedural filters are left out.
  EXPECT_EQ(*resources->FindList("hide_selectors"),
            base::test::ParseJsonList(R"([".ad"])"));
  EXPECT_EQ(*resources->FindList("force_hide_selectors"),
            base::test::ParseJsonList(R"([".promo"])"));
  // The other engine's procedural filter becomes a script for the page.
  EXPECT_TRUE(resources->FindString("procedural_actions_script"));
  EXPECT_FALSE(resources->Find("procedural_actions"));
}

TEST_F(CosmeticFiltersResourcesTest, AggressiveModeHidesEverything) {
  SetAggressiveBlocking(*map_, GURL(kNews), true);
  std::optional<base::DictValue> resources = Get(kNews, kNews);
  ASSERT_TRUE(resources);
  EXPECT_EQ(resources->FindBool("hide_first_party"), true);
  EXPECT_EQ(resources->FindList("hide_selectors")->size(), 2u);
}

TEST_F(CosmeticFiltersResourcesTest, FramesFromOtherSitesAndYouTube) {
  // A frame from another site hides its own content as third-party.
  std::optional<base::DictValue> frame =
      Get("https://widgets.example/embed", kNews, /*cross_origin=*/true);
  ASSERT_TRUE(frame);
  EXPECT_EQ(frame->FindBool("hide_first_party"), true);

  std::optional<base::DictValue> youtube =
      Get("https://www.youtube.com/watch?v=1", "https://www.youtube.com/");
  ASSERT_TRUE(youtube);
  EXPECT_EQ(youtube->FindBool("hide_first_party"), true);
  EXPECT_EQ(*youtube->FindList("hide_selectors"),
            base::test::ParseJsonList(R"([".video-ad"])"));
}

TEST_F(CosmeticFiltersResourcesTest, NothingWithShieldsDown) {
  SetShieldsUp(*map_, GURL(kNews), false);
  EXPECT_FALSE(Get(kNews, kNews));
  // The top-level site decides, also for its frames from other sites.
  EXPECT_FALSE(Get("https://widgets.example/embed", kNews, true));
  EXPECT_TRUE(Get("https://widgets.example/", "https://widgets.example/"));
  // Nothing for pages that aren't on the web.
  EXPECT_FALSE(Get("chrome://settings/", "chrome://settings/"));
}

TEST_F(CosmeticFiltersResourcesTest, HiddenClassIdSelectors) {
  ASSERT_TRUE(
      wrapper_->Load(false, MakeFilterSet("##.banner-ad\n##.ad\n"), *storage_));
  base::test::TestFuture<base::DictValue> future;
  resources_->HiddenClassIdSelectors(
      R"({"classes": ["banner-ad", "article"], "ids": []})", {},
      future.GetCallback());
  EXPECT_EQ(*future.Get().FindList("force_hide_selectors"),
            base::test::ParseJsonList(R"([".banner-ad"])"));
}

}  // namespace
}  // namespace pantera
