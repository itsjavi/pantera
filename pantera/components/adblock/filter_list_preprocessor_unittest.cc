// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/components/adblock/filter_list_preprocessor.h"

#include <optional>
#include <string>

#include "testing/gtest/include/gtest/gtest.h"

namespace pantera {
namespace {

TEST(FilterListPreprocessorTest, KeepsPlainLists) {
  EXPECT_EQ(PreprocessFilterList("! Title: x\n||a.example^\n", false),
            "! Title: x\n||a.example^\n\n");
}

TEST(FilterListPreprocessorTest, ResolvesConditions) {
  constexpr char kList[] =
      "||kept.example^\n"
      "!#if env_firefox\n"
      "||firefox.example^\n"
      "!#else\n"
      "||not-firefox.example^\n"
      "!#endif\n"
      "!#if !env_safari\n"
      "||not-safari.example^\n"
      "!#endif\n"
      "!#if cap_html_filtering\n"
      "!#if ext_ublock\n"
      "||nested-off.example^\n"
      "!#else\n"
      "||nested-else-off.example^\n"
      "!#endif\n"
      "!#endif\n"
      "!#if some_unknown_condition\n"
      "||unknown-if.example^\n"
      "!#else\n"
      "||unknown-else.example^\n"
      "!#endif\n";
  std::optional<std::string> result = PreprocessFilterList(kList, false);
  ASSERT_TRUE(result);
  EXPECT_EQ(*result,
            "||kept.example^\n"
            "||not-firefox.example^\n"
            "||not-safari.example^\n"
            "||unknown-if.example^\n"
            "||unknown-else.example^\n"
            "\n");
}

TEST(FilterListPreprocessorTest, RejectsUnbalancedConditions) {
  EXPECT_FALSE(PreprocessFilterList("!#if ext_ublock\n||a.example^\n", false));
  EXPECT_FALSE(PreprocessFilterList("||a.example^\n!#endif\n", false));
  EXPECT_FALSE(PreprocessFilterList("!#else\n", false));
}

TEST(FilterListPreprocessorTest, KeepsBraveScriptletsInBraveListsOnly) {
  constexpr char kList[] =
      "example.com##+js(brave-fix)\nexample.com##+js(set-constant, a, 1)\n";
  EXPECT_EQ(PreprocessFilterList(kList, true), std::string(kList) + "\n");
  EXPECT_EQ(PreprocessFilterList(kList, false),
            "example.com##+js(set-constant, a, 1)\n\n");
}

TEST(FilterListPreprocessorTest, DropsRulesAdblockRustCantUse) {
  EXPECT_EQ(
      PreprocessFilterList("/^dizipal\\d+\\.com$/##.ad\n||a.example^", false),
      "||a.example^\n");
}

}  // namespace
}  // namespace pantera
