/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Ported to Pantera from brave-core (brave/brave-core@2f490708,
 * components/cosmetic_filters/browser/cosmetic_filters_resources.h).
 * Pantera's changes: it checks the site's shields settings and decides
 * whether to hide first-party content (DECISION-20), and procedural
 * filtering is always on. */

#ifndef PANTERA_COMPONENTS_COSMETIC_FILTERS_BROWSER_COSMETIC_FILTERS_RESOURCES_H_
#define PANTERA_COMPONENTS_COSMETIC_FILTERS_BROWSER_COSMETIC_FILTERS_RESOURCES_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "pantera/components/cosmetic_filters/common/cosmetic_filters.mojom.h"
#include "url/gurl.h"

class HostContentSettingsMap;

namespace pantera {

class AdBlockEngineWrapper;

// CosmeticFiltersResources is a class that is responsible for interaction
// between CosmeticFiltersJSHandler class that lives inside renderer process.
// It lives on `engine_wrapper`'s sequence, and reads the shields settings
// from `settings_map` there.
class CosmeticFiltersResources final : public mojom::CosmeticFiltersResources {
 public:
  CosmeticFiltersResources(const CosmeticFiltersResources&) = delete;
  CosmeticFiltersResources& operator=(const CosmeticFiltersResources&) = delete;
  CosmeticFiltersResources(AdBlockEngineWrapper* engine_wrapper,
                           scoped_refptr<HostContentSettingsMap> settings_map);
  ~CosmeticFiltersResources() override;

  // Sends back to renderer a response about rules that has to be applied
  // for the specified selectors.
  void HiddenClassIdSelectors(const std::string& input,
                              const std::vector<std::string>& exceptions,
                              HiddenClassIdSelectorsCallback callback) override;

  // Sends the renderer a response including whether or not to apply cosmetic
  // filtering to first party elements along with an initial set of rules and
  // scripts to apply for the given URL, or nothing when shields are down.
  void UrlCosmeticResources(const GURL& url,
                            const GURL& top_frame_url,
                            bool cross_origin_to_top_frame,
                            UrlCosmeticResourcesCallback callback) override;

 private:
  // The ad-block service owns this and the engines, and destroys this first.
  const raw_ptr<AdBlockEngineWrapper> engine_wrapper_;
  const scoped_refptr<HostContentSettingsMap> settings_map_;
};

}  // namespace pantera

#endif  // PANTERA_COMPONENTS_COSMETIC_FILTERS_BROWSER_COSMETIC_FILTERS_RESOURCES_H_
