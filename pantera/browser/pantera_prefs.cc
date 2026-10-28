// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/browser/pantera_prefs.h"

#include "pantera/components/adblock/ad_block_service.h"

namespace pantera {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  AdBlockService::RegisterLocalStatePrefs(registry);
}

}  // namespace pantera
