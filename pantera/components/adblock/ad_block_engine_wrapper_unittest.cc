// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/components/adblock/ad_block_engine_wrapper.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace pantera {
namespace {

rust::Box<adblock::FilterSet> MakeFilterSet(std::string_view rules) {
  rust::Box<adblock::FilterSet> filter_set =
      adblock::new_filter_set(/*debug=*/false);
  filter_set->add_filter_list(std::vector<uint8_t>(rules.begin(), rules.end()));
  return filter_set;
}

class AdBlockEngineWrapperTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Once per process; later calls return false.
    adblock::set_domain_resolver();
    wrapper_ = AdBlockEngineWrapper::Create();
  }

  void Load(std::string_view default_rules, std::string_view additional_rules) {
    ASSERT_TRUE(wrapper_->Load(true, MakeFilterSet(default_rules), *storage_));
    ASSERT_TRUE(
        wrapper_->Load(false, MakeFilterSet(additional_rules), *storage_));
  }

  bool Blocks(const std::string& url,
              const std::string& initiator,
              bool aggressive) {
    return wrapper_
        ->ShouldStartRequest(GURL(url), "script",
                             url::Origin::Create(GURL(initiator)), "GET",
                             aggressive, false, false, false)
        .matched;
  }

  rust::Box<adblock::ResourceStorage> storage_ =
      adblock::new_empty_resource_storage();
  std::unique_ptr<AdBlockEngineWrapper> wrapper_;
};

TEST_F(AdBlockEngineWrapperTest, StandardModeSparesFirstPartyDefaultRules) {
  Load("||news.example/ads/*\n||tracker.example^\n",
       "||news.example/promo/*\n");

  // Third-party requests are blocked by both engines.
  EXPECT_TRUE(Blocks("https://tracker.example/t.js", "https://news.example",
                     /*aggressive=*/false));
  // First-party requests skip the default engine in standard mode...
  EXPECT_FALSE(Blocks("https://news.example/ads/a.js", "https://news.example",
                      /*aggressive=*/false));
  EXPECT_TRUE(Blocks("https://news.example/ads/a.js", "https://news.example",
                     /*aggressive=*/true));
  // ...but not the additional engine.
  EXPECT_TRUE(Blocks("https://news.example/promo/p.js", "https://news.example",
                     /*aggressive=*/false));
}

// As in Brave, standard mode drops even `$important` matches of the default
// engine for first-party requests; for third-party ones they win over the
// additional engine's exceptions.
TEST_F(AdBlockEngineWrapperTest, ImportantDefaultRules) {
  Load("||news.example/ads/*$important\n", "@@||news.example/ads/allowed.js\n");
  EXPECT_FALSE(Blocks("https://news.example/ads/a.js", "https://news.example",
                      /*aggressive=*/false));
  EXPECT_TRUE(Blocks("https://news.example/ads/allowed.js",
                     "https://other.example", /*aggressive=*/false));
}

TEST_F(AdBlockEngineWrapperTest, MergesCosmeticResources) {
  Load("news.example##.ad\nnews.example##div:has(> .sponsored)\n",
       "news.example##.promo\n");

  base::DictValue standard =
      wrapper_->UrlCosmeticResources("https://news.example/", false);
  EXPECT_EQ(*standard.FindList("hide_selectors"),
            base::test::ParseJsonList(R"([".ad"])"));
  EXPECT_EQ(*standard.FindList("force_hide_selectors"),
            base::test::ParseJsonList(R"([".promo"])"));

  base::DictValue aggressive =
      wrapper_->UrlCosmeticResources("https://news.example/", true);
  EXPECT_EQ(aggressive.FindList("hide_selectors")->size(), 2u);
}

TEST_F(AdBlockEngineWrapperTest, StripsProceduralFilters) {
  base::DictValue resources = base::test::ParseJsonDict(R"({
    "procedural_actions": [
      "{\"selector\":[{\"type\":\"css-selector\",\"arg\":\".ad\"}],\"action\":{\"type\":\"remove\"}}",
      "{\"selector\":[{\"type\":\"css-selector\",\"arg\":\"div\"},{\"type\":\"has-text\",\"arg\":\"Ad\"}]}",
      "{\"selector\":[{\"type\":\"has-text\",\"arg\":\"Ad\"}]}",
      "not json",
      7
    ]
  })");
  AdBlockEngineWrapper::StripProceduralFilters(resources);
  EXPECT_EQ(resources.FindList("procedural_actions")->size(), 3u);
}

}  // namespace
}  // namespace pantera
