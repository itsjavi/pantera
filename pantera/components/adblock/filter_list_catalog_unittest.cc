// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/components/adblock/filter_list_catalog.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace pantera {
namespace {

constexpr char kCatalog[] = R"([
  {
    "uuid": "default",
    "title": "Brave Default Adblock Filters",
    "langs": [],
    "hidden": true,
    "default_enabled": true,
    "first_party_protections": true,
    "permission_mask": 1,
    "sources": [
      {"url": "https://lists.example/ubo.txt", "title": "uBlock Origin Filters",
       "format": "Standard"},
      {"url": "https://lists.example/brave-unbreak.txt",
       "title": "Brave Unbreak", "format": "Standard"},
      {"url": "http://lists.example/insecure.txt", "format": "Standard"},
      {"url": "https://lists.example/hosts.txt", "format": "Hosts"}
    ]
  },
  {
    "uuid": "FR",
    "title": "French website ad blocker",
    "langs": ["fr"],
    "default_enabled": false,
    "sources": [{"url": "https://lists.example/fr.txt", "format": "Standard"}]
  },
  {
    "uuid": "IOS",
    "title": "Brave IOS-Specific Filters",
    "platforms": ["IOS"],
    "default_enabled": true,
    "sources": [{"url": "https://lists.example/ios.txt", "format": "Standard"}]
  },
  {"uuid": "", "sources": [{"url": "https://lists.example/a.txt"}]},
  {"uuid": "no-sources", "sources": []},
  "not an entry"
])";

TEST(FilterListCatalogTest, ParsesEntries) {
  std::vector<FilterListCatalogEntry> catalog =
      ParseFilterListCatalog(kCatalog);
  ASSERT_EQ(catalog.size(), 3u);

  const FilterListCatalogEntry& entry = catalog[0];
  EXPECT_EQ(entry.uuid, "default");
  EXPECT_TRUE(entry.hidden);
  EXPECT_TRUE(entry.default_enabled);
  EXPECT_TRUE(entry.first_party_protections);
  EXPECT_EQ(entry.permission_mask, 1);
  // The http and hosts-format sources are left out.
  ASSERT_EQ(entry.sources.size(), 2u);
  EXPECT_EQ(entry.sources[0].url, GURL("https://lists.example/ubo.txt"));
  EXPECT_FALSE(entry.sources[0].IsBraveList());
  EXPECT_TRUE(entry.sources[1].IsBraveList());

  // Sources without a title take their entry's.
  EXPECT_EQ(catalog[1].sources[0].title, "French website ad blocker");
}

TEST(FilterListCatalogTest, MatchesLocalesAndPlatforms) {
  std::vector<FilterListCatalogEntry> catalog =
      ParseFilterListCatalog(kCatalog);
  ASSERT_EQ(catalog.size(), 3u);
  EXPECT_TRUE(catalog[1].MatchesLocale("fr"));
  EXPECT_TRUE(catalog[1].MatchesLocale("fr-CA"));
  EXPECT_FALSE(catalog[1].MatchesLocale("en-US"));
  EXPECT_FALSE(catalog[0].MatchesLocale("en-US"));

  EXPECT_TRUE(catalog[0].SupportsMacOS());
  EXPECT_FALSE(catalog[2].SupportsMacOS());
}

TEST(FilterListCatalogTest, RejectsInvalidJson) {
  EXPECT_TRUE(ParseFilterListCatalog("").empty());
  EXPECT_TRUE(ParseFilterListCatalog("{}").empty());
}

}  // namespace
}  // namespace pantera
