// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Ported to Pantera from Brave's list packager (brave/brave-core-crx-packager,
// lib/adBlockRustUtils.js and scripts/generateAdBlockRustDataFiles.js), which
// Brave runs on its servers before shipping lists to its browser. Pantera
// downloads lists from their maintainers (DECISION-15), so it runs these steps
// itself.

#ifndef PANTERA_COMPONENTS_ADBLOCK_FILTER_LIST_PREPROCESSOR_H_
#define PANTERA_COMPONENTS_ADBLOCK_FILTER_LIST_PREPROCESSOR_H_

#include <optional>
#include <string>
#include <string_view>

namespace pantera {

// Prepares a downloaded filter list for adblock-rust, as Brave's packager
// does:
// - Resolves `!#if`, `!#else` and `!#endif` for a uBlock Origin-compatible
//   Chromium browser. Unknown conditions keep both branches.
// - Drops Brave's own `+js(brave-…)` scriptlets unless `is_brave_list`.
// - Drops rules adblock-rust can't handle.
// Returns nullopt when the `!#if` directives don't balance, which Brave
// treats as a corrupted download.
std::optional<std::string> PreprocessFilterList(std::string_view list,
                                                bool is_brave_list);

}  // namespace pantera

#endif  // PANTERA_COMPONENTS_ADBLOCK_FILTER_LIST_PREPROCESSOR_H_
