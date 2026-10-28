// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Ported to Pantera from brave-core (brave/brave-core@2f490708,
// components/brave_shields/content/browser/ad_block_engine.cc, where it is
// ResourceTypeToString). Pantera's changes: returns a string_view.

#include "pantera/browser/adblock/ad_block_request_type.h"

#include <string_view>

namespace pantera {

std::string_view GetAdBlockRequestType(blink::mojom::ResourceType type) {
  switch (type) {
    // top level page
    case blink::mojom::ResourceType::kMainFrame:
      return "main_frame";
    // frame or iframe
    case blink::mojom::ResourceType::kSubFrame:
      return "sub_frame";
    // a CSS stylesheet
    case blink::mojom::ResourceType::kStylesheet:
      return "stylesheet";
    // an external script
    case blink::mojom::ResourceType::kScript:
      return "script";
    // an image (jpg/gif/png/etc)
    case blink::mojom::ResourceType::kFavicon:
    case blink::mojom::ResourceType::kImage:
      return "image";
    // a font
    case blink::mojom::ResourceType::kFontResource:
      return "font";
    // an "other" subresource.
    case blink::mojom::ResourceType::kSubResource:
      return "other";
    // an object (or embed) tag for a plugin.
    case blink::mojom::ResourceType::kObject:
      return "object";
    // a media resource.
    case blink::mojom::ResourceType::kMedia:
      return "media";
    // a XMLHttpRequest
    case blink::mojom::ResourceType::kXhr:
      return "xhr";
    // a ping request for <a ping>/sendBeacon.
    case blink::mojom::ResourceType::kPing:
      return "ping";
    // the main resource of a dedicated worker.
    case blink::mojom::ResourceType::kWorker:
    // the main resource of a shared worker.
    case blink::mojom::ResourceType::kSharedWorker:
    // an explicitly requested prefetch
    case blink::mojom::ResourceType::kPrefetch:
    // the main resource of a service worker.
    case blink::mojom::ResourceType::kServiceWorker:
    // a report of Content Security Policy violations.
    case blink::mojom::ResourceType::kCspReport:
    // a resource that a plugin requested.
    case blink::mojom::ResourceType::kPluginResource:
    default:
      return "";
  }
}

}  // namespace pantera
