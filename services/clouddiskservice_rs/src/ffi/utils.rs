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

use ipc::Skeleton;

use crate::common::error_codes::{E_GET_TOKEN_INFO_ERROR, E_INVALID_ARG, E_OK, E_OSACCOUNT};

#[cxx::bridge(namespace = "OHOS::FileManagement::CloudDiskService")]
mod bridge {
    // SAFETY: The wrappers forward token IDs and a borrowed C++ string to
    // AccessTokenKit. CXX keeps strings/output references alive for synchronous
    // calls; the account wrappers do not retain borrowed data either.
    unsafe extern "C++" {
        include!("utils.h");

        #[rust_name = "get_token_type_flag"]
        fn GetTokenTypeFlag(token_id: u32) -> i32;

        #[rust_name = "verify_access_token"]
        fn VerifyAccessToken(token_id: u32, permission_name: &CxxString) -> i32;

        #[rust_name = "get_hap_bundle_name"]
        fn GetHapBundleName(
            token_id: u32,
            bundle_name: Pin<&mut CxxString>,
            inst_index: &mut i32,
        ) -> i32;

        #[rust_name = "get_native_process_name"]
        fn GetNativeProcessName(token_id: u32, process_name: Pin<&mut CxxString>) -> i32;

        #[rust_name = "get_active_user"]
        fn GetActiveUser(user_id: &mut i32) -> bool;

        #[rust_name = "is_user_verified"]
        fn IsUserVerified(user_id: i32) -> bool;
    }
}

pub(crate) use bridge::is_user_verified;

pub(crate) const PERM_CLOUD_DISK_SERVICE: &str = "ohos.permission.ACCESS_CLOUD_DISK_INFO";

const ROOT_UID: u64 = 0;
const BASE_USER_RANGE: u64 = 200_000;

// ATokenTypeEnum and PermissionState in access_token_basic_type.h.
pub(crate) const TOKEN_HAP: i32 = 0;
pub(crate) const TOKEN_NATIVE: i32 = 1;
pub(crate) const TOKEN_SHELL: i32 = 2;
pub(crate) const PERMISSION_DENIED: i32 = -1;

pub(crate) fn check_caller_permission(permission_name: &str) -> bool {
    let token_id = Skeleton::calling_token_id();
    let uid = Skeleton::calling_uid();
    let token_type = bridge::get_token_type_flag(token_id);
    info!(
        "[CheckCallerPermission] token_id={}, uid={}, token_type={}, permission={}",
        @private(token_id),
        @public(uid),
        @public(token_type),
        @public(permission_name),
    );
    match token_type {
        TOKEN_HAP | TOKEN_NATIVE => {
            let is_granted = check_permission(token_id, permission_name);
            if !is_granted {
                error!("[CheckCallerPermission] permission denied, token_type={}", @public(token_type));
            }
            is_granted
        }
        TOKEN_SHELL if uid == ROOT_UID => {
            info!("[CheckCallerPermission] root shell caller denied");
            false
        }
        _ => {
            error!("[CheckCallerPermission] unsupported token_type={}", @public(token_type));
            false
        }
    }
}

pub(crate) fn check_permission(token_id: u32, permission_name: &str) -> bool {
    cxx::let_cxx_string!(permission = permission_name);
    let ret = bridge::verify_access_token(token_id, &permission);
    info!(
        "[CheckPermission] permission={}, result={}",
        @public(permission_name),
        @public(ret),
    );
    if ret == PERMISSION_DENIED {
        error!("[CheckPermission] permission denied, permission={}", @public(permission_name));
        return false;
    }
    true
}

pub(crate) fn get_user_id() -> i32 {
    (Skeleton::calling_uid() / BASE_USER_RANGE) as i32
}

pub(crate) fn get_caller_bundle_name() -> Result<String, i32> {
    let token_id = Skeleton::calling_token_id();
    let token_type = bridge::get_token_type_flag(token_id);
    cxx::let_cxx_string!(name = "");
    let ret = match token_type {
        TOKEN_HAP => {
            let mut inst_index = 0;
            let ret = bridge::get_hap_bundle_name(token_id, name.as_mut(), &mut inst_index);
            if ret == E_OK && inst_index != 0 {
                error!("[GetCallerBundleName] app twin is not supported, inst_index={}", @public(inst_index));
                return Err(E_INVALID_ARG);
            }
            ret
        }
        TOKEN_NATIVE | TOKEN_SHELL => bridge::get_native_process_name(token_id, name.as_mut()),
        _ => {
            error!("[GetCallerBundleName] unsupported token_type={}", @public(token_type));
            return Err(E_GET_TOKEN_INFO_ERROR);
        }
    };
    if ret != E_OK {
        error!("[GetCallerBundleName] get token info failed, token_type={}, ret={}", @public(token_type), @public(ret));
        return Err(E_GET_TOKEN_INFO_ERROR);
    }
    let name = name.to_str().map_err(|_| E_INVALID_ARG)?;
    if name.is_empty() {
        error!("[GetCallerBundleName] bundle name is empty");
        return Err(E_INVALID_ARG);
    }
    info!("[GetCallerBundleName] bundle_name={}", @private(name));
    Ok(name.to_owned())
}

pub(crate) fn get_account_id(user_id: &mut i32) -> i32 {
    if !bridge::get_active_user(user_id) {
        error!("[GetAccountId] query active user failed");
        return E_OSACCOUNT;
    }
    if !bridge::is_user_verified(*user_id) {
        error!("[GetAccountId] user is not verified, user_id={}", @public(*user_id));
        return E_INVALID_ARG;
    }
    info!("[GetAccountId] user_id={}", @public(*user_id));
    E_OK
}
