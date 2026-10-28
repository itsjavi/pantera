// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Ported to Pantera from brave-core (brave/brave-core@2f490708,
// components/brave_shields/content/browser/ad_block_engine_wrapper.cc).
// Pantera's changes: see ad_block_engine_wrapper.h. StripProceduralFilters
// also no longer loops forever on a non-string entry or dereferences a
// missing selector list.

#include "pantera/components/adblock/ad_block_engine_wrapper.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/debug/leak_annotations.h"
#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "pantera/components/adblock/ad_block_engine.h"

namespace pantera {

namespace {

// Brave's MergeCspDirectiveInto (core/browser/ad_block_service_helper.cc).
void MergeCspDirectiveInto(std::optional<std::string> from,
                           std::optional<std::string>* into) {
  if (!from) {
    return;
  }
  if (!*into) {
    *into = std::move(from);
    return;
  }
  *into = base::StrCat({*from, ", ", **into});
}

}  // namespace

AdBlockEngineWrapper::AdBlockEngineWrapper(
    std::unique_ptr<AdBlockEngine> default_engine,
    std::unique_ptr<AdBlockEngine> additional_engine)
    : default_engine_(std::move(default_engine)),
      additional_filters_engine_(std::move(additional_engine)) {
  // `this` is stored using SequenceBound, so false-positive shutdown leaks
  // are expected
  ANNOTATE_LEAKING_OBJECT_PTR(this);
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

AdBlockEngineWrapper::~AdBlockEngineWrapper() = default;

// static
std::unique_ptr<AdBlockEngineWrapper> AdBlockEngineWrapper::Create() {
  return std::make_unique<AdBlockEngineWrapper>(
      std::make_unique<AdBlockEngine>(true /* is_default */),
      std::make_unique<AdBlockEngine>(false /* is_default */));
}

adblock::BlockerResult AdBlockEngineWrapper::ShouldStartRequest(
    const GURL& url,
    std::string_view request_type,
    const url::Origin& request_initiator,
    const std::string& method,
    bool aggressive_blocking,
    bool previously_matched_rule,
    bool previously_matched_exception,
    bool previously_matched_important) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  adblock::BlockerResult fp_result = default_engine_->ShouldStartRequest(
      url, request_type, request_initiator, method, previously_matched_rule,
      previously_matched_exception, previously_matched_important);

  // removeparam results from the default engine are always ignored
  fp_result.rewritten_url.has_value = false;

  if (aggressive_blocking ||
      !SameDomainOrHost(
          url, request_initiator,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    if (fp_result.important) {
      return fp_result;
    }
  } else {
    // if there's an exception from the default engine, it still needs to be
    // considered by the additional engine
    fp_result = {
        .has_exception = fp_result.has_exception,
        .exception = std::move(fp_result.exception),
    };
  }

  GURL request_url = fp_result.rewritten_url.has_value
                         ? GURL(std::string(fp_result.rewritten_url.value))
                         : url;
  auto result = additional_filters_engine_->ShouldStartRequest(
      request_url, request_type, request_initiator, method,
      previously_matched_rule | fp_result.matched,
      previously_matched_exception | fp_result.has_exception,
      previously_matched_important | fp_result.important);

  result.matched |= fp_result.matched;
  result.has_exception |= fp_result.has_exception;
  result.important |= fp_result.important;
  if (!result.filter) {
    result.filter = std::move(fp_result.filter);
  }
  if (!result.exception) {
    result.exception = std::move(fp_result.exception);
  }
  if (!result.redirect.has_value && fp_result.redirect.has_value) {
    result.redirect = fp_result.redirect;
  }
  if (!result.rewritten_url.has_value && fp_result.rewritten_url.has_value) {
    result.rewritten_url = fp_result.rewritten_url;
  }
  return result;
}

std::optional<std::string> AdBlockEngineWrapper::GetCspDirectives(
    const GURL& url,
    std::string_view request_type,
    const url::Origin& first_party_origin,
    const std::string& method) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto csp_directives = default_engine_->GetCspDirectives(
      url, request_type, first_party_origin, method);

  const auto additional_csp = additional_filters_engine_->GetCspDirectives(
      url, request_type, first_party_origin, method);
  MergeCspDirectiveInto(additional_csp, &csp_directives);

  return csp_directives;
}

bool AdBlockEngineWrapper::Load(bool is_default_engine,
                                rust::Box<adblock::FilterSet> filter_set,
                                const adblock::ResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return engine(is_default_engine).Load(std::move(filter_set), storage);
}

bool AdBlockEngineWrapper::LoadSerialized(
    bool is_default_engine,
    const std::vector<uint8_t>& serialized,
    const adblock::ResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return engine(is_default_engine).LoadSerialized(serialized, storage);
}

std::vector<uint8_t> AdBlockEngineWrapper::Serialize(bool is_default_engine) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return engine(is_default_engine).Serialize();
}

void AdBlockEngineWrapper::UseResources(
    const adblock::ResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  default_engine_->UseResources(storage);
  additional_filters_engine_->UseResources(storage);
}

