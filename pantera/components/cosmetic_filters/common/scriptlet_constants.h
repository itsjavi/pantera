// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Ported to Pantera from brave-core (brave/brave-core@2f490708,
// components/cosmetic_filters/common/scriptlet_constants.h).
// Pantera's changes: include path and namespace.

#ifndef PANTERA_COMPONENTS_COSMETIC_FILTERS_COMMON_SCRIPTLET_CONSTANTS_H_
#define PANTERA_COMPONENTS_COSMETIC_FILTERS_COMMON_SCRIPTLET_CONSTANTS_H_

#include <string>

namespace pantera {

// Builds the globals block that is prepended to injected scriptlets on all
// platforms. It defines:
//   - `scriptletGlobals`, a `Map` wrapped in a `Proxy` that some of uBlock
//     Origin's scriptlet resources rely on.
//   - `deAmpEnabled`, used by the de-amp scriptlet resource.
//
// `is_de_amp_enabled` toggles de-amping. `can_debug` enables uBlock Origin's
// scriptlet debug logging and is only set on non-iOS platforms.
std::string GetScriptletGlobalsScript(bool is_de_amp_enabled, bool can_debug);

}  // namespace pantera

#endif  // PANTERA_COMPONENTS_COSMETIC_FILTERS_COMMON_SCRIPTLET_CONSTANTS_H_
