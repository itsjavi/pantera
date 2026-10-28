// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Ported to Pantera from brave-core (brave/brave-core@2f490708,
// components/brave_shields/content/browser/ad_block_engine.cc, where it is
// ResourceTypeToString).

#ifndef PANTERA_BROWSER_ADBLOCK_AD_BLOCK_REQUEST_TYPE_H_
#define PANTERA_BROWSER_ADBLOCK_AD_BLOCK_REQUEST_TYPE_H_

#include <string_view>

#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"

namespace pantera {

// adblock-rust's request type for a Blink resource type, such as "script" or
// "sub_frame". Empty for types filter rules don't distinguish.
std::string_view GetAdBlockRequestType(blink::mojom::ResourceType type);

}  // namespace pantera

#endif  // PANTERA_BROWSER_ADBLOCK_AD_BLOCK_REQUEST_TYPE_H_
