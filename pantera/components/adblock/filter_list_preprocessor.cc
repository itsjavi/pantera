// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Ported to Pantera from Brave's list packager (brave/brave-core-crx-packager,
// lib/adBlockRustUtils.js and scripts/generateAdBlockRustDataFiles.js).
// Pantera's changes: an `!#else` inside a block that is already off stays off
// (the packager turned it on), and the check for adblock-rust 0.8.6, which
// Pantera doesn't use, is gone.

#include "pantera/components/adblock/filter_list_preprocessor.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/fixed_flat_map.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace pantera {

namespace {

// The packager's IF_CONDITIONS: which `!#if` conditions hold in a Chromium
// browser that runs uBlock Origin's syntax. Conditions not listed keep both
// branches.
constexpr auto kIfConditions = base::MakeFixedFlatMap<std::string_view, bool>({
    {"adguard", false},
    {"adguard_app_android", false},
    {"adguard_app_ios", false},
    {"adguard_app_mac", false},
    {"adguard_app_windows", false},
    {"adguard_ext_android_cb", false},
    {"adguard_ext_chromium", true},
    {"adguard_ext_edge", false},
    {"adguard_ext_firefox", false},
    {"adguard_ext_opera", true},
    {"adguard_ext_safari", false},
    {"cap_html_filtering", false},
    {"cap_user_stylesheet", true},
    {"env_chromium", true},
    {"env_devbuild", false},
    {"env_edge", false},
    {"env_firefox", false},
    {"env_legacy", false},
    {"env_safari", false},
    {"ext_abp", false},
    {"ext_devbuild", false},
    {"ext_ublock", true},
    {"false", false},
});

enum class IfState {
  // Lines are kept.
  kTrue,
  // Lines are dropped; `!#else` keeps the ones after it.
  kFalse,
  // Unknown condition: lines in both branches are kept.
  kUnknown,
  // Inside a block that is off: everything is dropped, `!#else` included.
  kDisabled,
};

}  // namespace

std::optional<std::string> PreprocessFilterList(std::string_view list,
                                                bool is_brave_list) {
  std::vector<IfState> stack;
  std::string result;
  result.reserve(list.size());

  for (std::string_view line : base::SplitStringPiece(
           list, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL)) {
    const std::string_view trimmed =
        base::TrimWhitespaceASCII(line, base::TRIM_ALL);
    const bool active = stack.empty() || stack.back() == IfState::kTrue ||
                        stack.back() == IfState::kUnknown;

    if (trimmed.starts_with("!#if ")) {
      if (!active) {
        stack.push_back(IfState::kDisabled);
        continue;
      }
      std::string_view condition = trimmed.substr(5);
      const bool negate = condition.starts_with('!');
      if (negate) {
        condition.remove_prefix(1);
      }
      const auto value = kIfConditions.find(condition);
      if (value == kIfConditions.end()) {
        stack.push_back(IfState::kUnknown);
      } else {
        stack.push_back(value->second != negate ? IfState::kTrue
                                                : IfState::kFalse);
      }
      continue;
    }
    if (trimmed == "!#else") {
      if (stack.empty()) {
        return std::nullopt;
      }
      if (stack.back() == IfState::kTrue) {
        stack.back() = IfState::kFalse;
      } else if (stack.back() == IfState::kFalse) {
        stack.back() = IfState::kTrue;
      }
      continue;
    }
    if (trimmed == "!#endif") {
      if (stack.empty()) {
        return std::nullopt;
      }
      stack.pop_back();
      continue;
    }
    if (!active) {
      continue;
    }

    // Only Brave's own lists may use Brave's scriptlets.
    if (!is_brave_list && line.find("+js(brave-") != std::string_view::npos) {
      continue;
    }
    // Rules the packager drops for adblock-rust.
    if (trimmed.starts_with("/^dizipal\\d+\\.com$/##")) {
      continue;
    }

    result.append(line);
    result.push_back('\n');
  }

  if (!stack.empty()) {
    return std::nullopt;
  }
  return result;
}

}  // namespace pantera
