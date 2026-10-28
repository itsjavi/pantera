// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Ported to Pantera from brave-core (brave/brave-core@2f490708,
// components/brave_shields/content/browser/adblock_stub_response.h).
// Pantera's changes: include guard and namespace.

#ifndef PANTERA_COMPONENTS_ADBLOCK_ADBLOCK_STUB_RESPONSE_H_
#define PANTERA_COMPONENTS_ADBLOCK_ADBLOCK_STUB_RESPONSE_H_

#include <optional>
#include <string>

#include "services/network/public/mojom/url_response_head.mojom-forward.h"

namespace network {
struct ResourceRequest;
}  // namespace network

namespace pantera {

// Intercepts certain requests and blocks them by silently returning 200 OK
// and not allowing them to hit the network.
void MakeStubResponse(const std::optional<std::string>& data_url,
                      const network::ResourceRequest& request,
                      network::mojom::URLResponseHeadPtr* response,
                      std::string* data);

}  // namespace pantera

#endif  // PANTERA_COMPONENTS_ADBLOCK_ADBLOCK_STUB_RESPONSE_H_
