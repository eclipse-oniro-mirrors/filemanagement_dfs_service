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

//! Business entry points corresponding to ipc/src/cloud_disk_service.cpp.
//!
//! Server framework scaffold: methods currently return synthetic success/output
//! values. They do not mutate files, persist registrations or invoke callbacks.
//! Replace these mock replies when implementing business logic.

use ipc::remote::RemoteObj;

use crate::common::error_codes::{E_INVALID_ARG, E_OK};
use crate::common::types::{
    CallbackExecuteRequest, ChangeData, ChangesResult, FailedList, FileSyncState,
    PlaceholderCustomInfo, PlaceholderInfo, ResultList,
};
use crate::framework::ICloudDiskService;

// SAFWK owns the registered value and the published stub owns a clone. Future
// mutable service state must be shared through Arc so both use the same data.
#[derive(Clone)]
pub(crate) struct CloudDiskService;

impl CloudDiskService {
    pub(crate) fn new() -> Self {
        Self
    }
}

impl ICloudDiskService for CloudDiskService {
    fn register_sync_folder_changes_inner(&self, sync_folder: &str, remote: &RemoteObj) -> i32 {
        info!(
            "[register_sync_folder_changes_inner] sync_folder={}, remote.is_proxy={}, remote.is_dead={}",
            @public(sync_folder),
            @public(remote.is_proxy()),
            @public(remote.is_dead()),
        );
        E_OK
    }

    fn unregister_sync_folder_changes_inner(&self, sync_folder: &str) -> i32 {
        info!(
            "[unregister_sync_folder_changes_inner] sync_folder={}",
            @public(sync_folder),
        );
        E_OK
    }

    fn register_callback_table_inner(&self, sync_folder: &str, remote: &RemoteObj) -> i32 {
        info!(
            "[register_callback_table_inner] sync_folder={}, remote.is_proxy={}, remote.is_dead={}",
            @public(sync_folder),
            @public(remote.is_proxy()),
            @public(remote.is_dead()),
        );
        E_OK
    }

    fn unregister_callback_table_inner(&self, sync_folder: &str) -> i32 {
        info!(
            "[unregister_callback_table_inner] sync_folder={}",
            @public(sync_folder),
        );
        E_OK
    }

    fn get_sync_folder_changes_inner(
        &self,
        sync_folder: &str,
        count: u64,
        start_usn: u64,
        result: &mut ChangesResult,
    ) -> i32 {
        info!(
            "[get_sync_folder_changes_inner] sync_folder={}, count={}, start_usn={}",
            @public(sync_folder),
            @public(count),
            @public(start_usn),
        );
        *result = ChangesResult {
            next_usn: start_usn,
            is_eof: false,
            changes_data: Vec::new(),
        };
        let length = count.min(2) as usize;
        let Some(next_usn) = start_usn.checked_add(length as u64) else {
            return E_INVALID_ARG;
        };
        for index in 0..length {
            let n = index as u64;
            result.changes_data.push(ChangeData {
                update_sequence_number: start_usn + n,
                file_id: format!("mock-file-{:06}", index + 1),
                parent_file_id: "mock-root-000000".to_string(),
                relative_path: format!("ipc_verify/模拟文件-{}.txt", index + 1),
                operation_type: if index == 0 { 0 } else { 4 },
                size: 4_294_967_297 + n,
                mtime: 1_700_000_000_123 + n,
                time_stamp: 1_700_000_000_456 + n,
            });
        }
        result.next_usn = next_usn;
        result.is_eof = count >= 2;
        E_OK
    }

    fn set_file_sync_states_inner(
        &self,
        sync_folder: &str,
        states: &[FileSyncState],
        result: &mut Vec<FailedList>,
    ) -> i32 {
        info!(
            "[set_file_sync_states_inner] sync_folder={}, states_count={}",
            @public(sync_folder),
            @public(states.len()),
        );
        for (index, item) in states.iter().enumerate() {
            info!(
                "[set_file_sync_states_inner] states[{}].path={}, state={}",
                @public(index),
                @public(&item.path),
                @public(item.state),
            );
        }
        *result = states
            .iter()
            .take(2)
            .enumerate()
            .map(|(index, item)| FailedList {
                path: item.path.clone(),
                error: if index == 0 { 1 } else { 3 },
            })
            .collect();
        E_OK
    }

