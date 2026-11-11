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

//! Shared CloudDiskService data types, independent of the IPC framework.

#[derive(Clone, Debug, Default)]
pub(crate) struct FileSyncState {
    pub path: String,
    pub state: i32,
}

#[derive(Clone, Debug, Default)]
pub(crate) struct ChangeData {
    pub update_sequence_number: u64,
    // C++ std::string stores the UUID's 16 raw bytes, including NUL/non-UTF-8 bytes.
    pub file_id: Vec<u8>,
    pub parent_file_id: Vec<u8>,
    pub relative_path: String,
    pub operation_type: u8,
    pub size: u64,
    pub mtime: u64,
    pub time_stamp: u64,
}

#[derive(Clone, Debug, Default)]
pub(crate) struct ChangesResult {
    pub next_usn: u64,
    pub is_eof: bool,
    pub changes_data: Vec<ChangeData>,
}

#[derive(Clone, Debug, Default)]
pub(crate) struct FailedList {
    pub path: String,
    pub error: i32,
}

#[derive(Clone, Debug, Default)]
pub(crate) struct ResultList {
    pub is_success: bool,
    pub state: i32,
    pub path: String,
    pub error: i32,
}

#[derive(Clone, Debug, Default)]
pub(crate) struct PlaceholderInfo {
    pub logical_size: u64,
    pub atime_ms: u64,
    pub mtime_ms: u64,
}

#[derive(Clone, Debug, Default)]
pub(crate) struct PlaceholderCustomInfo {
    pub data: Vec<u8>,
}

#[derive(Clone, Debug, Default)]
pub(crate) struct CallbackExecuteRequest {
    pub req_key: Vec<u8>,
    pub sync_folder: String,
    pub file_path: String,
    pub callback_type: i32,
    pub offset: u64,
    pub size: u64,
    pub total_size: u64,
    pub data: Vec<u8>,
    pub is_complete: bool,
}
