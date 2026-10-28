// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "pantera/components/adblock/resolver/adblock_domain_resolver.h"
#include "pantera/components/adblock/rs/src/lib.rs.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {
namespace {

using ::testing::HasSubstr;

std::vector<uint8_t> ToBytes(std::string_view text) {
  return std::vector<uint8_t>(text.begin(), text.end());
}

std::string ToString(const rust::String& text) {
  return std::string(text.data(), text.size());
}

class AdblockRustTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    // The resolver can be set once per process; later calls return false.
    static const bool kResolverSet = set_domain_resolver();
    ASSERT_TRUE(kResolverSet);
  }

  static rust::Box<Engine> MakeEngine(std::string_view rules) {
    rust::Box<FilterSet> filter_set = new_filter_set(/*debug=*/false);
    AddFilterListResult added = filter_set->add_filter_list(ToBytes(rules));
    EXPECT_EQ(added.result_kind, ResultKind::Success);
    BoxEngineResult engine = engine_from_filter_set(std::move(filter_set));
    EXPECT_EQ(engine.result_kind, ResultKind::Success);
    return std::move(engine.value);
  }

  static BlockerResult Match(const Engine& engine,
                             const std::string& url,
                             const std::string& hostname,
                             const std::string& initiator_hostname,
                             const std::string& request_type,
                             bool third_party) {
    return engine.matches(url, hostname, initiator_hostname, request_type,
                          third_party, "GET",
                          /*previously_matched_rule=*/false,
                          /*force_check_exceptions=*/false);
  }
};

TEST_F(AdblockRustTest, ResolvesRegistrableDomains) {
  DomainPosition position = resolve_domain_position("www.example.co.uk");
  EXPECT_EQ(position.start, 4u);
  EXPECT_EQ(position.end, 17u);
}

TEST_F(AdblockRustTest, BlocksMatchingRequests) {
  rust::Box<Engine> engine = MakeEngine("||ads.example.com^\n");
  EXPECT_TRUE(Match(*engine, "https://ads.example.com/banner.js",
                    "ads.example.com", "news.example.org", "script", true)
                  .matched);
  EXPECT_FALSE(Match(*engine, "https://cdn.example.org/app.js",
                     "cdn.example.org", "news.example.org", "script", false)
                   .matched);
}

TEST_F(AdblockRustTest, ExceptionsWin) {
  rust::Box<Engine> engine =
      MakeEngine("||example.com/ads/*\n@@||example.com/ads/ok.js\n");
  BlockerResult blocked = Match(*engine, "https://example.com/ads/banner.js",
                                "example.com", "example.com", "script", false);
  EXPECT_TRUE(blocked.matched);
  BlockerResult allowed = Match(*engine, "https://example.com/ads/ok.js",
                                "example.com", "example.com", "script", false);
  EXPECT_FALSE(allowed.matched);
  EXPECT_TRUE(allowed.has_exception);
}

TEST_F(AdblockRustTest, MatchesDomainOptionsOnSubdomains) {
  rust::Box<Engine> engine =
      MakeEngine("||tracker.net^$domain=example.co.uk\n");
  EXPECT_TRUE(Match(*engine, "https://tracker.net/pixel.gif", "tracker.net",
                    "news.example.co.uk", "image", true)
                  .matched);
  EXPECT_FALSE(Match(*engine, "https://tracker.net/pixel.gif", "tracker.net",
                     "news.other.co.uk", "image", true)
                   .matched);
}

TEST_F(AdblockRustTest, RedirectsToResources) {
  rust::Box<Engine> engine =
      MakeEngine("||ads.example.com/script.js$script,redirect=noop.js\n");
  // "(function() {})();" in base64.
  rust::Box<ResourceStorage> storage = new_resource_storage(
      R"([{"name": "noop.js", "aliases": [],
           "kind": {"mime": "application/javascript"},
           "content": "KGZ1bmN0aW9uKCkge30pKCk7"}])");
  EXPECT_TRUE(has_resource_for_testing(*storage, "noop.js"));
  engine->use_resource_storage(*storage);

  BlockerResult result =
      Match(*engine, "https://ads.example.com/script.js", "ads.example.com",
            "news.example.org", "script", true);
  EXPECT_TRUE(result.matched);
  ASSERT_TRUE(result.redirect.has_value);
  EXPECT_EQ(ToString(result.redirect.value),
            "data:application/javascript;base64,KGZ1bmN0aW9uKCkge30pKCk7");
}

TEST_F(AdblockRustTest, RemovesQueryParameters) {
  rust::Box<Engine> engine =
      MakeEngine("||example.com^$removeparam=utm_source\n");
  BlockerResult result =
      Match(*engine, "https://example.com/page?utm_source=feed&id=1",
            "example.com", "example.com", "document", false);
  ASSERT_TRUE(result.rewritten_url.has_value);
  EXPECT_EQ(ToString(result.rewritten_url.value),
            "https://example.com/page?id=1");
}

TEST_F(AdblockRustTest, ReturnsCspDirectives) {
  rust::Box<Engine> engine =
      MakeEngine("||example.com^$csp=script-src 'none'\n");
  rust::String directives =
      engine->get_csp_directives("https://example.com/", "example.com",
                                 "example.com", "document", false, "GET");
  EXPECT_EQ(ToString(directives), "script-src 'none'");
}

TEST_F(AdblockRustTest, ReturnsCosmeticResources) {
  rust::Box<Engine> engine =
      MakeEngine("example.com##.ad-banner\n##.sponsored\n");
  EXPECT_THAT(ToString(engine->url_cosmetic_resources("https://example.com/")),
              HasSubstr(".ad-banner"));
  EXPECT_THAT(ToString(engine->url_cosmetic_resources("https://example.org/")),
              ::testing::Not(HasSubstr(".ad-banner")));

  VecStringResult selectors =
      engine->hidden_class_id_selectors({"sponsored", "content"}, {}, {});
  ASSERT_EQ(selectors.result_kind, ResultKind::Success);
  ASSERT_EQ(selectors.value.size(), 1u);
  EXPECT_EQ(ToString(selectors.value[0]), ".sponsored");
}

TEST_F(AdblockRustTest, ReadsListMetadata) {
  FilterListMetadata metadata = read_list_metadata(
      ToBytes("! Title: Test list\n! Expires: 4 days\n||ads.example.com^\n"));
  ASSERT_TRUE(metadata.title.has_value);
  EXPECT_EQ(ToString(metadata.title.value), "Test list");
  ASSERT_TRUE(metadata.expires_hours.has_value);
  EXPECT_EQ(metadata.expires_hours.value, 96);
}

TEST_F(AdblockRustTest, RoundTripsSerializedEngines) {
  rust::Box<Engine> engine = MakeEngine("||ads.example.com^\n");
  rust::Vec<uint8_t> serialized = engine->serialize();
  ASSERT_FALSE(serialized.empty());

  rust::Box<Engine> loaded = new_engine();
  EXPECT_TRUE(loaded->deserialize(
      std::vector<uint8_t>(serialized.begin(), serialized.end())));
  EXPECT_TRUE(Match(*loaded, "https://ads.example.com/banner.js",
                    "ads.example.com", "news.example.org", "script", true)
                  .matched);
}

}  // namespace
}  // namespace adblock
