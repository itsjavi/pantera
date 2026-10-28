/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Ported to Pantera from brave-core (brave/brave-core@2f490708,
 * components/brave_shields/core/common/adblock/rs/src/resource_storage.rs).
 * Pantera's changes: renamed BraveCoreResourceStorage to ResourceStorage. */

use adblock::resources::{InMemoryResourceStorage, Resource, ResourceImpl, ResourceStorageBackend};
use cxx::CxxString;
use std::sync::Arc;

/// A wrapper around the inner storage to share ownership of the inner storage.
#[derive(Clone)]
pub struct ResourceStorage {
    shared_storage: Arc<InMemoryResourceStorage>,
}

impl ResourceStorageBackend for ResourceStorage {
    fn get_resource(&self, resource_ident: &str) -> Option<ResourceImpl> {
        self.shared_storage.get_resource(resource_ident)
    }
}

/// Creates a new ResourceStorage from JSON string.
pub fn new_resource_storage(resources_json: &CxxString) -> Box<ResourceStorage> {
    let resources = serde_json::from_str::<Vec<Resource>>(resources_json.to_str().unwrap_or("[]"))
        .unwrap_or_default();
    let in_memory_storage = InMemoryResourceStorage::from_resources(resources);
    let shared_storage = Arc::new(in_memory_storage);
    Box::new(ResourceStorage { shared_storage })
}

/// Creates a new empty ResourceStorage.
pub fn new_empty_resource_storage() -> Box<ResourceStorage> {
    let in_memory_storage = InMemoryResourceStorage::from_resources(vec![]);
    let shared_storage = Arc::new(in_memory_storage);
    Box::new(ResourceStorage { shared_storage })
}

/// Clones a ResourceStorage.
/// Clones only the Arc-based wrapper, the inner storage remains shared.
pub fn clone_resource_storage(storage: &ResourceStorage) -> Box<ResourceStorage> {
    Box::new(storage.clone())
}

/// Clones and extends a storage with additional resources.
pub fn extend_resource_storage(
    storage: &ResourceStorage,
    additional_resources_json: &str,
) -> Box<ResourceStorage> {
    if let Ok(additional_resources) =
        serde_json::from_str::<Vec<Resource>>(additional_resources_json)
    {
        if !additional_resources.is_empty() {
            let mut inner_storage = storage.shared_storage.as_ref().clone();

            for resource in additional_resources {
                let _ = inner_storage.add_resource(resource);
            }
            return Box::new(ResourceStorage { shared_storage: Arc::new(inner_storage) });
        }
    }
    clone_resource_storage(storage)
}

pub fn has_resource_for_testing(storage: &ResourceStorage, name: &CxxString) -> bool {
    storage.get_resource(name.to_str().unwrap_or("")).is_some()
}
