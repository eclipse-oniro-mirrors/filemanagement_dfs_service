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

//! Account subscription and user switching, corresponding to account_status_listener.cpp.

use std::sync::Mutex;

use cxx::UniquePtr;
use samgr::manage::SystemAbilityManager;

use crate::common::error_codes::E_OK;
use crate::ffi::{account, sync_folder_manager as manager_ffi};
use crate::framework::CLOUD_DISK_SERVICE_SA_ID;
use crate::monitor::DiskMonitor;
use crate::placeholder::{
    PlaceholderCallbackManager, PlaceholderProgressManager, PlaceholderTaskCancelReason,
    PlaceholderTaskManager,
};
use crate::sync_folder_manager::{self, CloudDiskSyncFolder, SyncFolderValue};
use crate::sync_folder_service::{CloudDiskServiceCallbackManager, CloudDiskServiceSyncFolder};

struct ListenerState {
    user_id: Option<i32>,
    subscription: UniquePtr<account::AccountStatusSubscription>,
}

pub(crate) struct AccountStatusListener {
    state: Mutex<ListenerState>,
}

impl AccountStatusListener {
    pub(crate) fn new() -> Self {
        Self {
            state: Mutex::new(ListenerState {
                user_id: None,
                subscription: UniquePtr::null(),
            }),
        }
    }

    pub(crate) fn prepare(&self, user_id: i32) {
        self.state
            .lock()
            .unwrap_or_else(|err| err.into_inner())
            .user_id = Some(user_id);
    }

    pub(crate) fn start(&self) {
        let mut state = self.state.lock().unwrap_or_else(|err| err.into_inner());
        let Some(user_id) = state.user_id else {
            return; // Ignore late SA notifications after OnStop.
        };
        if !state.subscription.is_null() {
            return;
        }
        let callback = Box::new(AccountStatusCallback {
            current_user_id: Mutex::new(Some(user_id)),
        });
        let mut ret = E_OK;
        state.subscription = account::subscribe_account_status(callback, &mut ret);
        if ret != E_OK || state.subscription.is_null() {
            error!("[AccountStatusListener] subscribe failed, user_id={}, ret={}",
                @public(user_id), @public(ret));
        } else {
            info!("[AccountStatusListener] subscribed, user_id={}", @public(user_id));
        }
    }

    pub(crate) fn stop(&self) {
        let mut state = self.state.lock().unwrap_or_else(|err| err.into_inner());
        state.user_id = None;
        if !state.subscription.is_null() {
            let ret = state.subscription.pin_mut().stop();
            state.subscription = UniquePtr::null();
            if ret == E_OK {
                info!("[AccountStatusListener] unsubscribed");
            } else {
                warn!("[AccountStatusListener] unsubscribe failed, ret={}", @public(ret));
            }
        }
    }
}

// CXX owns this callback through the native subscriber. It contains no borrowed
// service references, and its mutex serializes callbacks with deactivation.
pub struct AccountStatusCallback {
    current_user_id: Mutex<Option<i32>>,
}

impl AccountStatusCallback {
    pub(crate) fn deactivate(&self) {
        *self
            .current_user_id
            .lock()
            .unwrap_or_else(|err| err.into_inner()) = None;
    }

    pub(crate) fn on_state_changed(&self, switched: bool, user_id: i32) {
        let unload = {
            let mut current = self
                .current_user_id
                .lock()
                .unwrap_or_else(|err| err.into_inner());
            let Some(current_user_id) = *current else {
                return;
            };
            info!("[AccountStatusListener] switched={}, user_id={}, current_user_id={}",
                @public(switched), @public(user_id), @public(current_user_id));
            let unload = if switched {
                *current = Some(user_id);
                Self::handle_user_switched(user_id)
            } else {
                user_id == current_user_id
            };
            if unload {
                DiskMonitor::get_instance().stop_monitor();
            }
            unload
        };
        // Do not hold the callback mutex across SAMgr IPC: OnStop deactivates us.
        if unload {
            let ret = SystemAbilityManager::unload_system_ability(CLOUD_DISK_SERVICE_SA_ID);
            info!("[AccountStatusListener] unload requested, ret={}", @public(ret));
        }
    }

    fn handle_user_switched(user_id: i32) -> bool {
        // Clear the registry first so the monitor cannot reopen an old user's cache.
        CloudDiskSyncFolder::get_instance().clear_map();
        CloudDiskServiceSyncFolder::cloud_disk_service_clear_all();
        DiskMonitor::get_instance().stop_monitor();
        PlaceholderCallbackManager::get_instance().clear_all();
        PlaceholderTaskManager::get_instance()
            .cancel_all_tasks(PlaceholderTaskCancelReason::UserSwitch);
        PlaceholderTaskManager::get_instance().clear_cancellation_records();
        PlaceholderProgressManager::get_instance().drain();
        PlaceholderProgressManager::get_instance().clear();
        CloudDiskServiceCallbackManager::get_instance().clear_map();
        let folders = match manager_ffi::get_all_sync_folders_for_sa() {
            Ok(folders) => folders,
            Err(ret) => {
                error!("[HandleUserSwitched] get sync folders failed, ret={}", @public(ret));
                return true;
            }
        };
        let user = user_id.to_string();
        for folder in folders {
            let path = match sync_folder_manager::path_to_physical_path(&folder.path, &user) {
                Ok(path) => path,
                Err(ret) => {
                    warn!("[HandleUserSwitched] invalid sync folder, path={}, ret={}",
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
        let count = CloudDiskSyncFolder::get_instance().get_sync_folder_size();
        info!("[HandleUserSwitched] user_id={}, sync_folder_count={}", @public(user_id), @public(count));
        if count == 0 {
            return true;
        }
        DiskMonitor::get_instance().start_monitor(user_id);
        false
    }
}