    fn get_file_sync_states_inner(
        &self,
        sync_folder: &str,
        paths: &[String],
        result: &mut Vec<ResultList>,
    ) -> i32 {
        info!(
            "[get_file_sync_states_inner] sync_folder={}, paths_count={}",
            @public(sync_folder),
            @public(paths.len()),
        );
        for (index, path) in paths.iter().enumerate() {
            info!(
                "[get_file_sync_states_inner] paths[{}]={}",
                @public(index),
                @public(path),
            );
        }
        *result = paths
            .iter()
            .enumerate()
            .map(|(index, path)| ResultList {
                is_success: index % 2 == 0,
                state: if index % 2 == 0 { 2 } else { 0 },
                path: path.clone(),
                error: if index % 2 == 0 { 0 } else { 4 },
            })
            .collect();
        E_OK
    }

    fn create_placeholder_file_inner(
        &self,
        sync_folder: &str,
        relative_path: &str,
        info: &PlaceholderInfo,
        custom_info: &PlaceholderCustomInfo,
    ) -> i32 {
        info!(
            "[create_placeholder_file_inner] sync_folder={}, relative_path={}, logical_size={}, atime_ms={}, mtime_ms={}",
            @public(sync_folder),
            @public(relative_path),
            @public(info.logical_size),
            @public(info.atime_ms),
            @public(info.mtime_ms),
        );
        info!(
            "[create_placeholder_file_inner] custom_info.data_len={}, data={:02x?}",
            @public(custom_info.data.len()),
            @public(&custom_info.data),
        );
        E_OK
    }

    fn is_placeholder_file_inner(&self, sync_folder: &str, path: &str, result: &mut bool) -> i32 {
        info!(
            "[is_placeholder_file_inner] sync_folder={}, path={}",
            @public(sync_folder),
            @public(path),
        );
        *result = true;
        E_OK
    }

    fn register_sync_folder_inner(&self, user_id: i32, bundle_name: &str, path: &str) -> i32 {
        info!(
            "[register_sync_folder_inner] user_id={}, bundle_name={}, path={}",
            @public(user_id),
            @public(bundle_name),
            @private(path),
        );
        E_OK
    }

    fn unregister_sync_folder_inner(&self, user_id: i32, bundle_name: &str, path: &str) -> i32 {
        info!(
            "[unregister_sync_folder_inner] user_id={}, bundle_name={}, path={}",
            @public(user_id),
            @public(bundle_name),
            @private(path),
        );
        E_OK
    }

    fn unregister_for_sa_inner(&self, path: &str) -> i32 {
        info!(
            "[unregister_for_sa_inner] path={}",
            @public(path),
        );
        E_OK
    }

    fn convert_placeholder_to_file_inner(&self, sync_folder: &str, relative_path: &str) -> i32 {
        info!(
            "[convert_placeholder_to_file_inner] sync_folder={}, relative_path={}",
            @public(sync_folder),
            @public(relative_path),
        );
        E_OK
    }

    fn mark_file_as_placeholder_inner(&self, sync_folder: &str, relative_path: &str) -> i32 {
        info!(
            "[mark_file_as_placeholder_inner] sync_folder={}, relative_path={}",
            @public(sync_folder),
            @public(relative_path),
        );
        E_OK
    }

    fn unmark_placeholder_file_inner(&self, sync_folder: &str, relative_path: &str) -> i32 {
        info!(
            "[unmark_placeholder_file_inner] sync_folder={}, relative_path={}",
            @public(sync_folder),
            @public(relative_path),
        );
        E_OK
    }

    fn start_hydration_inner(&self, sync_folder: &str, relative_path: &str, priority: i32) -> i32 {
        info!(
            "[start_hydration_inner] sync_folder={}, relative_path={}, priority={}",
            @public(sync_folder),
            @public(relative_path),
            @public(priority),
        );
        E_OK
    }

