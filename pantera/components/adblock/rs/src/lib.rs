/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Ported to Pantera from brave-core (brave/brave-core@2f490708,
 * components/brave_shields/core/common/adblock/rs/src/lib.rs).
 * Pantera's changes: include path, ResourceStorage renamed from
 * BraveCoreResourceStorage, iOS content-blocking conversion removed. */

//! This crate provides a cxx-based FFI for the
//! [adblock-rust](https://github.com/brave/adblock-rust) library.

mod convert;
mod engine;
mod filter_set;
mod resource_storage;
mod result;

use engine::*;
use filter_set::*;
use resource_storage::*;

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = adblock)]
mod ffi {
    extern "Rust" {
        type FilterSet;
        fn new_filter_set(debug: bool) -> Box<FilterSet>;
        fn add_filter_list(&mut self, rules: &CxxVector<u8>) -> AddFilterListResult;
        fn add_filter_list_with_permissions(
            &mut self,
            rules: &CxxVector<u8>,
            permission_mask: u8,
        ) -> AddFilterListResult;
    }
    extern "Rust" {
        type ResourceStorage;
        fn new_resource_storage(resources_json: &CxxString) -> Box<ResourceStorage>;
        fn new_empty_resource_storage() -> Box<ResourceStorage>;
        fn clone_resource_storage(storage: &ResourceStorage) -> Box<ResourceStorage>;
        fn extend_resource_storage(
            storage: &ResourceStorage,
            additional_resources_json: &str,
        ) -> Box<ResourceStorage>;
        fn has_resource_for_testing(storage: &ResourceStorage, name: &CxxString) -> bool;
    }
    extern "Rust" {
        type Engine;
        /// Creates a new engine with no rules.
        fn new_engine() -> Box<Engine>;
        /// Creates a new engine with rules from a given filter list.
        /// Deprecated: Use engine_from_filter_set instead.
        fn engine_with_rules(rules: &CxxVector<u8>) -> BoxEngineResult;
        /// Creates a new engine with rules from a given filter set.
        fn engine_from_filter_set(filter_set: Box<FilterSet>) -> BoxEngineResult;

        /// Configures the adblock domain resolver to the call
        /// `resolve_domain_position` implemented in adblock_domain_resolver.cc.
        fn set_domain_resolver() -> bool;

        /// Extracts the homepage and title from the metadata contained in a
        /// filter list.
        fn read_list_metadata(list: &CxxVector<u8>) -> FilterListMetadata;

        /// Checks if a given request should be blocked and returns an
        /// evaluation result struct with information on a matching rule
        /// and actions.
        fn matches(
            &self,
            url: &CxxString,
            hostname: &CxxString,
            initiator_hostname: &CxxString,
            request_type: &CxxString,
            third_party_request: bool,
            method: &CxxString,
            previously_matched_rule: bool,
            force_check_exceptions: bool,
        ) -> BlockerResult;
        /// Returns additional CSP directives to be added to a web response,
        /// if applicable to the request.
        fn get_csp_directives(
            &self,
            url: &CxxString,
            hostname: &CxxString,
            source_hostname: &CxxString,
            request_type: &CxxString,
            third_party_request: bool,
            method: &CxxString,
        ) -> String;
        pub fn serialize(&self) -> Vec<u8>;
        /// Deserializes and loads a binary-serialized Engine.
        fn deserialize(&mut self, serialized: &CxxVector<u8>) -> bool;
        /// Loads resources from a ResourceStorage instance into the engine.
        fn use_resource_storage(&mut self, storage: &ResourceStorage);
        /// Returns JSON-serialized cosmetic filter resources for a given url.
        fn url_cosmetic_resources(&self, url: &CxxString) -> String;

        /// Returns list of CSS selectors that require a generic CSS hide rule,
        /// from a given set of classes, ids and exceptions.
        fn hidden_class_id_selectors(
            &self,
            classes: &CxxVector<CxxString>,
            ids: &CxxVector<CxxString>,
            exceptions: &CxxVector<CxxString>,
        ) -> VecStringResult;
        /// Returns the blocker debug info containing regex info.
        fn get_debug_info(&self) -> DebugInfo;
        /// Removes a regex entry by the id.
        fn discard_regex(&mut self, regex_id: u64);
        /// Sets a discard policy for the regex manager.
        fn set_regex_discard_policy(&mut self, new_discard_policy: &RegexManagerDiscardPolicy);
    }

    unsafe extern "C++" {
        include!("pantera/components/adblock/resolver/adblock_domain_resolver.h");

        /// Wrapper function for
        /// net::registry_controlled_domains::GetDomainAndRegistry
        /// implemented in Chromium.
        fn resolve_domain_position(host: &CxxString) -> DomainPosition;
    }

    struct DomainPosition {
        start: u32,
        end: u32,
    }

    struct BlockerResult {
        matched: bool,
        important: bool,
        has_exception: bool,
        redirect: OptionalString,
        rewritten_url: OptionalString,
        filter: UniquePtr<FilterRuleInfo>,
        exception: UniquePtr<FilterRuleInfo>,
    }

    // Represents `Option<adblock::sourcemap::FilterRuleDebugInfo>` since cxx
    // does not support optional fields in shared structs.
    #[derive(Default)]
    struct FilterRuleInfo {
        raw_line: String,
        source_index: i32, // -1 if not available
        line_number: i32,  // -1 if not available
    }

    struct RegexDebugEntry {
        id: u64,
        regex: OptionalString,
        unused_secs: u64,
        usage_count: usize,
    }

    struct SourceInfo {
        title: OptionalString,
        homepage: OptionalString,
        network_filter_count: usize,
        cosmetic_filter_count: usize,
        parse_error: usize,
        invalid_lines: Vec<String>,
    }

    struct DebugInfo {
        regex_data: Vec<RegexDebugEntry>,
        compiled_regex_count: usize,
        flatbuffer_size: usize,
        source_info: Vec<SourceInfo>,
    }

    struct RegexManagerDiscardPolicy {
        cleanup_interval_secs: u64,
        discard_unused_secs: u64,
    }

    #[derive(Default)]
    struct FilterListMetadata {
        homepage: OptionalString,
        title: OptionalString,
        expires_hours: OptionalU16,
    }

    #[derive(Default)]
    struct AddedFiltersRecord {
        source_index: usize,
        metadata: FilterListMetadata,
    }

    enum ResultKind {
        Success,
        JsonError,
        Utf8Error,
        AdblockError,
    }

    // The following structs are not generic because generic type parameters are
    // not yet supported in cxx.
    // Created custom Result structs because cxx auto converts Result<T> to
    // std::exception, and exceptions are not allowed in Chromium.

    struct VecStringResult {
        value: Vec<String>,
        result_kind: ResultKind,
        error_message: String,
    }

    struct BoxEngineResult {
        value: Box<Engine>,
        result_kind: ResultKind,
        error_message: String,
    }

    struct AddFilterListResult {
        value: AddedFiltersRecord,
        result_kind: ResultKind,
        error_message: String,
    }

    // Created custom Option struct because automatic conversion of Option<T>
    // is not yet supported in cxx.
    #[derive(Default)]
    struct OptionalString {
        has_value: bool,
        value: String,
    }

    #[derive(Default)]
    struct OptionalU16 {
        has_value: bool,
        value: u16,
    }
}
