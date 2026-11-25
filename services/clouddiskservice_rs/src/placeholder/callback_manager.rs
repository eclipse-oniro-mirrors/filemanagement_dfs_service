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

//! Provider callback tables, keyed by bundle name and registered sync root.

use std::collections::HashMap;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex, OnceLock};

use ipc::remote::RemoteObj;

use super::{PlaceholderTaskCancelReason, PlaceholderTaskManager};
use crate::common::error_codes::{
    E_CALLBACK_ALREADY_REGISTERED, E_CALLBACK_NOT_REGISTERED, E_INVALID_ARG, E_OK,
};
use crate::ffi::callback_remote::CallbackRemote;
use crate::framework::CallbackRequest;

type CallbackKey = (String, u32);

pub(super) struct CallbackEntry {
    remote_id: usize,
    object: RemoteObj,
    active: AtomicBool,
    dispatch: Mutex<()>,
}

impl CallbackEntry {
    pub(super) fn is_registered(&self) -> bool {
        self.active.load(Ordering::Acquire)
    }

    pub(super) fn send(
        &self,
        request: CallbackRequest,
        folder: &str,
        path: &str,
    ) -> Result<(), i32> {
        let _dispatch = self.dispatch.lock().unwrap_or_else(|err| err.into_inner());
        if !self.is_registered() {
            return Err(E_CALLBACK_NOT_REGISTERED);
        }
        request.send(&self.object, folder, path)
    }

    fn deactivate(&self) {
        self.active.store(false, Ordering::Release);
    }

    fn wait_for_dispatch(&self) {
        drop(self.dispatch.lock().unwrap_or_else(|err| err.into_inner()));
    }
}

struct RegisteredRemote {
    callback: CallbackRemote,
    folders: Vec<CallbackKey>,
}

#[derive(Default)]
struct CallbackTables {
    by_folder: HashMap<CallbackKey, Arc<CallbackEntry>>,
    by_remote: HashMap<usize, RegisteredRemote>,
}

pub(crate) struct PlaceholderCallbackManager {
    tables: Mutex<CallbackTables>,
}

impl PlaceholderCallbackManager {
    pub(crate) fn get_instance() -> &'static Self {
        static INSTANCE: OnceLock<PlaceholderCallbackManager> = OnceLock::new();
        INSTANCE.get_or_init(|| Self {
            tables: Mutex::default(),
        })
    }

    pub(crate) fn register_callback_table(
        &self,
        bundle_name: &str,
        index: u32,
        mut remote: CallbackRemote,
    ) -> i32 {
        if bundle_name.is_empty() {
            return E_INVALID_ARG;
        }
        let id = remote.resolve_table_key();
        if id == 0 {
            error!("[RegisterCallbackTable] invalid callback table interface, index={}", @public(index));
            return E_INVALID_ARG;
        }
        let key = (bundle_name.to_owned(), index);
        let mut tables = self.tables.lock().unwrap_or_else(|err| err.into_inner());
        if tables.by_folder.contains_key(&key) {
            warn!("[RegisterCallbackTable] already registered, index={}", @public(index));
            return E_CALLBACK_ALREADY_REGISTERED;
        }
        let Some(object) = remote.object() else {
            return E_INVALID_ARG;
        };
        let entry = Arc::new(CallbackEntry {
            remote_id: id,
            object,
            active: AtomicBool::new(true),
            dispatch: Mutex::new(()),
        });
        let provider = tables
            .by_remote
            .entry(id)
            .or_insert_with(|| RegisteredRemote {
                callback: remote,
                folders: Vec::new(),
            });
        // A failed watch does not fail registration. The next root for this
        // Binder object retries, as in AddDeathRecipientLocked.
        if !provider.callback.watch_death(Self::on_remote_died) {
            warn!("[RegisterCallbackTable] could not add death recipient, index={}", @public(index));
        }
        provider.folders.push(key.clone());
        tables.by_folder.insert(key, entry);
        E_OK
    }

    pub(super) fn callback(
        &self,
        bundle_name: &str,
        index: u32,
    ) -> Result<Arc<CallbackEntry>, i32> {
        self.tables
            .lock()
            .unwrap_or_else(|err| err.into_inner())
            .by_folder
            .get(&(bundle_name.to_owned(), index))
            .cloned()
            .ok_or(E_CALLBACK_NOT_REGISTERED)
    }

    pub(crate) fn is_callback_registered(&self, bundle_name: &str, index: u32) -> bool {
        self.callback(bundle_name, index).is_ok()
    }

    pub(crate) fn unregister_callback_table(&self, bundle_name: &str, index: u32) -> i32 {
        if bundle_name.is_empty() {
            return E_INVALID_ARG;
        }
        if !self.remove_folder(bundle_name, index) {
            warn!("[UnregisterCallbackTable] no callback table, index={}", @public(index));
            return E_CALLBACK_NOT_REGISTERED;
        }
        E_OK
    }

    pub(crate) fn clear_by_sync_folder(&self, bundle_name: &str, index: u32) {
        self.remove_folder(bundle_name, index);
    }

    fn remove_folder(&self, bundle_name: &str, index: u32) -> bool {
        let key = (bundle_name.to_owned(), index);
        let (entry, last_registration) = {
            let mut tables = self.tables.lock().unwrap_or_else(|err| err.into_inner());
            let Some(entry) = tables.by_folder.remove(&key) else {
                return false;
            };
            entry.deactivate();
            let id = entry.remote_id;
            let last = tables.by_remote.get_mut(&id).is_some_and(|provider| {
                provider.folders.retain(|folder| folder != &key);
                provider.folders.is_empty()
            });
            let last_registration = if last {
                tables.by_remote.remove(&id)
            } else {
                None
            };
            (entry, last_registration)
        };
        drop(last_registration);
        entry.wait_for_dispatch();
        PlaceholderTaskManager::get_instance()
            .cancel_tasks_for_callback(&entry, PlaceholderTaskCancelReason::Unregister);
        true
    }

    fn on_remote_died(id: usize) {
        let manager = Self::get_instance();
        let (provider, entries) = {
            let mut tables = manager.tables.lock().unwrap_or_else(|err| err.into_inner());
            let Some(provider) = tables.by_remote.remove(&id) else {
                return;
            };
            let mut entries = Vec::with_capacity(provider.folders.len());
            for key in &provider.folders {
                if let Some(entry) = tables.by_folder.remove(key) {
                    entry.deactivate();
                    entries.push(entry);
                }
            }
            (provider, entries)
        };
        drop(provider);
        warn!("[CallbackRemoteDied] removed callback tables, count={}", @public(entries.len()));
        for entry in entries {
            entry.wait_for_dispatch();
            PlaceholderTaskManager::get_instance()
                .cancel_tasks_for_callback(&entry, PlaceholderTaskCancelReason::CallbackDied);
        }
    }

    pub(crate) fn clear_all(&self) {
        let tables = {
            let mut tables = self.tables.lock().unwrap_or_else(|err| err.into_inner());
            for entry in tables.by_folder.values() {
                entry.deactivate();
            }
            std::mem::take(&mut *tables)
        };
        info!("[PlaceholderCallbackManager::ClearAll] removed callback tables, count={}",
            @public(tables.by_folder.len()));
        for entry in tables.by_folder.values() {
            entry.wait_for_dispatch();
        }
    }
}
