// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/components/adblock/filter_list_catalog.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "url/url_constants.h"

namespace pantera {

namespace {

std::vector<std::string> ReadStrings(const base::DictValue& dict,
                                     std::string_view key) {
  std::vector<std::string> strings;
  if (const base::ListValue* list = dict.FindList(key)) {
    for (const base::Value& value : *list) {
      if (value.is_string()) {
        strings.push_back(value.GetString());
      }
    }
  }
  return strings;
}

std::string ReadString(const base::DictValue& dict, std::string_view key) {
  const std::string* value = dict.FindString(key);
  return value ? *value : std::string();
}

}  // namespace

FilterListCatalogEntry::FilterListCatalogEntry() = default;
FilterListCatalogEntry::FilterListCatalogEntry(const FilterListCatalogEntry&) =
    default;
FilterListCatalogEntry::FilterListCatalogEntry(FilterListCatalogEntry&&) =
    default;
FilterListCatalogEntry& FilterListCatalogEntry::operator=(
    const FilterListCatalogEntry&) = default;
FilterListCatalogEntry& FilterListCatalogEntry::operator=(
    FilterListCatalogEntry&&) = default;
FilterListCatalogEntry::~FilterListCatalogEntry() = default;

bool FilterListCatalogEntry::MatchesLocale(std::string_view locale) const {
  // Brave matches the language part of the locale: "pt-BR" -> "pt".
  const std::string language =
      base::ToLowerASCII(locale.substr(0, locale.find('-')));
  for (const std::string& list_language : languages) {
    if (base::ToLowerASCII(list_language) == language) {
      return true;
    }
  }
  return false;
}

bool FilterListCatalogEntry::SupportsMacOS() const {
  return platforms.empty() || std::ranges::contains(platforms, "MAC") ||
         std::ranges::contains(platforms, "DESKTOP");
}

bool FilterListSource::IsBraveList() const {
  return title.starts_with("Brave ") || title == "Experimental ad blocker";
}

std::vector<FilterListCatalogEntry> ParseFilterListCatalog(
    std::string_view json) {
  std::vector<FilterListCatalogEntry> catalog;
  std::optional<base::ListValue> entries =
      base::JSONReader::ReadList(json, base::JSON_PARSE_RFC);
  if (!entries) {
    return catalog;
  }

  for (const base::Value& value : *entries) {
    const base::DictValue* dict = value.GetIfDict();
    if (!dict) {
      continue;
    }
    FilterListCatalogEntry entry;
    entry.uuid = ReadString(*dict, "uuid");
    entry.title = ReadString(*dict, "title");
    entry.description = ReadString(*dict, "desc");
    entry.languages = ReadStrings(*dict, "langs");
    entry.hidden = dict->FindBool("hidden").value_or(false);
    entry.default_enabled = dict->FindBool("default_enabled").value_or(false);
    entry.first_party_protections =
        dict->FindBool("first_party_protections").value_or(false);
    const int permission_mask = dict->FindInt("permission_mask").value_or(0);
    if (permission_mask < 0 || permission_mask > 0xFF) {
      continue;
    }
    entry.permission_mask = static_cast<uint8_t>(permission_mask);
    entry.platforms = ReadStrings(*dict, "platforms");

    if (const base::ListValue* sources = dict->FindList("sources")) {
      for (const base::Value& source_value : *sources) {
        const base::DictValue* source = source_value.GetIfDict();
        if (!source) {
          continue;
        }
        GURL url(ReadString(*source, "url"));
        // Only standard-format lists over https: hosts files and other
        // formats aren't in the catalog.
        const std::string* format = source->FindString("format");
        if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme) ||
            (format && *format != "Standard")) {
          continue;
        }
        std::string title = ReadString(*source, "title");
        entry.sources.push_back(
            {std::move(url), title.empty() ? entry.title : std::move(title)});
      }
    }

    if (entry.uuid.empty() || entry.sources.empty()) {
      continue;
    }
    catalog.push_back(std::move(entry));
  }
  return catalog;
}

}  // namespace pantera
