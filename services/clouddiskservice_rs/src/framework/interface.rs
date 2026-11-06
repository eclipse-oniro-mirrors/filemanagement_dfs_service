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

//! Mirrors services/clouddiskservice/ICloudDiskService.idl.
//!
//! Transaction IDs are part of the existing C++ ABI. Keep this list in IDL
//! order and update it whenever that IDL changes; never edit out/ products.

use ipc::remote::RemoteObj;
use ipc::IpcStatusCode;

use crate::common::types::{
    CallbackExecuteRequest, ChangesResult, FailedList, FileSyncState, PlaceholderCustomInfo,
    PlaceholderInfo, ResultList,
};

pub(super) const DESCRIPTOR: &str = "OHOS.FileManagement.CloudDiskService.ICloudDiskService";

#[derive(Clone, Copy)]
#[repr(u32)]
#[allow(clippy::enum_variant_names)] // Match the existing IDL method names.
pub(super) enum CloudDiskServiceIpcCode {
    RegisterSyncFolderChangesInner = 1,
    UnregisterSyncFolderChangesInner = 2,
    RegisterCallbackTableInner = 3,
    UnregisterCallbackTableInner = 4,
    GetSyncFolderChangesInner = 5,
    SetFileSyncStatesInner = 6,
    GetFileSyncStatesInner = 7,
    CreatePlaceholderFileInner = 8,
    IsPlaceholderFileInner = 9,
    RegisterSyncFolderInner = 10,
    UnregisterSyncFolderInner = 11,
    UnregisterForSaInner = 12,
    ConvertPlaceholderToFileInner = 13,
    MarkFileAsPlaceholderInner = 14,
    UnmarkPlaceholderFileInner = 15,
    StartHydrationInner = 16,
    CancelHydrationInner = 17,
    ExecuteInner = 18,
    DehydrateInner = 19,
    UpdatePlaceholderInner = 20,
    GetPlaceholderCustomInfoInner = 21,
    StartHydrationByPathInner = 22,
    DehydrateFileByPathInner = 23,
    RegisterProgressCallbackInner = 24,
    UnregisterProgressCallbackInner = 25,
    GetPlaceholderStateInner = 26,
}

impl TryFrom<u32> for CloudDiskServiceIpcCode {
    type Error = IpcStatusCode;

    fn try_from(code: u32) -> Result<Self, Self::Error> {
        match code {
            1 => Ok(Self::RegisterSyncFolderChangesInner),
            2 => Ok(Self::UnregisterSyncFolderChangesInner),
            3 => Ok(Self::RegisterCallbackTableInner),
            4 => Ok(Self::UnregisterCallbackTableInner),
            5 => Ok(Self::GetSyncFolderChangesInner),
            6 => Ok(Self::SetFileSyncStatesInner),
            7 => Ok(Self::GetFileSyncStatesInner),
            8 => Ok(Self::CreatePlaceholderFileInner),
            9 => Ok(Self::IsPlaceholderFileInner),
            10 => Ok(Self::RegisterSyncFolderInner),
            11 => Ok(Self::UnregisterSyncFolderInner),
            12 => Ok(Self::UnregisterForSaInner),
            13 => Ok(Self::ConvertPlaceholderToFileInner),
            14 => Ok(Self::MarkFileAsPlaceholderInner),
            15 => Ok(Self::UnmarkPlaceholderFileInner),
            16 => Ok(Self::StartHydrationInner),
            17 => Ok(Self::CancelHydrationInner),
            18 => Ok(Self::ExecuteInner),
            19 => Ok(Self::DehydrateInner),
            20 => Ok(Self::UpdatePlaceholderInner),
            21 => Ok(Self::GetPlaceholderCustomInfoInner),
            22 => Ok(Self::StartHydrationByPathInner),
            23 => Ok(Self::DehydrateFileByPathInner),
            24 => Ok(Self::RegisterProgressCallbackInner),
            25 => Ok(Self::UnregisterProgressCallbackInner),
            26 => Ok(Self::GetPlaceholderStateInner),
            _ => Err(IpcStatusCode::Failed),
        }
    }
}

pub(crate) trait ICloudDiskService {
    fn register_sync_folder_changes_inner(&self, sync_folder: &str, remote: &RemoteObj) -> i32;
    fn unregister_sync_folder_changes_inner(&self, sync_folder: &str) -> i32;
    fn register_callback_table_inner(&self, sync_folder: &str, remote: &RemoteObj) -> i32;
    fn unregister_callback_table_inner(&self, sync_folder: &str) -> i32;
    fn get_sync_folder_changes_inner(
        &self,
        sync_folder: &str,
        count: u64,
        start_usn: u64,
        result: &mut ChangesResult,
    ) -> i32;
    fn set_file_sync_states_inner(
        &self,
        sync_folder: &str,
        states: &[FileSyncState],
        result: &mut Vec<FailedList>,
    ) -> i32;
    fn get_file_sync_states_inner(
        &self,
        sync_folder: &str,
        paths: &[String],
        result: &mut Vec<ResultList>,
    ) -> i32;
    fn create_placeholder_file_inner(
        &self,
        sync_folder: &str,
        relative_path: &str,
        info: &PlaceholderInfo,
        custom_info: &PlaceholderCustomInfo,
    ) -> i32;
    fn is_placeholder_file_inner(&self, sync_folder: &str, path: &str, result: &mut bool) -> i32;
    fn register_sync_folder_inner(&self, user_id: i32, bundle_name: &str, path: &str) -> i32;
    fn unregister_sync_folder_inner(&self, user_id: i32, bundle_name: &str, path: &str) -> i32;
    fn unregister_for_sa_inner(&self, path: &str) -> i32;
    fn convert_placeholder_to_file_inner(&self, sync_folder: &str, relative_path: &str) -> i32;
    fn mark_file_as_placeholder_inner(&self, sync_folder: &str, relative_path: &str) -> i32;
    fn unmark_placeholder_file_inner(&self, sync_folder: &str, relative_path: &str) -> i32;
    fn start_hydration_inner(&self, sync_folder: &str, relative_path: &str, priority: i32) -> i32;
    fn cancel_hydration_inner(&self, sync_folder: &str, relative_path: &str) -> i32;
    fn execute_inner(&self, request: &CallbackExecuteRequest) -> i32;
    fn dehydrate_inner(&self, sync_folder: &str, relative_path: &str) -> i32;
    fn update_placeholder_inner(
        &self,
        sync_folder: &str,
        relative_path: &str,
        metadata: &PlaceholderInfo,
        custom_info: &PlaceholderCustomInfo,
    ) -> i32;
    fn get_placeholder_custom_info_inner(
        &self,
        sync_folder: &str,
        relative_path: &str,
        custom_info: &mut PlaceholderCustomInfo,
    ) -> i32;
    fn start_hydration_by_path_inner(
        &self,
        path: &str,
        callback_type: i32,
        priority: i32,
        accessor_id: u64,
    ) -> i32;
    fn dehydrate_file_by_path_inner(&self, path: &str) -> i32;
    fn register_progress_callback_inner(&self, accessor_id: u64, remote: &RemoteObj) -> i32;
    fn unregister_progress_callback_inner(&self, accessor_id: u64) -> i32;
    fn get_placeholder_state_inner(
        &self,
        sync_folder: &str,
        relative_path: &str,
        result: &mut i32,
    ) -> i32;
}
