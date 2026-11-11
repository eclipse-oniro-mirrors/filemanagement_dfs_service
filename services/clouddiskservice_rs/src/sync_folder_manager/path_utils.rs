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

//! Sync-folder path conversion and validation.

use std::fs;
use std::io::ErrorKind;

use crate::common::error_codes::{E_INVALID_ARG, E_SYNC_FOLDER_PATH_NOT_EXIST};

pub(crate) fn path_to_physical_path(path: &str, user_id: &str) -> Result<String, i32> {
    info!(
        "[PathToPhysicalPath] user_id={}, path={}",
        @public(user_id),
        @private(path),
    );
    let sandbox_path = "/storage/Users/currentUser/";
    let replacement_path = format!("/data/service/el2/{user_id}/hmdfs/account/files/Docs/");
    replace_path_prefix(sandbox_path, &replacement_path, path)
}

pub(crate) fn path_to_mnt_path_by_sandbox_path(path: &str, user_id: &str) -> Result<String, i32> {
    info!("[PathToMntPathBySandboxPath] user_id={}, path={}", @public(user_id), @private(path));
    let replacement_path = format!("/mnt/hmdfs/{user_id}/account/device_view/local/files/Docs");
    replace_path_prefix("/storage/Users/currentUser", &replacement_path, path)
}

pub(crate) fn path_to_mnt_path_by_physical_path(path: &str, user_id: &str) -> Result<String, i32> {
    let physical_path = format!("/data/service/el2/{user_id}/hmdfs/account/files/Docs");
    let replacement_path = format!("/mnt/hmdfs/{user_id}/account/device_view/local/files/Docs");
    replace_physical_prefix(&physical_path, &replacement_path, path)
}

pub(crate) fn path_to_sandbox_path_by_physical_path(
    path: &str,
    user_id: &str,
) -> Result<String, i32> {
    let physical_path = format!("/data/service/el2/{user_id}/hmdfs/account/files/Docs");
    replace_physical_prefix(&physical_path, "/storage/Users/currentUser", path)
}

// Unlike ReplacePathPrefix, the C++ physical-path helpers do not call realpath:
// UnregisterForSa must also handle a root that was already deleted or moved.
fn replace_physical_prefix(old_prefix: &str, new_prefix: &str, path: &str) -> Result<String, i32> {
    let Some(suffix) = path.strip_prefix(old_prefix) else {
        error!("[ReplacePhysicalPrefix] invalid path prefix, path={}", @private(path));
        return Err(E_INVALID_ARG);
    };
    let result = format!("{new_prefix}{suffix}");
    info!("[ReplacePhysicalPrefix] path={}, result={}", @private(path), @private(&result));
    Ok(result)
}

fn replace_path_prefix(
    old_prefix: &str,
    new_prefix: &str,
    input_path: &str,
) -> Result<String, i32> {
    let Some(suffix) = input_path.strip_prefix(old_prefix) else {
        error!("[ReplacePathPrefix] invalid path prefix, path={}", @private(input_path));
        return Err(E_INVALID_ARG);
    };
    let new_path = format!("{new_prefix}{suffix}");
    // On Unix, canonicalize resolves symlinks and dot components via realpath.
    let real_path = fs::canonicalize(&new_path).map_err(|error| {
        let ret = if error.kind() == ErrorKind::NotFound {
            E_SYNC_FOLDER_PATH_NOT_EXIST
        } else {
            E_INVALID_ARG
        };
        error!(
            "[ReplacePathPrefix] realpath failed, path={}, error={}, ret={}",
            @private(&new_path),
            @public(error),
            @public(ret),
        );
        ret
    })?;
    // Never use lossy conversion for a path that will be hashed.
    let real_path = real_path.into_os_string().into_string().map_err(|_| {
        error!("[ReplacePathPrefix] resolved path is not UTF-8");
        E_INVALID_ARG
    })?;
    // Preserve the exact prefix check used by the C++ path helpers.
    if !real_path.starts_with(new_prefix) {
        error!(
            "[ReplacePathPrefix] resolved path outside prefix, prefix={}, path={}",
            @private(new_prefix),
            @private(&real_path),
        );
        return Err(E_INVALID_ARG);
    }
    info!("[ReplacePathPrefix] resolved_path={}", @private(&real_path));
    Ok(real_path)
}