    fn cancel_hydration_inner(&self, sync_folder: &str, relative_path: &str) -> i32 {
        info!(
            "[cancel_hydration_inner] sync_folder={}, relative_path={}",
            @public(sync_folder),
            @public(relative_path),
        );
        E_OK
    }

    fn execute_inner(&self, request: &CallbackExecuteRequest) -> i32 {
        info!(
            "[execute_inner] sync_folder={}, file_path={}, callback_type={}, offset={}, size={}, total_size={}, is_complete={}",
            @public(&request.sync_folder),
            @public(&request.file_path),
            @public(request.callback_type),
            @public(request.offset),
            @public(request.size),
            @public(request.total_size),
            @public(request.is_complete),
        );
        info!(
            "[execute_inner] req_key_len={}, req_key={:02x?}, data_len={}, data={:02x?}",
            @public(request.req_key.len()),
            @public(&request.req_key),
            @public(request.data.len()),
            @public(&request.data),
        );
        E_OK
    }

    fn dehydrate_inner(&self, sync_folder: &str, relative_path: &str) -> i32 {
        info!(
            "[dehydrate_inner] sync_folder={}, relative_path={}",
            @public(sync_folder),
            @public(relative_path),
        );
        E_OK
    }

    fn update_placeholder_inner(
        &self,
        sync_folder: &str,
        relative_path: &str,
        metadata: &PlaceholderInfo,
        custom_info: &PlaceholderCustomInfo,
    ) -> i32 {
        info!(
            "[update_placeholder_inner] sync_folder={}, relative_path={}, logical_size={}, atime_ms={}, mtime_ms={}",
            @public(sync_folder),
            @public(relative_path),
            @public(metadata.logical_size),
            @public(metadata.atime_ms),
            @public(metadata.mtime_ms),
        );
        info!(
            "[update_placeholder_inner] custom_info.data_len={}, data={:02x?}",
            @public(custom_info.data.len()),
            @public(&custom_info.data),
        );
        E_OK
    }

    fn get_placeholder_custom_info_inner(
        &self,
        sync_folder: &str,
        relative_path: &str,
        custom_info: &mut PlaceholderCustomInfo,
    ) -> i32 {
        info!(
            "[get_placeholder_custom_info_inner] sync_folder={}, relative_path={}",
            @public(sync_folder),
            @public(relative_path),
        );
        custom_info.data = b"RSIPC\0\x01\x7f\x80\xff".to_vec();
        E_OK
    }

    fn start_hydration_by_path_inner(
        &self,
        path: &str,
        callback_type: i32,
        priority: i32,
        accessor_id: u64,
    ) -> i32 {
        info!(
            "[start_hydration_by_path_inner] path={}, callback_type={}, priority={}, accessor_id={}",
            @public(path),
            @public(callback_type),
            @public(priority),
            @public(accessor_id),
        );
        E_OK
    }

    fn dehydrate_file_by_path_inner(&self, path: &str) -> i32 {
        info!(
            "[dehydrate_file_by_path_inner] path={}",
            @public(path),
        );
        E_OK
    }

    fn register_progress_callback_inner(&self, accessor_id: u64, remote: &RemoteObj) -> i32 {
        info!(
            "[register_progress_callback_inner] accessor_id={}, remote.is_proxy={}, remote.is_dead={}",
            @public(accessor_id),
            @public(remote.is_proxy()),
            @public(remote.is_dead()),
        );
        E_OK
    }

    fn unregister_progress_callback_inner(&self, accessor_id: u64) -> i32 {
        info!(
            "[unregister_progress_callback_inner] accessor_id={}",
            @public(accessor_id),
        );
        E_OK
    }

    fn get_placeholder_state_inner(
        &self,
        sync_folder: &str,
        relative_path: &str,
        result: &mut i32,
    ) -> i32 {
        info!(
            "[get_placeholder_state_inner] sync_folder={}, relative_path={}",
            @public(sync_folder),
            @public(relative_path),
        );
        *result = 2;
        E_OK
    }
}
