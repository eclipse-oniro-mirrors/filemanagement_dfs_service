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

//! System accessor paths are routed to the provider of the longest registered sync root.
use super::file::{ExistingFile, TaskKey};
use super::{HydrateProgressOwner, PlaceholderCallbackManager, PlaceholderTaskManager};
use crate::common::error_codes::{E_CALLBACK_NOT_REGISTERED, E_INVALID_ARG, E_OK, E_TRY_AGAIN};
use crate::ffi::utils;
use crate::framework::{CANCEL_FETCH_DATA, FETCH_DATA};
use crate::sync_folder_manager::{self, CloudDiskSyncFolder};

const SANDBOX_PREFIX: &str = "/storage/Users/currentUser/";

pub(crate) fn resolve_user() -> Result<i32, i32> {
    let mut user_id = utils::get_user_id();
    if (user_id == 0 && utils::get_account_id(&mut user_id) != E_OK) || user_id < 0 {
        return Err(E_TRY_AGAIN);
    }
    Ok(user_id)
}

fn user_for_path(path: &str) -> Result<i32, i32> {
    if path.len() > libc::PATH_MAX as usize
        || !path.starts_with(SANDBOX_PREFIX)
        || path.as_bytes().contains(&0)
        || path.ends_with('/')
        || path
            .split('/')
            .any(|component| matches!(component, "." | ".."))
    {
        return Err(E_INVALID_ARG);
    }
    resolve_user()
}

// Match IsPathInSyncFolder at a component boundary and preserve the original suffix.
fn relative_under<'a>(root: &str, path: &'a str) -> Option<&'a str> {
    if root.is_empty() {
        return None;
    }
    let suffix = path.strip_prefix(root)?;
    let relative = if root.ends_with('/') {
        suffix
    } else {
        suffix.strip_prefix('/')?
    };
    (!relative.is_empty()).then_some(relative)
}

fn resolve_target(path: &str, user_id: i32) -> Result<ExistingFile, i32> {
    let user = user_id.to_string();
    let physical = sync_folder_manager::path_to_physical_path(path, &user)?;
    let (index, folder, relative) = CloudDiskSyncFolder::get_instance()
        .get_sync_folder_map()
        .into_iter()
        .filter(|(_, folder)| !folder.bundle_name.is_empty())
        .filter_map(|(index, folder)| {
            let relative = relative_under(&folder.path, &physical)?;
            Some((index, folder, relative))
        })
        .max_by_key(|(_, folder, _)| folder.path.len())
        .ok_or(E_CALLBACK_NOT_REGISTERED)?;
    let root = sync_folder_manager::path_to_sandbox_path_by_physical_path(&folder.path, &user)
        .map_err(|_| E_CALLBACK_NOT_REGISTERED)?;
    ExistingFile::resolve_for_accessor(
        TaskKey::new(index, &root, relative),
        user_id,
        folder.bundle_name,
    )
}

fn resolve_cancel_key(path: &str, user_id: i32) -> Result<(TaskKey, String), i32> {
    // Cancellation must find the task even when its target has been renamed or deleted.
    let user = user_id.to_string();
    let user_root = format!("/data/service/el2/{user}/hmdfs/account/files/Docs");
    CloudDiskSyncFolder::get_instance()
        .get_sync_folder_map()
        .into_iter()
        .filter(|(_, f)| !f.bundle_name.is_empty() && relative_under(&user_root, &f.path).is_some())
        .filter_map(|(index, f)| {
            let root =
                sync_folder_manager::path_to_sandbox_path_by_physical_path(&f.path, &user).ok()?;
            let relative = relative_under(&root, path)?;
            Some((TaskKey::new(index, &root, relative), f.bundle_name))
        })
        .max_by_key(|(key, _)| key.sync_folder.len())
        .ok_or(E_CALLBACK_NOT_REGISTERED)
}

pub(crate) fn start_hydration(
    path: &str,
    callback_type: i32,
    priority: i32,
    owner: HydrateProgressOwner,
) -> Result<(), i32> {
    if !matches!(callback_type, FETCH_DATA | CANCEL_FETCH_DATA) || !(0..=2).contains(&priority) {
        return Err(E_INVALID_ARG);
    }
    let user_id = user_for_path(path)?;
    let callbacks = PlaceholderCallbackManager::get_instance();
    let tasks = PlaceholderTaskManager::get_instance();
    if callback_type == CANCEL_FETCH_DATA {
        let (key, bundle) = resolve_cancel_key(path, user_id)?;
        if !callbacks.is_callback_registered(&bundle, key.index) {
            return Err(E_CALLBACK_NOT_REGISTERED);
        }
        return tasks.cancel_task(&key.sync_folder, &key.file_path, key.index, Some(owner));
    }
    let target = resolve_target(path, user_id)?;
    if !callbacks.is_callback_registered(&target.bundle, target.key.index) {
        return Err(E_CALLBACK_NOT_REGISTERED);
    }
    tasks.create_task(target, priority, Some((owner, path)))
}

pub(crate) fn dehydrate(path: &str) -> Result<(), i32> {
    resolve_target(path, user_for_path(path)?)?.dehydrate()
}
