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

//! Fanotify event routing corresponding to disk_monitor.cpp.

mod fanotify;

use std::collections::HashSet;
use std::fs;
use std::io;
use std::os::unix::{ffi::OsStrExt, fs::MetadataExt};
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex, OnceLock};

use self::fanotify::Fanotify;
use crate::common::error_codes::E_OK;
use crate::ffi::{self, ffrt};
use crate::sync_folder_manager::CloudDiskSyncFolder;
use crate::sync_folder_service::{
    CloudDiskServiceCallbackManager, CloudDiskServiceLogFile, LogFileMgr,
};

struct Worker {
    running: Arc<AtomicBool>,
    task: ffrt::Task,
}

pub(crate) struct DiskMonitor {
    worker: Mutex<Option<Worker>>,
}

impl DiskMonitor {
    pub(crate) fn get_instance() -> &'static Self {
        static INSTANCE: OnceLock<DiskMonitor> = OnceLock::new();
        INSTANCE.get_or_init(|| Self {
            worker: Mutex::new(None),
        })
    }

    pub(crate) fn start_monitor(&self, user_id: i32) -> bool {
        let mut worker = self.worker.lock().unwrap_or_else(|err| err.into_inner());
        if worker
            .as_ref()
            .is_some_and(|worker| worker.running.load(Ordering::Acquire))
        {
            info!("[StartMonitor] monitor is already running");
            return false;
        }
        if let Some(old) = worker.take() {
            old.task.wait();
        }
        let events = match Fanotify::new() {
            Ok(events) => events,
            Err(err) => {
                error!("[StartMonitor] fanotify setup failed, user_id={}, error={}", @public(user_id), @public(err));
                return false;
            }
        };
        let running = Arc::new(AtomicBool::new(true));
        let task_running = Arc::clone(&running);
        let task = ffrt::spawn(move || {
            collect_events(events, user_id, &task_running);
            task_running.store(false, Ordering::Release);
            info!("[DiskMonitor] worker exited, user_id={}", @public(user_id));
        });
        *worker = Some(Worker { running, task });
        info!("[StartMonitor] started, user_id={}", @public(user_id));
        true
    }

    pub(crate) fn stop_monitor(&self) {
        let worker = self
            .worker
            .lock()
            .unwrap_or_else(|err| err.into_inner())
            .take();
        if let Some(worker) = worker {
            worker.running.store(false, Ordering::Release);
            // The worker owns both descriptors; it drops them before signalling completion.
            worker.task.wait();
            info!("[StopMonitor] stopped");
        }
    }
}

fn collect_events(mut events: Fanotify, user_id: i32, running: &AtomicBool) {
    let mut buffer = [0; 4096];
    let mut changes = Changes::default();
    while running.load(Ordering::Acquire) {
        let len = match events.read(&mut buffer) {
            Ok(len) => len,
            Err(err)
                if matches!(
                    err.kind(),
                    io::ErrorKind::Interrupted | io::ErrorKind::WouldBlock
                ) =>
            {
                continue
            }
            Err(err) => {
                error!("[DiskMonitor] read failed, error={}", @public(err));
                break;
            }
        };
        for (mask, path) in events.events(&buffer[..len]) {
            if !running.load(Ordering::Acquire) {
                break;
            }
            let tracked = match_folder(user_id, path);
            if let Some(item) = &tracked {
                info!("[DiskMonitorEvent] user_id={}, index={}, mask={}, path={}",
                    @public(user_id), @public(item.index), @public(mask), @private(item.path.display()));
            }
            changes.process(mask, tracked, running);
        }
    }
}

struct TrackedPath {
    user_id: i32,
    index: u32,
    root: PathBuf,
    path: PathBuf,
    file: Arc<CloudDiskServiceLogFile>,
}