base::DictValue AdBlockEngineWrapper::UrlCosmeticResources(
    const std::string& url,
    bool aggressive_blocking) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::DictValue resources = default_engine_->UrlCosmeticResources(url);

  if (!aggressive_blocking) {
    // `:has` procedural selectors from the default engine should not be hidden
    // in standard blocking mode.
    base::ListValue* default_hide_selectors =
        resources.FindList("hide_selectors");
    if (default_hide_selectors) {
      default_hide_selectors->EraseIf([](const base::Value& selector) {
        return selector.is_string() &&
               selector.GetString().find(":has(") != std::string::npos;
      });
    }

    // In standard blocking mode, drop procedural filters but otherwise keep
    // action filters.
    StripProceduralFilters(resources);
  }

  base::DictValue additional_resources =
      additional_filters_engine_->UrlCosmeticResources(url);

  MergeResourcesInto(std::move(additional_resources), resources,
                     /*force_hide=*/true);

  return resources;
}

base::DictValue AdBlockEngineWrapper::HiddenClassIdSelectors(
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& exceptions) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::ListValue hide_selectors =
      default_engine_->HiddenClassIdSelectors(classes, ids, exceptions);

  base::ListValue force_hide_selectors =
      additional_filters_engine_->HiddenClassIdSelectors(classes, ids,
                                                         exceptions);

  base::DictValue result;
  result.Set("hide_selectors", std::move(hide_selectors));
  result.Set("force_hide_selectors", std::move(force_hide_selectors));
  return result;
}

// Removes any procedural filters from the given UrlCosmeticResources Value.
//
// Procedural filters are filters with at least one selector operator of a type
// that isn't `css-selector`.
//
// These filters are represented as JSON provided by adblock-rust. The format
// is documented at:
// https://docs.rs/adblock/latest/adblock/cosmetic_filter_cache/struct.ProceduralOrActionFilter.html
// static
void AdBlockEngineWrapper::StripProceduralFilters(base::DictValue& resources) {
  base::ListValue* procedural_actions =
      resources.FindList(kCosmeticResourcesProceduralActions);
  if (!procedural_actions) {
    return;
  }
  procedural_actions->EraseIf([](const base::Value& filter) {
    const std::string* filter_json = filter.GetIfString();
    if (!filter_json) {
      return false;
    }
    std::optional<base::DictValue> parsed =
        base::JSONReader::ReadDict(*filter_json, base::JSON_PARSE_RFC);
    if (!parsed) {
      return false;
    }
    const base::ListValue* selector = parsed->FindList("selector");
    // Non-procedural filters are always a single operator in length, and
    // that operator must be a `css-selector`.
    if (!selector || selector->size() != 1) {
      return true;
    }
    const base::DictValue* op = selector->front().GetIfDict();
    const std::string* type = op ? op->FindString("type") : nullptr;
    return type && *type != "css-selector";
  });
}

// Merges the contents of the first UrlCosmeticResources Value into the second
// one provided.
//
// If `force_hide` is true, the contents of `from`'s `hide_selectors` field
// will be moved into a possibly new field of `into` called
// `force_hide_selectors`.
void AdBlockEngineWrapper::MergeResourcesInto(base::DictValue from,
                                              base::DictValue& into,
                                              bool force_hide) {
  base::ListValue* resources_hide_selectors = nullptr;
  if (force_hide) {
    resources_hide_selectors = into.FindList("force_hide_selectors");
    if (!resources_hide_selectors) {
      resources_hide_selectors =
          into.Set("force_hide_selectors", base::ListValue())->GetIfList();
    }
  } else {
    resources_hide_selectors = into.FindList("hide_selectors");
  }
  base::ListValue* from_resources_hide_selectors =
      from.FindList("hide_selectors");
  if (resources_hide_selectors && from_resources_hide_selectors) {
    for (auto& selector : *from_resources_hide_selectors) {
      resources_hide_selectors->Append(std::move(selector));
    }
  }

  constexpr std::string_view kListKeys[] = {
      "exceptions", kCosmeticResourcesProceduralActions};
  for (const auto& key : kListKeys) {
    base::ListValue* resources = into.FindList(key);
    base::ListValue* from_resources = from.FindList(key);
    if (resources && from_resources) {
      for (auto& exception : *from_resources) {
        resources->Append(std::move(exception));
      }
    }
  }

  auto* resources_injected_script = into.FindString("injected_script");
  auto* from_resources_injected_script = from.FindString("injected_script");
  if (resources_injected_script && from_resources_injected_script) {
    *resources_injected_script =
        base::StrCat({"{\n", *resources_injected_script, "\n}\n{\n",
                      *from_resources_injected_script, "\n}"});
  }

  auto from_resources_generichide = from.FindBool("generichide");
  if (from_resources_generichide && *from_resources_generichide) {
    into.Set("generichide", true);
  }
}

AdBlockEngine& AdBlockEngineWrapper::engine(bool is_default_engine) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return is_default_engine ? *default_engine_ : *additional_filters_engine_;
}

}  // namespace pantera
