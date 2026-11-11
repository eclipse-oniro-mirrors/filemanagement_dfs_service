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

//! Sync-folder persistence and change callbacks, with CloudDiskServiceSyncFolder entry points.

mod cache;
mod callback_manager;
pub(crate) mod file_state;
mod history;
mod log_file;
mod metafile;

pub(crate) use callback_manager::CloudDiskServiceCallbackManager;
pub(crate) use log_file::{CloudDiskServiceLogFile, LogFileMgr};

use crate::common::types::ChangesResult;

pub(crate) struct CloudDiskServiceSyncFolder;

impl CloudDiskServiceSyncFolder {
    pub(crate) fn register_sync_folder(user_id: i32, index: u32, path: &str) -> i32 {
        LogFileMgr::get_instance().register_sync_folder(user_id, index, path)
    }

    pub(crate) fn unregister_sync_folder(user_id: i32, index: u32) -> i32 {
        LogFileMgr::get_instance().unregister_sync_folder(user_id, index)
    }

    pub(crate) fn register_sync_folder_changes(user_id: i32, index: u32) {
        LogFileMgr::get_instance().register_sync_folder_changes(user_id, index);
    }

    pub(crate) fn unregister_sync_folder_changes(user_id: i32, index: u32) {
        LogFileMgr::get_instance().unregister_sync_folder_changes(user_id, index);
    }

    pub(crate) fn get_sync_folder_changes(
        user_id: i32,
        index: u32,
        start: u64,
        count: u64,
        result: &mut ChangesResult,
    ) -> i32 {
        LogFileMgr::get_instance().parse_request(user_id, index, start, count, result)
    }

    pub(crate) fn cloud_disk_service_clear_all() {
        info!("[CloudDiskServiceClearAll] clearing cached state");
        // Rust metadata handles are scoped to cache operations; there is no
        // separate MetaFileMgr cache to clear.
        LogFileMgr::get_instance().stop();
    }
}
