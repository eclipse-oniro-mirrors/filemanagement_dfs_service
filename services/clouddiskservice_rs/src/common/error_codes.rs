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

//! Shared business errors mirror every CloudDiskServiceErrCode entry in
//! utils/clouddiskservice/include/cloud_disk_service_error.h.
//!
//! Explicit values preserve the C++ enum's implicit increments. Although the
//! C++ enum uses uint32_t, all values fit i32, the service return and IPC type.

// Retain the complete error contract while business methods are implemented.
#![allow(dead_code)]

pub(crate) const E_OK: i32 = 0;
pub(crate) const E_GET_TOKEN_INFO_ERROR: i32 = 1;
pub(crate) const E_OSACCOUNT: i32 = 2;
pub(crate) const E_PATH_NOT_EXIST: i32 = 3;
pub(crate) const E_PERM: i32 = 13_900_001;
pub(crate) const E_ACCES: i32 = 13_900_012;

pub(crate) const E_INVALID_ARG: i32 = 34_400_001;
pub(crate) const E_SYNC_FOLDER_PATH_UNAUTHORIZED: i32 = 34_400_002;
pub(crate) const E_IPC_FAILED: i32 = 34_400_003;
pub(crate) const E_SYNC_FOLDER_LIMIT_EXCEEDED: i32 = 34_400_004;
pub(crate) const E_CONFLICT_THIS_APP: i32 = 34_400_005;
pub(crate) const E_CONFLICT_OTHER_APP: i32 = 34_400_006;
pub(crate) const E_REGISTER_SYNC_FOLDER_FAILED: i32 = 34_400_007;
pub(crate) const E_SYNC_FOLDER_NOT_REGISTERED: i32 = 34_400_008;
pub(crate) const E_UNREGISTER_SYNC_FOLDER_FAILED: i32 = 34_400_009;
pub(crate) const E_SYNC_FOLDER_PATH_NOT_EXIST: i32 = 34_400_010;
pub(crate) const E_LISTENER_NOT_REGISTERED: i32 = 34_400_011;
pub(crate) const E_LISTENER_ALREADY_REGISTERED: i32 = 34_400_012;
pub(crate) const E_INVALID_CHANGE_SEQUENCE: i32 = 34_400_013;
pub(crate) const E_TRY_AGAIN: i32 = 34_400_014;
pub(crate) const E_NOT_ALLOWED: i32 = 34_400_015;
pub(crate) const E_FILE_ALREADY_EXISTS: i32 = 34_400_016;
pub(crate) const E_NOT_A_PLACEHOLDER: i32 = 34_400_017;
pub(crate) const E_IS_A_PLACEHOLDER: i32 = 34_400_018;
pub(crate) const E_HYDRATE_IN_PROGRESS: i32 = 34_400_019;
pub(crate) const E_NO_SPACE_LEFT: i32 = 34_400_020;
pub(crate) const E_CALLBACK_NOT_REGISTERED: i32 = 34_400_021;
pub(crate) const E_CALLBACK_ALREADY_REGISTERED: i32 = 34_400_022;
pub(crate) const E_NOT_A_DIRECTORY: i32 = 34_400_023;
pub(crate) const E_FILE_NOT_EXIST: i32 = 34_400_024;
pub(crate) const E_NAME_TOO_LONG: i32 = 34_400_025;
pub(crate) const E_FILE_TOO_LARGE: i32 = 34_400_026;
pub(crate) const E_PLACEHOLDER_CUSTOM_INFO_NOT_FOUND: i32 = 34_400_027;
pub(crate) const E_PLACEHOLDER_NOT_FULLY_HYDRATED: i32 = 34_400_028;
pub(crate) const E_DEHYDRATE_DENIED: i32 = 34_400_029;
pub(crate) const E_CANCELLED: i32 = 34_400_030;
pub(crate) const E_ALREADY_HYDRATED: i32 = 34_400_031;
pub(crate) const E_NO_HYDRATION_IN_PROGRESS: i32 = 34_400_032;
pub(crate) const E_INVALID_PLACEHOLDER_STATE: i32 = 34_400_033;
pub(crate) const E_HYDRATION_TASK_LIMIT_REACHED: i32 = 34_400_034;

pub(crate) const E_NOT_SUPPORTED: i32 = 801;
pub(crate) const E_PERMISSION_DENIED: i32 = 201;
pub(crate) const E_PERMISSION_SYSTEM: i32 = 202;
