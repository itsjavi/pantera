// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/components/shields/shields_settings.h"

#include <string>

#include "base/strings/strcat.h"
#include "components/content_settings/core/browser/cookie_settings.h"
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

bool AreScriptsBlocked(const HostContentSettingsMap& map, const GURL& top_url) {
  return IsWebUrl(top_url) &&
         map.GetContentSetting(top_url, top_url,
                               ContentSettingsType::JAVASCRIPT) ==
             CONTENT_SETTING_BLOCK;
}

void SetScriptsBlocked(HostContentSettingsMap& map,
                       const GURL& top_url,
                       bool blocked) {
  if (!IsWebUrl(top_url)) {
    return;
  }
  ContentSetting setting = CONTENT_SETTING_BLOCK;
  if (!blocked) {
    // Only an exception allows scripts where they're blocked by default.
    setting = map.GetDefaultContentSetting(ContentSettingsType::JAVASCRIPT,
                                           nullptr) == CONTENT_SETTING_BLOCK
                  ? CONTENT_SETTING_ALLOW
                  : CONTENT_SETTING_DEFAULT;
  }
  map.SetContentSettingCustomScope(SitePattern(top_url),
                                   ContentSettingsPattern::Wildcard(),
                                   ContentSettingsType::JAVASCRIPT, setting);
}

CookieMode GetCookieMode(const HostContentSettingsMap& map,
                         const content_settings::CookieSettings& cookies,
                         const GURL& top_url) {
  if (map.GetContentSetting(top_url, top_url, ContentSettingsType::COOKIES) ==
      CONTENT_SETTING_BLOCK) {
    return CookieMode::kBlockAll;
  }
  return cookies.IsThirdPartyAccessAllowed(top_url)
             ? CookieMode::kAllowAll
             : CookieMode::kBlockThirdParty;
}

void SetCookieMode(const HostContentSettingsMap& map,
                   content_settings::CookieSettings& cookies,
                   const GURL& top_url,
                   CookieMode mode) {
  if (!IsWebUrl(top_url)) {
    return;
  }
  cookies.ResetThirdPartyCookieSetting(top_url);
  if (mode == CookieMode::kBlockAll) {
    cookies.SetCookieSetting(top_url, CONTENT_SETTING_BLOCK);
    return;
  }

  // The site's own cookies: allowed, with an exception if all cookies are
  // blocked by default.
  if (map.GetDefaultContentSetting(ContentSettingsType::COOKIES, nullptr) ==
      CONTENT_SETTING_BLOCK) {
    cookies.SetCookieSetting(top_url, CONTENT_SETTING_ALLOW);
  } else {
    cookies.ResetCookieSetting(top_url);
  }

  // Third parties: an exception unless the default already does it.
  if (mode == CookieMode::kAllowAll) {
    cookies.SetThirdPartyCookieSetting(top_url, CONTENT_SETTING_ALLOW);
  } else if (!cookies.ShouldBlockThirdPartyCookies()) {
    cookies.SetThirdPartyCookieSetting(top_url, CONTENT_SETTING_BLOCK);
  }
}

}  // namespace pantera
