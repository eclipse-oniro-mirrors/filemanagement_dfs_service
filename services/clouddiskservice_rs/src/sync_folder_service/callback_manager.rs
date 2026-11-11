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

//! CloudDiskServiceCallbackManager: per-app death listeners and per-root callbacks.

use std::collections::{HashMap, HashSet};
use std::sync::{Arc, Mutex, OnceLock, Weak};

use ipc::remote::RemoteObj;

use super::CloudDiskServiceSyncFolder;
use crate::common::types::ChangeData;
use crate::framework::CloudDiskServiceCallbackProxy;
use crate::sync_folder_manager::{self, CloudDiskSyncFolder};

#[derive(Default)]
struct CallbackValue {
    callback: Option<Arc<CloudDiskServiceCallbackProxy>>,
    indices: HashSet<u32>,
}

struct IndexCallback {
    user_id: i32,
    callback: Arc<CloudDiskServiceCallbackProxy>,
}

#[derive(Default)]
struct CallbackMaps {
    apps: HashMap<String, CallbackValue>,
    indices: HashMap<u32, IndexCallback>,
}

pub(crate) struct CloudDiskServiceCallbackManager {
    maps: Mutex<CallbackMaps>,
}

impl CloudDiskServiceCallbackManager {
    pub(crate) fn get_instance() -> &'static Self {
        static INSTANCE: OnceLock<CloudDiskServiceCallbackManager> = OnceLock::new();
        INSTANCE.get_or_init(|| Self {
            maps: Mutex::new(CallbackMaps::default()),
        })
    }

    pub(crate) fn register_sync_folder_map(
        &self,
        bundle_name: &str,
        index: u32,
        remote: &RemoteObj,
        user_id: i32,
    ) -> bool {
        let mut maps = self.maps.lock().unwrap_or_else(|err| err.into_inner());
        if maps.indices.contains_key(&index) {
            warn!("[RegisterSyncFolderMap] listener already registered, index={}", @public(index));
            return false;
        }
        maps.indices.insert(
            index,
            IndexCallback {
                user_id,
                callback: Arc::new(CloudDiskServiceCallbackProxy::new(remote)),
            },
        );
        maps.apps
            .entry(bundle_name.to_owned())
            .or_default()
            .indices
            .insert(index);
        info!("[RegisterSyncFolderMap] bundle_name={}, user_id={}, index={}",
            @private(bundle_name), @public(user_id), @public(index));
        true
    }

    pub(crate) fn add_callback(&self, bundle_name: &str, remote: &RemoteObj) {
        let callback = {
            let mut maps = self.maps.lock().unwrap_or_else(|err| err.into_inner());
            let Some(app) = maps.apps.get_mut(bundle_name) else {
                return;
            };
            if app.callback.is_some() {
                return;
            }
            let callback = Arc::new(CloudDiskServiceCallbackProxy::new(remote));
            app.callback = Some(Arc::clone(&callback));
            callback
        };
        let bundle = bundle_name.to_owned();
        let weak = Arc::downgrade(&callback);
        // As in C++, the remote owns the death recipient. This SDK's optional
        // removal handle does not report registration success reliably; dropping
        // it does not remove the recipient. Do not use it to decide API success.
        let _ = callback.remote.add_death_recipient(move |_| {
            Self::get_instance().on_remote_died(&bundle, &weak);
        });
        if callback.remote.is_dead() {
            self.on_remote_died(bundle_name, &Arc::downgrade(&callback));
        }
        info!("[AddCallback] bundle_name={}", @private(bundle_name));
    }

    fn on_remote_died(&self, bundle_name: &str, dead: &Weak<CloudDiskServiceCallbackProxy>) {
        let removed = {
            let mut maps = self.maps.lock().unwrap_or_else(|err| err.into_inner());
            let Some(current) = maps
                .apps
                .get(bundle_name)
                .and_then(|app| app.callback.as_ref())
            else {
                return;
            };
            if !Weak::ptr_eq(&Arc::downgrade(current), dead) {
                return; // A late death notice must not remove a newly registered app.
            }
            let app = maps.apps.remove(bundle_name).unwrap();
            app.indices
                .into_iter()
                .filter_map(|index| {
                    maps.indices
                        .remove(&index)
                        .map(|entry| (entry.user_id, index))
                })
                .collect::<Vec<_>>()
        };
        info!("[OnRemoteDied] bundle_name={}, listener_count={}", @private(bundle_name), @public(removed.len()));
        for (user_id, index) in removed {
            CloudDiskServiceSyncFolder::unregister_sync_folder_changes(user_id, index);
        }
    }

    pub(crate) fn unregister_sync_folder_for_changes_map(
        &self,
        bundle_name: &str,
        index: u32,
    ) -> bool {
        let mut maps = self.maps.lock().unwrap_or_else(|err| err.into_inner());
        if maps.indices.remove(&index).is_none() {
            warn!("[UnregisterSyncFolderForChangesMap] listener not registered, index={}", @public(index));
            return false;
        }
        let Some(app) = maps.apps.get_mut(bundle_name) else {
            return false;
        };
        app.indices.remove(&index);
        info!("[UnregisterSyncFolderForChangesMap] bundle_name={}, index={}", @private(bundle_name), @public(index));
        true
    }

    pub(crate) fn unregister_sync_folder_map(&self, bundle_name: &str, index: u32) {
        let removed = {
            let mut maps = self.maps.lock().unwrap_or_else(|err| err.into_inner());
            let removed = maps.indices.remove(&index);
            if let Some(app) = maps.apps.get_mut(bundle_name) {
                app.indices.remove(&index);
            }
            removed
        };
        if let Some(entry) = removed {
            CloudDiskServiceSyncFolder::unregister_sync_folder_changes(entry.user_id, index);
        }
        info!("[UnregisterSyncFolderMap] bundle_name={}, index={}", @private(bundle_name), @public(index));
    }

    pub(crate) fn on_change_data(&self, index: u32, changes: &[ChangeData]) {
        let Some((callback, sandbox)) = self.callback_for_index(index) else {
            return;
        };
        if let Err(err) = callback.on_change_data(&sandbox, changes) {
            error!("[OnChangeData] callback failed, index={}, error={}", @public(index), @public(format!("{err:?}")));
        }
    }

    pub(super) fn callback_for_index(
        &self,
        index: u32,
    ) -> Option<(Arc<CloudDiskServiceCallbackProxy>, String)> {
        let (user_id, callback) = {
            let maps = self.maps.lock().unwrap_or_else(|err| err.into_inner());
            let entry = maps.indices.get(&index)?;
            (entry.user_id, Arc::clone(&entry.callback))
        };
        let root = CloudDiskSyncFolder::get_instance().get_sync_folder_value_by_index(index)?;
        let sandbox = sync_folder_manager::path_to_sandbox_path_by_physical_path(
            &root.path,
            &user_id.to_string(),
        )
        .ok()?;
        Some((callback, sandbox))
    }

    pub(crate) fn clear_map(&self) {
        let maps = {
            let mut maps = self.maps.lock().unwrap_or_else(|err| err.into_inner());
            std::mem::take(&mut *maps)
        };
        info!("[CloudDiskServiceCallbackManager::ClearMap] listener_count={}", @public(maps.indices.len()));
        for (index, entry) in maps.indices {
            CloudDiskServiceSyncFolder::unregister_sync_folder_changes(entry.user_id, index);
        }
    }
}
