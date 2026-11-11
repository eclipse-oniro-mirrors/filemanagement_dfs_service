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

//! Sync-folder registry, path conversion, dentry hashing and file cleanup.

mod cleanup;
mod hash;
mod path_utils;
mod sync_folder;

pub(crate) use cleanup::{
    remove_placeholder_files_batch, remove_xattr, CLOUD_DISK_FILE_SYNC_STATE_XATTR,
    CLOUD_DISK_PLACEHOLDER_COUNT_XATTR,
};
pub(crate) use hash::dentry_hash;
pub(crate) use path_utils::{
    path_to_mnt_path_by_physical_path, path_to_mnt_path_by_sandbox_path, path_to_physical_path,
    path_to_sandbox_path_by_physical_path,
};
pub(crate) use sync_folder::{CloudDiskSyncFolder, SyncFolderValue};
