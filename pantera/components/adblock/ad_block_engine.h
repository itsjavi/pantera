// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Ported to Pantera from brave-core (brave/brave-core@2f490708,
// components/brave_shields/content/browser/ad_block_engine.h).
// Pantera's changes: request types are adblock-rust's strings rather than
// Blink's enum, serialized engines are plain byte vectors, and the histograms,
// trace events, debug info and regex discard policy are gone.

#ifndef PANTERA_COMPONENTS_ADBLOCK_AD_BLOCK_ENGINE_H_
#define PANTERA_COMPONENTS_ADBLOCK_AD_BLOCK_ENGINE_H_

#include <stdint.h>

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

// One adblock-rust engine. Lives on a single sequence.
class AdBlockEngine {
 public:
  explicit AdBlockEngine(bool is_default_engine);
  AdBlockEngine(const AdBlockEngine&) = delete;
  AdBlockEngine& operator=(const AdBlockEngine&) = delete;
  ~AdBlockEngine();

  // `request_type` is one of adblock-rust's request types, such as "script"
  // or "image".
  adblock::BlockerResult ShouldStartRequest(
      const GURL& url,
      std::string_view request_type,
      const url::Origin& request_initiator,
      const std::string& method,
      bool previously_matched_rule,
      bool previously_matched_exception,
      bool previously_matched_important);
  std::optional<std::string> GetCspDirectives(
      const GURL& url,
      std::string_view request_type,
      const url::Origin& first_party_origin,
      const std::string& method);
  void UseResources(const adblock::ResourceStorage& storage);

  base::DictValue UrlCosmeticResources(const std::string& url);
  base::ListValue HiddenClassIdSelectors(
      const std::vector<std::string>& classes,
      const std::vector<std::string>& ids,
      const std::vector<std::string>& exceptions);

  // Replaces the engine with one built from `filter_set`, or with a serialized
  // engine. Returns false, keeping the current engine, if that fails.
  bool Load(rust::Box<adblock::FilterSet> filter_set,
            const adblock::ResourceStorage& storage);
  bool LoadSerialized(const std::vector<uint8_t>& serialized,
                      const adblock::ResourceStorage& storage);

  // Empty if serialization fails.
  std::vector<uint8_t> Serialize();

 private:
  void UpdateAdBlockClient(rust::Box<adblock::Engine> ad_block_client,
                           const adblock::ResourceStorage& storage);

  rust::Box<adblock::Engine> ad_block_client_
      GUARDED_BY_CONTEXT(sequence_checker_);

  const bool is_default_engine_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace pantera

#endif  // PANTERA_COMPONENTS_ADBLOCK_AD_BLOCK_ENGINE_H_