fn match_folder(user_id: i32, path: PathBuf) -> Option<TrackedPath> {
    let docs = format!("/data/service/el2/{user_id}/hmdfs/account/files/Docs");
    let bytes = path.as_os_str().as_bytes();
    if !bytes.starts_with(docs.as_bytes())
        || ["/.Trash", "/.Recent", "/.thumbs", "/HO_DATA_EXT_MISC"]
            .iter()
            .any(|blocked| bytes.starts_with(format!("{docs}{blocked}").as_bytes()))
    {
        return None;
    }
    for (index, value) in CloudDiskSyncFolder::get_instance().get_sync_folder_map() {
        let root = PathBuf::from(value.path);
        if index != 0 && path.starts_with(&root) {
            let file = LogFileMgr::get_instance().get_log_file(user_id, index)?;
            return Some(TrackedPath {
                user_id,
                index,
                root,
                path,
                file,
            });
        }
    }
    None
}

impl TrackedPath {
    fn is_root(&self) -> bool {
        self.path == self.root
    }

    fn create(&self) {
        if !self.is_root() {
            self.file.record_create(&self.path);
        }
    }

    fn remove(&self, running: &AtomicBool) {
        if !self.is_root() {
            self.file.record_delete(&self.path);
            return;
        }
        let docs = PathBuf::from(format!(
            "/data/service/el2/{}/hmdfs/account/files/Docs",
            self.user_id
        ));
        let Ok(relative) = self.path.strip_prefix(&docs) else {
            return;
        };
        let sandbox = Path::new("/storage/Users/currentUser").join(relative);
        let Some(sandbox) = sandbox.to_str() else {
            return;
        };
        self.file.notify_sync_folder_invalid();
        let folder = CloudDiskSyncFolder::get_instance().get_sync_folder_value_by_index(self.index);
        let ret = ffi::sync_folder_manager::unregister_for_sa(sandbox);
        if ret != E_OK {
            error!("[DiskMonitor] unregister invalid root failed, index={}, ret={}", @public(self.index), @public(ret));
            return;
        }
        CloudDiskSyncFolder::get_instance().delete_sync_folder(self.index);
        if let Some(folder) = folder {
            CloudDiskServiceCallbackManager::get_instance()
                .unregister_sync_folder_map(&folder.bundle_name, self.index);
        }
        LogFileMgr::get_instance().unregister_sync_folder(self.user_id, self.index);
        if CloudDiskSyncFolder::get_instance().get_sync_folder_size() == 0 {
            // Exit this worker directly; waiting on its own task would deadlock.
            running.store(false, Ordering::Release);
        }
        info!("[DiskMonitor] invalid root removed, index={}", @public(self.index));
    }

    fn inode(&self) -> Option<u64> {
        match fs::metadata(&self.path) {
            Ok(stat) => Some(stat.ino()),
            Err(err) => {
                error!("[DiskMonitor] stat failed, path={}, error={}", @private(self.path.display()), @public(err));
                None
            }
        }
    }
}

#[derive(Default)]
struct Changes {
    moved_from: Option<TrackedPath>,
    modified_files: HashSet<u64>,
}

impl Changes {
    fn process(&mut self, mask: u64, tracked: Option<TrackedPath>, running: &AtomicBool) {
        if mask & libc::FAN_CREATE != 0 {
            if let Some(item) = tracked {
                item.create();
            }
        } else if mask & libc::FAN_DELETE != 0 {
            if let Some(item) = tracked {
                item.remove(running);
            }
        } else if mask & libc::FAN_MOVED_FROM != 0 {
            self.moved_from = tracked;
        } else if mask & libc::FAN_MOVED_TO != 0 {
            self.move_to(tracked, running);
        } else if let Some(item) = tracked {
            if mask & libc::FAN_CLOSE_WRITE != 0 {
                if let Some(inode) = item.inode() {
                    let modified = self.modified_files.remove(&inode);
                    item.file.record_close_write(&item.path, modified);
                }
            } else if mask & libc::FAN_MODIFY != 0 {
                if let Some(inode) = item.inode() {
                    self.modified_files.insert(inode);
                }
            }
        }
    }

    fn move_to(&mut self, new: Option<TrackedPath>, running: &AtomicBool) {
        let old = self.moved_from.take();
        if let (Some(old), Some(new)) = (&old, &new) {
            if !old.is_root() && old.index == new.index {
                old.file.record_move(&old.path, &new.path);
                return;
            }
        }
        if let Some(old) = old {
            old.remove(running);
        }
        if let Some(new) = new {
            new.create();
        }
    }
}
