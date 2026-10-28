/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Ported to Pantera from brave-core (brave/brave-core@2f490708,
 * components/brave_shields/core/common/adblock/rs/src/result.rs).
 * Pantera's changes: InternalError implements Display and From by hand
 * instead of with thiserror, which Chromium doesn't offer to first-party
 * code; iOS result type removed. */

use std::str::Utf8Error;

use adblock::resources::AddResourceError;

use crate::engine::Engine;
use crate::ffi::*;

#[derive(Debug)]
pub(crate) enum InternalError {
    Json(serde_json::Error),
    Utf8(Utf8Error),
    AddResource(AddResourceError),
}

impl std::fmt::Display for InternalError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Json(e) => write!(f, "json error: {e}"),
            Self::Utf8(e) => write!(f, "utf-8 encoding error: {e}"),
            Self::AddResource(e) => write!(f, "{e}"),
        }
    }
}

impl From<serde_json::Error> for InternalError {
    fn from(e: serde_json::Error) -> Self {
        Self::Json(e)
    }
}

impl From<Utf8Error> for InternalError {
    fn from(e: Utf8Error) -> Self {
        Self::Utf8(e)
    }
}

impl From<AddResourceError> for InternalError {
    fn from(e: AddResourceError) -> Self {
        Self::AddResource(e)
    }
}

impl From<&InternalError> for ResultKind {
    fn from(error: &InternalError) -> Self {
        match error {
            InternalError::Json(_) => Self::JsonError,
            InternalError::Utf8(_) => Self::Utf8Error,
            InternalError::AddResource(_) => Self::AdblockError,
        }
    }
}

/// This is an intermediate "result" type.
///
/// The purpose of this is to streamline conversions of typical Rust results to
/// final cxx-compatible structures that are to be consumed by callers.
/// The PreResult is generic to make conversions from Rust results easy
/// (via implementation of the From<Result<T, InternalError>> trait). Since cxx
/// does not support generics, this PreResult to converted to a similar final
/// structure that is not generic.
///
/// The typical chain of conversion for "result" types is as follows:
/// Result<T, InternalError> to PreResult<T> to
/// TResult (i.e. StringResult defined in the ffi module).
struct PreResult<T: Default> {
    value: T,
    result_kind: ResultKind,
    error_message: String,
}

/// Implements the From<PreResult<T>> trait for a given final ffi result type.
macro_rules! impl_result_from_trait {
    ($result_type:ty, $value_type:ty) => {
        impl From<Result<$value_type, InternalError>> for $result_type {
            fn from(result: Result<$value_type, InternalError>) -> Self {
                let PreResult { value, result_kind, error_message } = PreResult::from(result);
                Self { value, result_kind, error_message }
            }
        }
    };
}

impl<T> From<Result<T, InternalError>> for PreResult<T>
where
    T: Default,
{
    fn from(result: Result<T, InternalError>) -> Self {
        match result {
            Ok(value) => {
                Self { value, result_kind: ResultKind::Success, error_message: String::new() }
            }
            Err(e) => Self {
                value: Default::default(),
                result_kind: (&e).into(),
                error_message: e.to_string(),
            },
        }
    }
}

impl_result_from_trait!(VecStringResult, Vec<String>);
impl_result_from_trait!(BoxEngineResult, Box<Engine>);
impl_result_from_trait!(AddFilterListResult, AddedFiltersRecord);
