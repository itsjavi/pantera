// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PANTERA_COMPONENTS_ADBLOCK_FILTER_LIST_CATALOG_H_
#define PANTERA_COMPONENTS_ADBLOCK_FILTER_LIST_CATALOG_H_

#include <stdint.h>

#include <string>
#include <string_view>
#include <vector>

#include "url/gurl.h"

namespace pantera {

// One file of a filter list, downloaded from its maintainer.
struct FilterListSource {
  // Lists by Brave may use Brave's own +js(brave-…) scriptlets; Brave's
  // packager strips them from the others, by title.
  bool IsBraveList() const;

  GURL url;
  // The source's title, or its catalog entry's.
  std::string title;
};

// An entry of Brave's filter list catalog (brave/adblock-resources,
// filter_lists/list_catalog.json), bundled with Pantera (DECISION-15).
struct FilterListCatalogEntry {
  FilterListCatalogEntry();
  FilterListCatalogEntry(const FilterListCatalogEntry&);
  FilterListCatalogEntry(FilterListCatalogEntry&&);
  FilterListCatalogEntry& operator=(const FilterListCatalogEntry&);
  FilterListCatalogEntry& operator=(FilterListCatalogEntry&&);
  ~FilterListCatalogEntry();

  // Whether the list is for the browser's language, such as "fr" or "pt-BR".
  bool MatchesLocale(std::string_view locale) const;
  // Whether the list applies to macOS: Brave marks mobile-only lists.
  bool SupportsMacOS() const;

  std::string uuid;
  std::string title;
  std::string description;
  // Language codes of a regional list; empty for the others.
  std::vector<std::string> languages;
  bool hidden = false;
  bool default_enabled = false;
  // In standard mode, these lists don't apply to first-party content
  // (DECISION-16). They go to the default engine.
  bool first_party_protections = false;
  // Scriptlet permissions: bit 0 lets a list use uBlock Origin's trusted
  // scriptlets.
  uint8_t permission_mask = 0;
  std::vector<std::string> platforms;
  std::vector<FilterListSource> sources;
};

// Parses the catalog's JSON. Entries without a uuid or any valid https
// source are left out.
std::vector<FilterListCatalogEntry> ParseFilterListCatalog(
    std::string_view json);

}  // namespace pantera

#endif  // PANTERA_COMPONENTS_ADBLOCK_FILTER_LIST_CATALOG_H_
