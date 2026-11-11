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
//! Sync-folder registration, change callbacks/history and file sync states use
//! the Rust implementation. Placeholder entry points remain framework stubs.

use std::fs;
use std::path::Path;
use std::sync::atomic::{AtomicBool, AtomicI32, Ordering};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use cxx::UniquePtr;
use ipc::remote::RemoteObj;
use samgr::manage::SystemAbilityManager;
use system_ability_fwk::ability::Handler;

use crate::account_status_listener::AccountStatusListener;
use crate::common::error_codes::{
    error_reason, E_INVALID_ARG, E_IPC_FAILED, E_LISTENER_ALREADY_REGISTERED,
    E_LISTENER_NOT_REGISTERED, E_OK, E_PERMISSION_DENIED, E_SYNC_FOLDER_NOT_REGISTERED,
    E_TRY_AGAIN,
};
use crate::common::types::{
    CallbackExecuteRequest, ChangesResult, FailedList, FileSyncState, PlaceholderCustomInfo,
    PlaceholderInfo, ResultList,
};
use crate::ffi::{self, ffrt, utils};
use crate::framework::{AbilityHandler, ICloudDiskService, CLOUD_DISK_SERVICE_SA_ID};
use crate::monitor::DiskMonitor;
use crate::placeholder::{
    PlaceholderCallbackManager, PlaceholderProgressManager, PlaceholderTaskManager,
};
use crate::sync_folder_manager::{self, CloudDiskSyncFolder, SyncFolderValue};
use crate::sync_folder_service::{
    file_state, CloudDiskServiceCallbackManager, CloudDiskServiceSyncFolder,
};

const UNLOAD_DELAY_MS: u64 = 5_000;
const USER_UNLOCKED_REASON: &str = "usual.event.USER_UNLOCKED";
const GET_SYNC_FOLDERS_RETRY_ERR_CODE: i32 = 34_400_001;
const GET_SYNC_FOLDERS_MAX_RETRY_TIMES: u32 = 2;
const GET_SYNC_FOLDERS_RETRY_INTERVAL: Duration = Duration::from_millis(200);
const GET_SYNC_FOLDER_CHANGE_MAX: u64 = 100;
const GET_FILE_SYNC_MAX: usize = 100;
const SYNC_CONFLICTED: u8 = 5;

struct InitState {
    result: AtomicI32,
    stopped: AtomicBool,
}

// C++ startMutex_ protects initTask_ and initState_ together.
struct InitTask {
    handle: UniquePtr<ffrt::FfrtTaskHandle>,
    state: Option<Arc<InitState>>,
}

impl Default for InitTask {
    fn default() -> Self {
        Self {
            handle: UniquePtr::null(),
            state: None,
        }
    }
}

// SAFWK and the published stub share the same lifecycle state.
#[derive(Clone)]
pub(crate) struct CloudDiskService {
    init_task: Arc<Mutex<InitTask>>,
    ability_handler: Arc<Mutex<Option<AbilityHandler>>>,
    account_status_listener: Arc<AccountStatusListener>,
    // Corresponds to C++ unloadMutex_ and unloadTaskHandle_.
    unload_task_handle: Arc<Mutex<UniquePtr<ffrt::FfrtTaskHandle>>>,
}

impl CloudDiskService {
    pub(crate) fn new() -> Self {
        Self {
            init_task: Arc::new(Mutex::new(InitTask::default())),
            ability_handler: Arc::new(Mutex::new(None)),
            account_status_listener: Arc::new(AccountStatusListener::new()),
            unload_task_handle: Arc::new(Mutex::new(UniquePtr::null())),
        }
    }

