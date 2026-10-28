// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PANTERA_COMPONENTS_SHIELDS_SHIELDS_SETTINGS_H_
#define PANTERA_COMPONENTS_SHIELDS_SHIELDS_SETTINGS_H_

#include "url/gurl.h"

class HostContentSettingsMap;

namespace content_settings {
class CookieSettings;
}  // namespace content_settings

namespace url {
class Origin;
}  // namespace url

namespace pantera {

// Per-site shields settings, keyed on the top-level site and stored in the
// PANTERA_SHIELDS and PANTERA_FIRST_PARTY_ADS content settings. A setting for
// a site covers its subdomains.

// Whether Pantera's protections run on `top_url`'s site. On by default, and
// for pages that aren't http or https there is nothing to protect.
bool AreShieldsUp(const HostContentSettingsMap& map, const GURL& top_url);
void SetShieldsUp(HostContentSettingsMap& map, const GURL& top_url, bool up);

// Whether ad blocking is aggressive on `top_url`'s site: every filter rule
// also applies to first-party content (DECISION-17). Standard by default.
bool IsAggressiveBlocking(const HostContentSettingsMap& map,
                          const GURL& top_url);
void SetAggressiveBlocking(HostContentSettingsMap& map,
                           const GURL& top_url,
                           bool aggressive);

// youtube.com is always blocked aggressively, as in Brave: its ads are
// first-party.
bool IsForcedAggressive(const url::Origin& request_initiator);

// The Shields panel's other rows use Chromium's own settings for the site
// (DECISION-19).

// Whether JavaScript is blocked on `top_url`'s site.
bool AreScriptsBlocked(const HostContentSettingsMap& map, const GURL& top_url);
void SetScriptsBlocked(HostContentSettingsMap& map,
                       const GURL& top_url,
                       bool blocked);

enum class CookieMode {
  kBlockThirdParty,
  kBlockAll,
  kAllowAll,
};

// Which cookies `top_url` and the sites it embeds may use.
CookieMode GetCookieMode(const HostContentSettingsMap& map,
                         const content_settings::CookieSettings& cookies,
                         const GURL& top_url);
void SetCookieMode(const HostContentSettingsMap& map,
                   content_settings::CookieSettings& cookies,
                   const GURL& top_url,
                   CookieMode mode);

}  // namespace pantera

#endif  // PANTERA_COMPONENTS_SHIELDS_SHIELDS_SETTINGS_H_
