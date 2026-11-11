/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! Sync-folder registry corresponding to CloudDiskSyncFolder.

use std::collections::HashMap;
use std::sync::{Mutex, OnceLock};

#[derive(Clone)]
pub(crate) struct SyncFolderValue {
    pub(crate) bundle_name: String,
    pub(crate) path: String,
}

pub(crate) struct CloudDiskSyncFolder {
    sync_folders: Mutex<HashMap<u32, SyncFolderValue>>,
}

impl CloudDiskSyncFolder {
    pub(crate) fn get_instance() -> &'static Self {
        static INSTANCE: OnceLock<CloudDiskSyncFolder> = OnceLock::new();
        INSTANCE.get_or_init(|| Self {
            sync_folders: Mutex::new(HashMap::new()),
        })
    }

    pub(crate) fn get_sync_folder_size(&self) -> usize {
        self.sync_folders
            .lock()
            .unwrap_or_else(|err| err.into_inner())
            .len()
    }

    pub(crate) fn add_sync_folder(&self, index: u32, value: SyncFolderValue) {
        info!(
            "[AddSyncFolder] index={}, bundle_name={}, path={}",
            @public(index), @private(&value.bundle_name), @private(&value.path),
        );
        let mut folders = self
            .sync_folders
            .lock()
            .unwrap_or_else(|err| err.into_inner());
        let previous_size = folders.len();
        folders.entry(index).or_insert(value);
        info!("[AddSyncFolder] completed, index={}, inserted={}, total_roots={}",
            @public(index), @public(folders.len() != previous_size), @public(folders.len()));
    }

    pub(crate) fn get_sync_folder_map(&self) -> HashMap<u32, SyncFolderValue> {
        self.sync_folders
            .lock()
            .unwrap_or_else(|err| err.into_inner())
            .clone()
    }

    pub(crate) fn delete_sync_folder(&self, index: u32) {
        info!("[DeleteSyncFolder] index={}", @public(index));
        self.sync_folders
            .lock()
            .unwrap_or_else(|err| err.into_inner())
            .remove(&index);
    }

    pub(crate) fn get_sync_folder_value_by_index(&self, index: u32) -> Option<SyncFolderValue> {
        self.sync_folders
            .lock()
            .unwrap_or_else(|err| err.into_inner())
            .get(&index)
            .cloned()
    }

    pub(crate) fn clear_map(&self) {
        self.sync_folders
            .lock()
            .unwrap_or_else(|err| err.into_inner())
            .clear();
    }
}