    pub(crate) fn start(&self, handler: Handler, reason_name: String, reason_value: String) {
        info!("[OnStart] begin, reason={}, value={}", @public(&reason_name), @private(&reason_value));
        let mut init_task = self.init_task.lock().unwrap_or_else(|err| err.into_inner());
        if !init_task.handle.is_null() {
            warn!("[OnStart] service already started");
            return;
        }
        let state = Arc::new(InitState {
            result: AtomicI32::new(E_TRY_AGAIN),
            stopped: AtomicBool::new(false),
        });
        init_task.state = Some(Arc::clone(&state));
        if !handler.publish(self.clone()) {
            error!("[OnStart] publish failed");
            return;
        }
        *self
            .ability_handler
            .lock()
            .unwrap_or_else(|err| err.into_inner()) = Some(AbilityHandler::new(handler));

        let service = self.clone();
        let task_state = Arc::clone(&state);
        init_task.handle = ffrt::submit_task_with_handle(move || {
            let ret = service.init(&reason_name, &reason_value);
            task_state.result.store(ret, Ordering::Release);
        });
        let on_initialized = move || {
            let ret = state.result.load(Ordering::Acquire);
            if ret != E_OK && !state.stopped.load(Ordering::Acquire) {
                error!("[OnStart] initialization failed, ret={}", @public(ret));
                let ret = SystemAbilityManager::unload_system_ability(CLOUD_DISK_SERVICE_SA_ID);
                info!("[OnStart] unload after initialization failure, ret={}", @public(ret));
            }
        };
        if let Some(handle) = init_task.handle.as_ref() {
            // Keep unload outside the init task: OnStop waits for that task.
            ffrt::submit_task_after(handle, on_initialized);
            info!("[OnStart] published, initialization submitted");
        } else {
            error!("[OnStart] submit initialization failed");
            let _ = ffrt::spawn(on_initialized);
        }
    }

    fn init(&self, reason_name: &str, reason_value: &str) -> i32 {
        let Some(user_id) = get_user_id_by_start_reason(reason_name, reason_value) else {
            return E_TRY_AGAIN;
        };
        info!("[Init] user_id={}", @public(user_id));
        let folders = match get_all_sync_folders_for_sa_with_retry() {
            Ok(folders) => folders,
            Err(ret) => return ret,
        };
        let user = user_id.to_string();
        for folder in folders {
            let path = match sync_folder_manager::path_to_physical_path(&folder.path, &user) {
                Ok(path) => path,
                Err(ret) => {
                    warn!("[Init] skip invalid sync folder, path={}, ret={}",
                        @private(&folder.path), @public(ret));
                    continue;
                }
            };
            let index = sync_folder_manager::dentry_hash(&path);
            CloudDiskSyncFolder::get_instance().add_sync_folder(
                index,
                SyncFolderValue {
                    bundle_name: folder.bundle_name,
                    path,
                },
            );
        }

        PlaceholderTaskManager::get_instance().start_scheduler();
        self.account_status_listener.prepare(user_id);
        if let Some(handler) = self
            .ability_handler
            .lock()
            .unwrap_or_else(|err| err.into_inner())
            .as_ref()
        {
            if !handler.listen_common_events() {
                error!("[Init] listen to common-event SA failed");
            }
        }
        let count = CloudDiskSyncFolder::get_instance().get_sync_folder_size();
        if count > 0 {
            DiskMonitor::get_instance().start_monitor(user_id);
        } else {
            self.unload_sa();
        }
        info!("[Init] completed, user_id={}, sync_folder_count={}", @public(user_id), @public(count));
        E_OK
    }

    pub(crate) fn wait_for_init(&self) -> i32 {
        let (handle, state) = {
            let init_task = self.init_task.lock().unwrap_or_else(|err| err.into_inner());
            let (Some(handle), Some(state)) = (init_task.handle.as_ref(), init_task.state.as_ref())
            else {
                error!("[WaitForInit] initialization task unavailable");
                return E_IPC_FAILED;
            };
            (ffrt::clone_task_handle(handle), Arc::clone(state))
        };
        // Do not hold startMutex_ while an IPC thread waits for initialization.
        ffrt::wait_task(&handle);
        let ret = state.result.load(Ordering::Acquire);
        let stopped = state.stopped.load(Ordering::Acquire);
        if stopped || ret != E_OK {
            error!("[WaitForInit] unavailable, stopped={}, init_result={}", @public(stopped), @public(ret));
            return E_IPC_FAILED;
        }
        E_OK
    }

    pub(crate) fn on_common_event_service_ready(&self) {
        self.account_status_listener.start();
    }

