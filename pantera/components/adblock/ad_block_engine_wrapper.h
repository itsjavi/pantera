// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Ported to Pantera from brave-core (brave/brave-core@2f490708,
// components/brave_shields/content/browser/ad_block_engine_wrapper.h).
// Pantera's changes: request types are strings, serialized engines are plain
// byte vectors, first-party requests are never blocked by the default engine
// in standard mode (Brave's kBraveAdblockDefault1pBlocking feature is off),
// and the trace events, debug info and regex discard policy are gone.

#ifndef PANTERA_COMPONENTS_ADBLOCK_AD_BLOCK_ENGINE_WRAPPER_H_
#define PANTERA_COMPONENTS_ADBLOCK_AD_BLOCK_ENGINE_WRAPPER_H_

#include <stdint.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/sequence_checker.h"
#include "base/values.h"
#include "pantera/components/adblock/rs/src/lib.rs.h"
#include "third_party/rust/cxx/v1/cxx.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace pantera {

class AdBlockEngine;

// Key for procedural and action filters in adblock-rust's cosmetic resources.
inline constexpr char kCosmeticResourcesProceduralActions[] =
    "procedural_actions";

// Brave's two engines: the default engine holds the lists with first-party
// protections, which standard mode doesn't apply to first-party content
// (DECISION-17); the additional engine holds every other list and always
// applies.
class AdBlockEngineWrapper {
 public:
  AdBlockEngineWrapper(std::unique_ptr<AdBlockEngine> default_engine,
                       std::unique_ptr<AdBlockEngine> additional_engine);
  AdBlockEngineWrapper(const AdBlockEngineWrapper&) = delete;
  AdBlockEngineWrapper& operator=(const AdBlockEngineWrapper&) = delete;
  ~AdBlockEngineWrapper();

  static std::unique_ptr<AdBlockEngineWrapper> Create();

  adblock::BlockerResult ShouldStartRequest(
      const GURL& url,
      std::string_view request_type,
      const url::Origin& request_initiator,
      const std::string& method,
      bool aggressive_blocking,
      bool previously_matched_rule,
      bool previously_matched_exception,
      bool previously_matched_important);
  std::optional<std::string> GetCspDirectives(
      const GURL& url,
      std::string_view request_type,
      const url::Origin& first_party_origin,
      const std::string& method);

  bool Load(bool is_default_engine,
            rust::Box<adblock::FilterSet> filter_set,
            const adblock::ResourceStorage& storage);
  bool LoadSerialized(bool is_default_engine,
                      const std::vector<uint8_t>& serialized,
                      const adblock::ResourceStorage& storage);
  std::vector<uint8_t> Serialize(bool is_default_engine);
  void UseResources(const adblock::ResourceStorage& storage);

  base::DictValue UrlCosmeticResources(const std::string& url,
                                       bool aggressive_blocking);
  base::DictValue HiddenClassIdSelectors(
      const std::vector<std::string>& classes,
      const std::vector<std::string>& ids,
      const std::vector<std::string>& exceptions);

  static void StripProceduralFilters(base::DictValue& resources);
  static void MergeResourcesInto(base::DictValue from,
                                 base::DictValue& into,
                                 bool force_hide);

 private:
  AdBlockEngine& engine(bool is_default_engine);

  const std::unique_ptr<AdBlockEngine> default_engine_
      GUARDED_BY_CONTEXT(sequence_checker_);
  const std::unique_ptr<AdBlockEngine> additional_filters_engine_
      GUARDED_BY_CONTEXT(sequence_checker_);

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace pantera

#endif  // PANTERA_COMPONENTS_ADBLOCK_AD_BLOCK_ENGINE_WRAPPER_H_
