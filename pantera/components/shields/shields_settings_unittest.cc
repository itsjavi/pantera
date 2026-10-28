// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/components/shields/shields_settings.h"

#include "base/memory/scoped_refptr.h"
#include "base/test/task_environment.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace pantera {
namespace {

class ShieldsSettingsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    HostContentSettingsMap::RegisterProfilePrefs(prefs_.registry());
    map_ = base::MakeRefCounted<HostContentSettingsMap>(
        &prefs_, /*is_off_the_record=*/false, /*store_last_modified=*/false,
        /*restore_session=*/false, /*should_record_metrics=*/false);
  }

  void TearDown() override { map_->ShutdownOnUIThread(); }

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  scoped_refptr<HostContentSettingsMap> map_;
};

TEST_F(ShieldsSettingsTest, ShieldsAreUpByDefaultOnWebPages) {
  EXPECT_TRUE(AreShieldsUp(*map_, GURL("https://news.example.com/")));
  EXPECT_TRUE(AreShieldsUp(*map_, GURL("http://127.0.0.1:8000/")));
  EXPECT_FALSE(AreShieldsUp(*map_, GURL("chrome://settings")));
  EXPECT_FALSE(AreShieldsUp(*map_, GURL()));
}

TEST_F(ShieldsSettingsTest, ShieldsDownCoversTheWholeSite) {
  SetShieldsUp(*map_, GURL("https://www.news.example.com/article"), false);
  EXPECT_FALSE(AreShieldsUp(*map_, GURL("https://www.news.example.com/")));
  EXPECT_FALSE(AreShieldsUp(*map_, GURL("http://example.com/")));
  EXPECT_FALSE(AreShieldsUp(*map_, GURL("https://cdn.example.com/")));
  EXPECT_TRUE(AreShieldsUp(*map_, GURL("https://example.org/")));

  SetShieldsUp(*map_, GURL("https://example.com/"), true);
  EXPECT_TRUE(AreShieldsUp(*map_, GURL("https://www.news.example.com/")));
}

TEST_F(ShieldsSettingsTest, HostsWithoutARegistrableDomain) {
  SetShieldsUp(*map_, GURL("http://localhost:8000/"), false);
  EXPECT_FALSE(AreShieldsUp(*map_, GURL("http://localhost:9000/")));
  EXPECT_TRUE(AreShieldsUp(*map_, GURL("http://127.0.0.1:8000/")));
}

TEST_F(ShieldsSettingsTest, AggressiveBlockingPerSite) {
  const GURL url("https://news.example.com/");
  EXPECT_FALSE(IsAggressiveBlocking(*map_, url));
  SetAggressiveBlocking(*map_, url, true);
  EXPECT_TRUE(IsAggressiveBlocking(*map_, url));
  EXPECT_TRUE(IsAggressiveBlocking(*map_, GURL("https://example.com/")));
  EXPECT_FALSE(IsAggressiveBlocking(*map_, GURL("https://example.org/")));
  SetAggressiveBlocking(*map_, url, false);
  EXPECT_FALSE(IsAggressiveBlocking(*map_, url));
}

TEST_F(ShieldsSettingsTest, YouTubeIsAlwaysAggressive) {
  EXPECT_TRUE(IsForcedAggressive(
      url::Origin::Create(GURL("https://www.youtube.com/watch?v=1"))));
  EXPECT_FALSE(
      IsForcedAggressive(url::Origin::Create(GURL("https://example.com/"))));
}

TEST_F(ShieldsSettingsTest, ScriptsPerSite) {
  const GURL url("https://news.example.com/");
  EXPECT_FALSE(AreScriptsBlocked(*map_, url));
  SetScriptsBlocked(*map_, url, true);
  EXPECT_TRUE(AreScriptsBlocked(*map_, url));
  EXPECT_TRUE(AreScriptsBlocked(*map_, GURL("https://example.com/")));
  EXPECT_FALSE(AreScriptsBlocked(*map_, GURL("https://example.org/")));
  SetScriptsBlocked(*map_, url, false);
  EXPECT_FALSE(AreScriptsBlocked(*map_, url));

  // With scripts blocked everywhere, unblocking a site needs an exception.
  map_->SetDefaultContentSetting(ContentSettingsType::JAVASCRIPT,
                                 CONTENT_SETTING_BLOCK);
  EXPECT_TRUE(AreScriptsBlocked(*map_, url));
  SetScriptsBlocked(*map_, url, false);
  EXPECT_FALSE(AreScriptsBlocked(*map_, url));
  EXPECT_TRUE(AreScriptsBlocked(*map_, GURL("https://example.org/")));
}

}  // namespace
}  // namespace pantera