    pub(crate) fn stop(&self) {
        info!("[OnStop] begin");
        self.cancel_pending_unload();
        let mut init_task = self.init_task.lock().unwrap_or_else(|err| err.into_inner());
        if let Some(state) = &init_task.state {
            state.stopped.store(true, Ordering::Release);
        }
        if let Some(handle) = init_task.handle.as_ref() {
            ffrt::wait_task(handle);
        }
        init_task.handle = UniquePtr::null();
        drop(init_task);
        // Init may have scheduled an idle unload while OnStop was waiting.
        self.cancel_pending_unload();
        PlaceholderCallbackManager::get_instance().clear_all();
        PlaceholderTaskManager::get_instance().stop_scheduler();
        PlaceholderProgressManager::get_instance().drain();
        PlaceholderProgressManager::get_instance().clear();
        self.account_status_listener.stop();
        CloudDiskServiceCallbackManager::get_instance().clear_map();
        // Match C++ OnStop: leave the common-event SA listener with SAFWK.
        // The stopped account listener ignores any late SA notifications.
        *self
            .ability_handler
            .lock()
            .unwrap_or_else(|err| err.into_inner()) = None;
        // Prevent lazy cache reopen, then cancel scans before joining the monitor.
        CloudDiskSyncFolder::get_instance().clear_map();
        CloudDiskServiceSyncFolder::cloud_disk_service_clear_all();
        DiskMonitor::get_instance().stop_monitor();
        info!("[OnStop] completed");
    }

    pub(crate) fn cancel_pending_unload(&self) {
        info!("[CancelPendingUnload] begin");
        let mut task_handle = self.unload_task_handle.lock().unwrap_or_else(|poisoned| {
            error!("[CancelPendingUnload] unload mutex poisoned, recovering");
            poisoned.into_inner()
        });
        if let Some(handle) = task_handle.as_ref() {
            let ret = ffrt::cancel_queue_task(handle);
            // C++ clears the handle even if the task has already started.
            *task_handle = UniquePtr::null();
            info!(
                "[CancelPendingUnload] cancel result={}, handle cleared",
                @public(ret),
            );
        } else {
            info!("[CancelPendingUnload] no pending task");
        }
    }

    fn unload_sa(&self) {
        let count = CloudDiskSyncFolder::get_instance().get_sync_folder_size();
        info!("[UnloadSa] sync_folder_count={}", @public(count));
        if count == 0 {
            DiskMonitor::get_instance().stop_monitor();
            self.delay_unload_sa();
        }
    }

    fn delay_unload_sa(&self) {
        self.cancel_pending_unload();
        let mut handle = self
            .unload_task_handle
            .lock()
            .unwrap_or_else(|err| err.into_inner());
        *handle = ffrt::submit_delayed_task(
            || {
                if CloudDiskSyncFolder::get_instance().get_sync_folder_size() > 0 {
                    info!("[DelayUnloadSa] sync folder registered during delay, skip unload");
                    return;
                }
                let ret = SystemAbilityManager::unload_system_ability(CLOUD_DISK_SERVICE_SA_ID);
                if ret != E_OK {
                    error!(
                        "[UnloadSa] unload request failed, sa_id={}, ret={}",
                        @public(CLOUD_DISK_SERVICE_SA_ID),
                        @public(ret),
                    );
                } else {
                    info!(
                        "[UnloadSa] unload system ability requested, sa_id={}",
                        @public(CLOUD_DISK_SERVICE_SA_ID),
                    );
                }
            },
            UNLOAD_DELAY_MS * 1_000,
        );
        if handle.is_null() {
            error!("[DelayUnloadSa] submit failed");
        } else {
            info!("[DelayUnloadSa] submitted, delay_ms={}", @public(UNLOAD_DELAY_MS));
        }
    }
}

fn get_user_id_by_start_reason(reason_name: &str, reason_value: &str) -> Option<i32> {
    if reason_name == USER_UNLOCKED_REASON {
        // std::from_chars rejects leading '+', whitespace, trailing text and overflow.
        let user_id = reason_value
            .parse::<i32>()
            .ok()
            .filter(|id| *id >= 0 && !reason_value.starts_with('+'));
        if let Some(user_id) = user_id {
            if utils::is_user_verified(user_id) {
                return Some(user_id);
            }
        }
        error!("[GetUserIdByStartReason] invalid or unverified user, value={}", @private(reason_value));
        return None;
    }
    let mut user_id = utils::get_user_id();
    if user_id == 0 && utils::get_account_id(&mut user_id) != E_OK {
        return None;
    }
    Some(user_id)
}

