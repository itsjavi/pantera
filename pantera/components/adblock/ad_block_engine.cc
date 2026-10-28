// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Ported to Pantera from brave-core (brave/brave-core@2f490708,
// components/brave_shields/content/browser/ad_block_engine.cc).
// Pantera's changes: see ad_block_engine.h.

#include "pantera/components/adblock/ad_block_engine.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/to_vector.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

using net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES;

namespace pantera {

AdBlockEngine::AdBlockEngine(bool is_default_engine)
    : ad_block_client_(adblock::new_engine()),
      is_default_engine_(is_default_engine) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

AdBlockEngine::~AdBlockEngine() = default;

adblock::BlockerResult AdBlockEngine::ShouldStartRequest(
    const GURL& url,
    std::string_view request_type,
    const url::Origin& request_initiator,
    const std::string& method,
    bool previously_matched_rule,
    bool previously_matched_exception,
    bool previously_matched_important) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Determine third-party here so the library doesn't need to figure it out.
  bool is_third_party =
      !SameDomainOrHost(url, request_initiator, INCLUDE_PRIVATE_REGISTRIES);
  const auto initiator_hostname = request_initiator.host();
  return ad_block_client_->matches(
      url.spec(), std::string(url.host()), initiator_hostname,
      std::string(request_type), is_third_party, method,
      // Checking normal rules is skipped if a normal rule or exception rule was
      // found previously
      previously_matched_rule || previously_matched_exception,
      // Always check exceptions unless one was found previously
      !previously_matched_exception);
}

std::optional<std::string> AdBlockEngine::GetCspDirectives(
    const GURL& url,
    std::string_view request_type,
    const url::Origin& first_party_origin,
    const std::string& method) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Determine third-party here so the library doesn't need to figure it out.
  bool is_third_party =
      !SameDomainOrHost(url, first_party_origin, INCLUDE_PRIVATE_REGISTRIES);

  auto result = ad_block_client_->get_csp_directives(
      url.spec(), std::string(url.host()), first_party_origin.host(),
      std::string(request_type), is_third_party, method);

  if (result.empty()) {
    return std::nullopt;
  }
  return std::string(result);
}

void AdBlockEngine::UseResources(const adblock::ResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ad_block_client_->use_resource_storage(storage);
}

base::DictValue AdBlockEngine::UrlCosmeticResources(const std::string& url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto result = ad_block_client_->url_cosmetic_resources(url);

  std::optional<base::DictValue> parsed_result = base::JSONReader::ReadDict(
      std::string_view(result.data(), result.size()), base::JSON_PARSE_RFC);
  if (!parsed_result) {
    return base::DictValue();
  }
  return std::move(parsed_result).value();
}

base::ListValue AdBlockEngine::HiddenClassIdSelectors(
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& exceptions) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto result =
      ad_block_client_->hidden_class_id_selectors(classes, ids, exceptions);
  if (result.result_kind != adblock::ResultKind::Success) {
    LOG(ERROR) << "AdBlockEngine::HiddenClassIdSelectors failed: "
               << result.error_message.c_str();
    return base::ListValue();
  }

  base::ListValue list_result;
  for (const auto& selector : result.value) {
    list_result.Append(std::string(selector));
  }

  return list_result;
}

bool AdBlockEngine::Load(rust::Box<adblock::FilterSet> filter_set,
                         const adblock::ResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto result = adblock::engine_from_filter_set(std::move(filter_set));
  if (result.result_kind != adblock::ResultKind::Success) {
    LOG(ERROR) << "AdBlockEngine::Load failed (default engine: "
               << is_default_engine_ << "): " << result.error_message.c_str();
    return false;
  }
  UpdateAdBlockClient(std::move(result.value), storage);
  return true;
}

bool AdBlockEngine::LoadSerialized(const std::vector<uint8_t>& serialized,
                                   const adblock::ResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // An empty buffer will not load successfully.
  if (serialized.empty()) {
    return false;
  }

  auto client = adblock::new_engine();
  if (!client->deserialize(serialized)) {
    LOG(ERROR) << "AdBlockEngine::LoadSerialized failed (default engine: "
               << is_default_engine_ << ")";
    return false;
  }

  UpdateAdBlockClient(std::move(client), storage);
  return true;
}

std::vector<uint8_t> AdBlockEngine::Serialize() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return base::ToVector(ad_block_client_->serialize());
}

void AdBlockEngine::UpdateAdBlockClient(
    rust::Box<adblock::Engine> ad_block_client,
    const adblock::ResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ad_block_client_ = std::move(ad_block_client);
  UseResources(storage);
}

}  // namespace pantera
