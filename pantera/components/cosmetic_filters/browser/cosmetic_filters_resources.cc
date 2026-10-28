/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Ported to Pantera from brave-core (brave/brave-core@2f490708,
 * components/cosmetic_filters/browser/cosmetic_filters_resources.cc).
 * Pantera's changes: it checks the site's shields settings and decides
 * whether to hide first-party content (DECISION-20), and procedural
 * filtering is always on. */

#include "pantera/components/cosmetic_filters/browser/cosmetic_filters_resources.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/string_escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "pantera/components/adblock/ad_block_engine_wrapper.h"
#include "pantera/components/shields/shields_settings.h"
#include "url/origin.h"

namespace {

constexpr char kProceduralActionsScript[] =
    R"((function() {
          const CC = window.content_cosmetic;
          let stylesheet = '';
          const takeStyleFilter = filter => {
            if (filter.selector.length === 1 && filter.selector[0].type === 'css-selector' && filter.action && filter.action.type === 'style') {
              stylesheet += filter.selector[0].arg + '{' + filter.action.arg + '}\n';
              return false;
            }
            return $1;
          };
          CC.proceduralActionFilters = JSON.parse($2).filter(f => takeStyleFilter(f));
          CC.hasProceduralActions = CC.proceduralActionFilters.length > 0;
          return stylesheet;
        })();)";

}  // namespace

namespace pantera {

CosmeticFiltersResources::CosmeticFiltersResources(
    AdBlockEngineWrapper* engine_wrapper,
    scoped_refptr<HostContentSettingsMap> settings_map)
    : engine_wrapper_(engine_wrapper), settings_map_(std::move(settings_map)) {}

CosmeticFiltersResources::~CosmeticFiltersResources() = default;

void CosmeticFiltersResources::HiddenClassIdSelectors(
    const std::string& input,
    const std::vector<std::string>& exceptions,
    HiddenClassIdSelectorsCallback callback) {
  std::optional<base::DictValue> input_dict = base::JSONReader::ReadDict(
      input, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                 base::JSON_REPLACE_INVALID_CHARACTERS);

  if (!input_dict) {
    // Nothing to work with
    std::move(callback).Run(base::DictValue());
    return;
  }

  std::vector<std::string> classes;
  base::ListValue* classes_list = input_dict->FindList("classes");
  if (classes_list) {
    for (const auto& class_item : *classes_list) {
      if (!class_item.is_string()) {
        continue;
      }
      classes.push_back(class_item.GetString());
    }
  }
  std::vector<std::string> ids;
  base::ListValue* ids_list = input_dict->FindList("ids");
  if (ids_list) {
    for (const auto& id_item : *ids_list) {
      if (!id_item.is_string()) {
        continue;
      }
      ids.push_back(id_item.GetString());
    }
  }

  auto selectors =
      engine_wrapper_->HiddenClassIdSelectors(classes, ids, exceptions);

  std::move(callback).Run(std::move(selectors));
}

void CosmeticFiltersResources::UrlCosmeticResources(
    const GURL& url,
    const GURL& top_frame_url,
    bool cross_origin_to_top_frame,
    UrlCosmeticResourcesCallback callback) {
  if (!url.SchemeIsHTTPOrHTTPS() ||
      !AreShieldsUp(*settings_map_, top_frame_url)) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  // As brave-core's renderer decides: aggressive mode, frames from another
  // site and YouTube hide first-party content too.
  const bool hide_first_party =
      cross_origin_to_top_frame ||
      IsAggressiveBlocking(*settings_map_, top_frame_url) ||
      IsForcedAggressive(url::Origin::Create(url));

  auto resources =
      engine_wrapper_->UrlCosmeticResources(url.spec(), hide_first_party);

  const auto* procedural_actions_list =
      resources.FindList(kCosmeticResourcesProceduralActions);
  if (procedural_actions_list && !procedural_actions_list->empty()) {
    // Each element of procedural_actions_list is already formatted as JSON.
    // Combine them into a single JSON list using string concatenation to avoid
    // double-escaping.
    auto procedural_actions_strings = std::vector<std::string>();
    std::transform(procedural_actions_list->cbegin(),
                   procedural_actions_list->cend(),
                   std::back_inserter(procedural_actions_strings),
                   [](const base::Value& action) -> std::string {
                     return action.GetString();
                   });
    std::string procedural_actions_json = base::StrCat(
        {"[", base::JoinString(procedural_actions_strings, ","), "]"});
    std::string escaped_procedural_actions;
    base::EscapeJSONString(procedural_actions_json, true,
                           &escaped_procedural_actions);
    std::string procedural_actions_script = base::ReplaceStringPlaceholders(
        kProceduralActionsScript, {"true", escaped_procedural_actions},
        nullptr);
    resources.Set("procedural_actions_script", procedural_actions_script);
  }
  resources.Remove(kCosmeticResourcesProceduralActions);
  resources.Set("hide_first_party", hide_first_party);

  std::move(callback).Run(std::move(resources));
}

}  // namespace pantera