fn get_all_sync_folders_for_sa_with_retry() -> Result<Vec<SyncFolderValue>, i32> {
    for retry in 0..=GET_SYNC_FOLDERS_MAX_RETRY_TIMES {
        match ffi::sync_folder_manager::get_all_sync_folders_for_sa() {
            Err(ret)
                if ret == GET_SYNC_FOLDERS_RETRY_ERR_CODE
                    && retry < GET_SYNC_FOLDERS_MAX_RETRY_TIMES =>
            {
                warn!("[Init] get sync folders failed, ret={}, retry={}", @public(ret), @public(retry + 1));
                std::thread::sleep(GET_SYNC_FOLDERS_RETRY_INTERVAL);
            }
            result => {
                match &result {
                    Ok(folders) => {
                        info!("[Init] got sync folders, count={}", @public(folders.len()))
                    }
                    Err(ret) => error!("[Init] get sync folders failed, ret={}", @public(ret)),
                }
                return result;
            }
        }
    }
    Err(GET_SYNC_FOLDERS_RETRY_ERR_CODE)
}

// C++ CheckPermissions currently ignores isSystemApp.
fn check_permissions(permission: &str, _is_system_app: bool) -> i32 {
    info!("[CheckPermissions] permission={}", @public(permission));
    if !permission.is_empty() && !utils::check_caller_permission(permission) {
        error!("[CheckPermissions] permission denied");
        return E_PERMISSION_DENIED;
    }
    info!("[CheckPermissions] permission check passed");
    E_OK
}

// C++ GetErrorNum maps errno to the per-file ErrorReason, not the API status.
fn get_error_num(error: i32) -> i32 {
    match error {
        libc::EDQUOT => error_reason::NO_SPACE_LEFT,
        libc::ERANGE => error_reason::OUT_OF_RANGE,
        libc::ENOENT => error_reason::NO_SUCH_FILE,
        _ => error_reason::NO_SYNC_STATE,
    }
}

fn set_file_sync_state(item: &FileSyncState, user_id: i32, sync_folder: &str) -> Result<(), i32> {
    let path =
        sync_folder_manager::path_to_mnt_path_by_sandbox_path(&item.path, &user_id.to_string())
            .map_err(|_| error_reason::NO_SUCH_FILE)?;
    let prefix = if sync_folder.ends_with('/') {
        sync_folder.to_owned()
    } else {
        format!("{sync_folder}/")
    };
    if !item.path.starts_with(&prefix) {
        return Err(error_reason::INVALID_ARGUMENT);
    }
    // C++ casts SyncState to uint8_t before checking SYNC_CONFLICTED.
    let state = item.state as u8;
    if state > SYNC_CONFLICTED {
        return Err(error_reason::OUT_OF_RANGE);
    }
    file_state::set_file_sync_state(Path::new(&path), state).map_err(|err| {
        error!("[SetFileSyncState] set xattr failed, path={}, error={}", @private(&path), @public(&err));
        get_error_num(err.raw_os_error().unwrap_or(libc::EIO))
    })
}

fn get_file_sync_state(path: &str, user_id: i32, sync_folder: &str) -> ResultList {
    let mut result = ResultList {
        path: path.to_owned(),
        ..ResultList::default()
    };
    let mnt_path =
        match sync_folder_manager::path_to_mnt_path_by_sandbox_path(path, &user_id.to_string()) {
            Ok(path) => path,
            Err(_) => {
                result.error = error_reason::NO_SUCH_FILE;
                return result;
            }
        };
    let prefix = if sync_folder.ends_with('/') {
        sync_folder.to_owned()
    } else {
        format!("{sync_folder}/")
    };
    if !path.starts_with(&prefix) {
        result.error = error_reason::INVALID_ARGUMENT;
        return result;
    }
    match fs::metadata(&mnt_path) {
        Ok(stat) if stat.is_dir() => {
            result.error = error_reason::NO_SYNC_STATE;
            return result;
        }
        Ok(_) => {}
        Err(err) => {
            error!("[GetFileSyncState] stat failed, path={}, error={}", @private(&mnt_path), @public(&err));
            result.error = get_error_num(err.raw_os_error().unwrap_or(libc::EIO));
            return result;
        }
    }
    let byte = match file_state::get_file_sync_state_byte(Path::new(&mnt_path), false) {
        Ok(byte) => byte,
        Err(err) => {
            error!("[GetFileSyncState] get xattr failed, path={}, error={}", @private(&mnt_path), @public(&err));
            result.error = get_error_num(err.raw_os_error().unwrap_or(libc::EIO));
            return result;
        }
    };
    let state = file_state::get_sync_state_from_file_sync_state(byte);
    if state > SYNC_CONFLICTED {
        result.error = error_reason::INVALID_ARGUMENT;
        return result;
    }
    result.is_success = true;
    result.state = i32::from(state);
    result
}

