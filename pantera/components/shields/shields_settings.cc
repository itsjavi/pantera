// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/components/shields/shields_settings.h"

#include <string>

#include "base/strings/strcat.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace pantera {

namespace {

using net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES;

// The site and its subdomains, on any scheme and port: "[*.]example.com".
ContentSettingsPattern SitePattern(const GURL& url) {
  std::string site = net::registry_controlled_domains::GetDomainAndRegistry(
      url, INCLUDE_PRIVATE_REGISTRIES);
  if (site.empty()) {
    // IP addresses, localhost and other hosts without a registrable domain.
    return ContentSettingsPattern::FromString(std::string(url.host()));
  }
  return ContentSettingsPattern::FromString(base::StrCat({"[*.]", site}));
}

bool IsWebUrl(const GURL& url) {
  return url.is_valid() && url.SchemeIsHTTPOrHTTPS();
}

}  // namespace

bool AreShieldsUp(const HostContentSettingsMap& map, const GURL& top_url) {
  if (!IsWebUrl(top_url)) {
    return false;
  }
  return map.GetContentSetting(top_url, GURL(),
                               ContentSettingsType::PANTERA_SHIELDS) !=
         CONTENT_SETTING_BLOCK;
}

void SetShieldsUp(HostContentSettingsMap& map, const GURL& top_url, bool up) {
  if (!IsWebUrl(top_url)) {
    return;
  }
  // On is the default, so turning shields back on removes the exception.
  map.SetContentSettingCustomScope(
      SitePattern(top_url), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::PANTERA_SHIELDS,
      up ? CONTENT_SETTING_DEFAULT : CONTENT_SETTING_BLOCK);
}

bool IsAggressiveBlocking(const HostContentSettingsMap& map,
                          const GURL& top_url) {
  return IsWebUrl(top_url) &&
         map.GetContentSetting(top_url, GURL(),
                               ContentSettingsType::PANTERA_FIRST_PARTY_ADS) ==
             CONTENT_SETTING_BLOCK;
}

void SetAggressiveBlocking(HostContentSettingsMap& map,
                           const GURL& top_url,
                           bool aggressive) {
  if (!IsWebUrl(top_url)) {
    return;
  }
  map.SetContentSettingCustomScope(
      SitePattern(top_url), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::PANTERA_FIRST_PARTY_ADS,
      aggressive ? CONTENT_SETTING_BLOCK : CONTENT_SETTING_DEFAULT);
}

bool IsForcedAggressive(const url::Origin& request_initiator) {
  return SameDomainOrHost(request_initiator,
                          url::Origin::CreateFromNormalizedTuple(
                              url::kHttpsScheme, "youtube.com", 443),
                          INCLUDE_PRIVATE_REGISTRIES);
}

}  // namespace pantera
