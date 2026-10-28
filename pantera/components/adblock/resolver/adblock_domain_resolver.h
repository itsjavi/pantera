/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Ported to Pantera from brave-core (brave/brave-core@2f490708,
 * components/brave_shields/core/common/adblock/resolver/adblock_domain_resolver.h).
 * Pantera's changes: include paths and header guard. */

#ifndef PANTERA_COMPONENTS_ADBLOCK_RESOLVER_ADBLOCK_DOMAIN_RESOLVER_H_
#define PANTERA_COMPONENTS_ADBLOCK_RESOLVER_ADBLOCK_DOMAIN_RESOLVER_H_

#include <stdint.h>

#include <string>

namespace adblock {

struct DomainPosition;

DomainPosition resolve_domain_position(const std::string& host);

}  // namespace adblock

#endif  // PANTERA_COMPONENTS_ADBLOCK_RESOLVER_ADBLOCK_DOMAIN_RESOLVER_H_