impl ICloudDiskService for CloudDiskService {
    fn register_sync_folder_changes_inner(&self, sync_folder: &str, remote: &RemoteObj) -> i32 {
        info!(
            "[register_sync_folder_changes_inner] sync_folder={}, remote.is_proxy={}, remote.is_dead={}",
            @private(sync_folder),
            @public(remote.is_proxy()),
            @public(remote.is_dead()),
        );
        // RemoteObj is non-null: the IPC stub rejects a missing remote object.
        let user_id = utils::get_user_id();
        let path =
            match sync_folder_manager::path_to_physical_path(sync_folder, &user_id.to_string()) {
                Ok(path) => path,
                Err(ret) => return ret,
            };
        let bundle_name = match utils::get_caller_bundle_name() {
            Ok(name) => name,
            Err(ret) => {
                error!("[RegisterSyncFolderChanges] get bundle name failed, ret={}", @public(ret));
                return E_TRY_AGAIN;
            }
        };
        let index = sync_folder_manager::dentry_hash(path);
        if !CloudDiskSyncFolder::get_instance()
            .get_sync_folder_value_by_index(index)
            .is_some_and(|folder| folder.bundle_name == bundle_name)
        {
            error!("[RegisterSyncFolderChanges] sync folder is not registered for caller, index={}", @public(index));
            return E_SYNC_FOLDER_NOT_REGISTERED;
        }
        let callbacks = CloudDiskServiceCallbackManager::get_instance();
        if !callbacks.register_sync_folder_map(&bundle_name, index, remote, user_id) {
            return E_LISTENER_ALREADY_REGISTERED;
        }
        CloudDiskServiceSyncFolder::register_sync_folder_changes(user_id, index);
        callbacks.add_callback(&bundle_name, remote);
        info!("[RegisterSyncFolderChanges] completed, user_id={}, index={}, bundle_name={}",
            @public(user_id), @public(index), @private(&bundle_name));
        E_OK
    }

