// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PANTERA_COMPONENTS_ADBLOCK_PREF_NAMES_H_
#define PANTERA_COMPONENTS_ADBLOCK_PREF_NAMES_H_

namespace pantera::prefs {

// Local state. Whether Pantera downloads filter list updates from the lists'
// maintainers (DECISION-15). Off, it only uses the copies it already has.
inline constexpr char kAdBlockListUpdatesEnabled[] =
    "pantera.adblock.list_updates_enabled";

// Local state. Lists the user turned on or off, by catalog uuid:
// {"<uuid>": true | false}. Lists not in it use the catalog's default.
inline constexpr char kAdBlockListSettings[] = "pantera.adblock.lists";

// Local state. Download state of each list file, by URL:
// {"<url>": {"last_attempt": time, "last_success": time,
//            "expires_hours": int}}.
inline constexpr char kAdBlockListDownloads[] =
    "pantera.adblock.list_downloads";

}  // namespace pantera::prefs

#endif  // PANTERA_COMPONENTS_ADBLOCK_PREF_NAMES_H_
