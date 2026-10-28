// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PANTERA_BROWSER_PANTERA_PREFS_H_
#define PANTERA_BROWSER_PANTERA_PREFS_H_

class PrefRegistrySimple;

namespace pantera {

// Registers Pantera's local state prefs. Called from Chrome's
// RegisterLocalState().
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

}  // namespace pantera

#endif  // PANTERA_BROWSER_PANTERA_PREFS_H_