    fn unregister_sync_folder_changes_inner(&self, sync_folder: &str) -> i32 {
        info!(
            "[unregister_sync_folder_changes_inner] sync_folder={}",
            @private(sync_folder),
        );
        let user_id = utils::get_user_id();
        let path =
            match sync_folder_manager::path_to_physical_path(sync_folder, &user_id.to_string()) {
                Ok(path) => path,
                Err(ret) => return ret,
            };
        let bundle_name = match utils::get_caller_bundle_name() {
            Ok(name) => name,
            Err(ret) => {
                error!("[UnregisterSyncFolderChanges] get bundle name failed, ret={}", @public(ret));
                return E_TRY_AGAIN;
            }
        };
        let index = sync_folder_manager::dentry_hash(path);
        if !CloudDiskSyncFolder::get_instance()
            .get_sync_folder_value_by_index(index)
            .is_some_and(|folder| folder.bundle_name == bundle_name)
        {
            error!("[UnregisterSyncFolderChanges] sync folder is not registered for caller, index={}", @public(index));
            return E_SYNC_FOLDER_NOT_REGISTERED;
        }
        if !CloudDiskServiceCallbackManager::get_instance()
            .unregister_sync_folder_for_changes_map(&bundle_name, index)
        {
            return E_LISTENER_NOT_REGISTERED;
        }
        CloudDiskServiceSyncFolder::unregister_sync_folder_changes(user_id, index);
        info!("[UnregisterSyncFolderChanges] completed, user_id={}, index={}, bundle_name={}",
            @public(user_id), @public(index), @private(&bundle_name));
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
            @private(sync_folder),
            @public(count),
            @public(start_usn),
        );
        if count > GET_SYNC_FOLDER_CHANGE_MAX {
            error!("[GetSyncFolderChanges] invalid count={}", @public(count));
            return E_INVALID_ARG;
        }
        let user_id = utils::get_user_id();
        let path =
            match sync_folder_manager::path_to_physical_path(sync_folder, &user_id.to_string()) {
                Ok(path) => path,
                Err(ret) => return ret,
            };
        let bundle_name = match utils::get_caller_bundle_name() {
            Ok(name) => name,
            Err(ret) => {
                error!("[GetSyncFolderChanges] get bundle name failed, ret={}", @public(ret));
                return E_TRY_AGAIN;
            }
        };
        let index = sync_folder_manager::dentry_hash(path);
        if !CloudDiskSyncFolder::get_instance()
            .get_sync_folder_value_by_index(index)
            .is_some_and(|folder| folder.bundle_name == bundle_name)
        {
            error!("[GetSyncFolderChanges] sync folder is not registered for caller, index={}", @public(index));
            return E_SYNC_FOLDER_NOT_REGISTERED;
        }
        let ret = CloudDiskServiceSyncFolder::get_sync_folder_changes(
            user_id, index, start_usn, count, result,
        );
        info!("[GetSyncFolderChanges] completed, index={}, ret={}, count={}, next_usn={}, is_eof={}",
            @public(index), @public(ret), @public(result.changes_data.len()), @public(result.next_usn), @public(result.is_eof));
        ret
    }

    fn set_file_sync_states_inner(
        &self,
        sync_folder: &str,
        states: &[FileSyncState],
        result: &mut Vec<FailedList>,
    ) -> i32 {
        info!(
            "[set_file_sync_states_inner] sync_folder={}, states_count={}",
            @private(sync_folder),
            @public(states.len()),
        );
        if states.is_empty() || states.len() > GET_FILE_SYNC_MAX {
            error!("[SetFileSyncStates] invalid count={}", @public(states.len()));
            return E_INVALID_ARG;
        }
        let user_id = utils::get_user_id();
        let path =
            match sync_folder_manager::path_to_physical_path(sync_folder, &user_id.to_string()) {
                Ok(path) => path,
                Err(ret) => return ret,
            };
        let bundle_name = match utils::get_caller_bundle_name() {
            Ok(name) => name,
            Err(ret) => {
                error!("[SetFileSyncStates] get bundle name failed, ret={}", @public(ret));
                return E_TRY_AGAIN;
            }
        };
        let index = sync_folder_manager::dentry_hash(&path);
        if !CloudDiskSyncFolder::get_instance()
            .get_sync_folder_value_by_index(index)
            .is_some_and(|folder| folder.bundle_name == bundle_name)
        {
            error!("[SetFileSyncStates] sync folder is not registered for caller, index={}", @public(index));
            return E_SYNC_FOLDER_NOT_REGISTERED;
        }
        for (index, item) in states.iter().enumerate() {
            info!(
                "[set_file_sync_states_inner] states[{}].path={}, state={}",
                @public(index),
                @private(&item.path),
                @public(item.state),
            );
            if let Err(error) = set_file_sync_state(item, user_id, sync_folder) {
                warn!("[SetFileSyncStates] item failed, path={}, error={}", @private(&item.path), @public(error));
                result.push(FailedList {
                    path: item.path.clone(),
                    error,
                });
            }
        }
        info!("[SetFileSyncStates] completed, index={}, failed_count={}", @public(index), @public(result.len()));
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
            @private(sync_folder),
            @public(paths.len()),
        );
        if paths.is_empty() || paths.len() > GET_FILE_SYNC_MAX {
            error!("[GetFileSyncStates] invalid count={}", @public(paths.len()));
            return E_INVALID_ARG;
        }
        let user_id = utils::get_user_id();
        let path =
            match sync_folder_manager::path_to_physical_path(sync_folder, &user_id.to_string()) {
                Ok(path) => path,
                Err(ret) => return ret,
            };
        let bundle_name = match utils::get_caller_bundle_name() {
            Ok(name) => name,
            Err(ret) => {
                error!("[GetFileSyncStates] get bundle name failed, ret={}", @public(ret));
                return E_TRY_AGAIN;
            }
        };
        let index = sync_folder_manager::dentry_hash(&path);
        if !CloudDiskSyncFolder::get_instance()
            .get_sync_folder_value_by_index(index)
            .is_some_and(|folder| folder.bundle_name == bundle_name)
        {
            error!("[GetFileSyncStates] sync folder is not registered for caller, index={}", @public(index));
            return E_SYNC_FOLDER_NOT_REGISTERED;
        }
        for (item_index, path) in paths.iter().enumerate() {
            info!(
                "[get_file_sync_states_inner] paths[{}]={}",
                @public(item_index),
                @private(path),
            );
            let item = get_file_sync_state(path, user_id, sync_folder);
            info!("[GetFileSyncStates] path={}, is_success={}, state={}, error={}",
                @private(path), @public(item.is_success), @public(item.state), @public(item.error));
            result.push(item);
        }
        info!("[GetFileSyncStates] completed, index={}, result_count={}", @public(index), @public(result.len()));
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
        self.cancel_pending_unload();
        let ret = check_permissions(utils::PERM_CLOUD_DISK_SERVICE, true);
        if ret != E_OK {
            error!("[register_sync_folder_inner] stage=permission, user_id={}, ret={}", @public(user_id), @public(ret));
            return ret;
        }

        let register_sync_folder = match sync_folder_manager::path_to_physical_path(
            path,
            &user_id.to_string(),
        ) {
            Ok(physical_path) => physical_path,
            Err(ret) => {
                error!("[register_sync_folder_inner] get physical path failed, ret={}", @public(ret));
                return ret;
            }
        };
        let sync_folder_index = sync_folder_manager::dentry_hash(&register_sync_folder);
        info!(
            "[register_sync_folder_inner] physical_path={}, sync_folder_index={}",
            @private(&register_sync_folder),
            @public(sync_folder_index),
        );

        let ret = CloudDiskServiceSyncFolder::register_sync_folder(
            user_id,
            sync_folder_index,
            &register_sync_folder,
        );
        if ret != E_OK {
            error!("[register_sync_folder_inner] stage=register-cache, user_id={}, index={}, ret={}",
                @public(user_id), @public(sync_folder_index), @public(ret));
            return ret;
        }
        let folder_count = CloudDiskSyncFolder::get_instance().get_sync_folder_size();
        info!("[register_sync_folder_inner] cache ready, user_id={}, index={}, registered_roots={}",
            @public(user_id), @public(sync_folder_index), @public(folder_count));
        if folder_count == 0 {
            // C++ logs monitor setup failures but keeps the registration successful.
            let started = DiskMonitor::get_instance().start_monitor(user_id);
            info!("[register_sync_folder_inner] monitor start result={}, user_id={}", @public(started), @public(user_id));
        } else {
            info!("[register_sync_folder_inner] monitor already requested for existing roots, count={}", @public(folder_count));
        }
        let value = SyncFolderValue {
            bundle_name: bundle_name.to_owned(),
            path: register_sync_folder,
        };
        CloudDiskSyncFolder::get_instance().add_sync_folder(sync_folder_index, value);
        info!("[register_sync_folder_inner] finished, user_id={}, index={}, ret={}",
            @public(user_id), @public(sync_folder_index), @public(E_OK));
        E_OK
    }

    fn unregister_sync_folder_inner(&self, user_id: i32, bundle_name: &str, path: &str) -> i32 {
        info!(
            "[unregister_sync_folder_inner] user_id={}, bundle_name={}, path={}",
            @public(user_id),
            @public(bundle_name),
            @private(path),
        );
        let ret = check_permissions(utils::PERM_CLOUD_DISK_SERVICE, true);
        if ret != E_OK {
            return ret;
        }
        let user = user_id.to_string();
        let physical_path = match sync_folder_manager::path_to_physical_path(path, &user) {
            Ok(path) => path,
            Err(ret) => {
                error!("[unregister_sync_folder_inner] get physical path failed, ret={}", @public(ret));
                return ret;
            }
        };
        let index = sync_folder_manager::dentry_hash(&physical_path);
        let mnt_path = match sync_folder_manager::path_to_mnt_path_by_sandbox_path(path, &user) {
            Ok(path) => path,
            Err(ret) => {
                error!("[unregister_sync_folder_inner] get mnt path failed, ret={}", @public(ret));
                return ret;
            }
        };
        info!("[unregister_sync_folder_inner] index={}, physical_path={}, mnt_path={}",
            @public(index), @private(&physical_path), @private(&mnt_path));
        let ret = CloudDiskServiceSyncFolder::unregister_sync_folder(user_id, index);
        if ret != E_OK {
            error!("[unregister_sync_folder_inner] unregister cache failed, ret={}", @public(ret));
            return ret;
        }
        CloudDiskSyncFolder::get_instance().delete_sync_folder(index);
        CloudDiskServiceCallbackManager::get_instance()
            .unregister_sync_folder_map(bundle_name, index);
        PlaceholderCallbackManager::get_instance().clear_by_sync_folder(bundle_name, index);
        sync_folder_manager::remove_placeholder_files_batch(&mnt_path);
        sync_folder_manager::remove_xattr(
            &mnt_path,
            sync_folder_manager::CLOUD_DISK_FILE_SYNC_STATE_XATTR,
        );
        sync_folder_manager::remove_xattr(
            &mnt_path,
            sync_folder_manager::CLOUD_DISK_PLACEHOLDER_COUNT_XATTR,
        );
        self.unload_sa();
        info!("[unregister_sync_folder_inner] finished, index={}", @public(index));
        E_OK
    }

    fn unregister_for_sa_inner(&self, path: &str) -> i32 {
        info!(
            "[unregister_for_sa_inner] path={}",
            @private(path),
        );
        let mut user_id = utils::get_user_id();
        if user_id == 0 {
            // C++ uses the output user ID without propagating GetAccountId's status.
            let _ = utils::get_account_id(&mut user_id);
        }
        let user = user_id.to_string();
        let sandbox_path =
            match sync_folder_manager::path_to_sandbox_path_by_physical_path(path, &user) {
                Ok(path) => path,
                Err(ret) => return ret,
            };
        info!("[unregister_for_sa_inner] user_id={}, sandbox_path={}", @public(user_id), @private(&sandbox_path));
        let ret = ffi::sync_folder_manager::unregister_for_sa(&sandbox_path);
        if ret != E_OK {
            error!("[unregister_for_sa_inner] manager unregister failed, ret={}", @public(ret));
            return ret;
        }
        let mnt_path = match sync_folder_manager::path_to_mnt_path_by_physical_path(path, &user) {
            Ok(path) => path,
            Err(ret) => return ret,
        };
        sync_folder_manager::remove_placeholder_files_batch(&mnt_path);
        sync_folder_manager::remove_xattr(
            &mnt_path,
            sync_folder_manager::CLOUD_DISK_FILE_SYNC_STATE_XATTR,
        );
        sync_folder_manager::remove_xattr(
            &mnt_path,
            sync_folder_manager::CLOUD_DISK_PLACEHOLDER_COUNT_XATTR,
        );
        let index = sync_folder_manager::dentry_hash(path);
        let Some(value) = CloudDiskSyncFolder::get_instance().get_sync_folder_value_by_index(index)
        else {
            error!("[unregister_for_sa_inner] sync folder not registered, index={}", @public(index));
            return E_SYNC_FOLDER_NOT_REGISTERED;
        };
        CloudDiskSyncFolder::get_instance().delete_sync_folder(index);
        CloudDiskServiceCallbackManager::get_instance()
            .unregister_sync_folder_map(&value.bundle_name, index);
        PlaceholderCallbackManager::get_instance().clear_by_sync_folder(&value.bundle_name, index);
        // As in C++, the invalid-root event caller handles log-cache removal.
        self.unload_sa();
        info!("[unregister_for_sa_inner] finished, index={}, bundle_name={}", @public(index), @private(&value.bundle_name));
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
