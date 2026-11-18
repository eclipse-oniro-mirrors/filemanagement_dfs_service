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
use std::os::fd::OwnedFd;
use std::os::unix::{ffi::OsStrExt, fs::MetadataExt};
use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex, OnceLock};

use self::fanotify::Fanotify;
use crate::common::error_codes::E_OK;
use crate::ffi::ffrt;
use crate::service::CloudDiskService;
use crate::sync_folder_manager::CloudDiskSyncFolder;
use crate::sync_folder_service::{CloudDiskServiceLogFile, LogFileMgr};

const BLOCKED_PREFIXES: [&[u8]; 4] = [b"/.Trash", b"/.Recent", b"/.thumbs", b"/HO_DATA_EXT_MISC"];

struct Worker {
    running: Arc<AtomicBool>,
    stop_writer: OwnedFd,
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
        let mut worker = loop {
            let mut worker = self.worker.lock().unwrap_or_else(|err| err.into_inner());
            if CloudDiskSyncFolder::get_instance().get_sync_folder_size() == 0 {
                return false;
            }
            if worker
                .as_ref()
                .is_some_and(|w| w.running.load(Ordering::Acquire))
            {
                info!("[StartMonitor] monitor is already running");
                return false;
            }
            let Some(old) = worker.take() else {
                break worker;
            };
            drop(worker);
            Self::stop_worker(Some(old));
        };
        let (events, stop_writer) = match Fanotify::new() {
            Ok(monitor) => monitor,
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
        *worker = Some(Worker {
            running,
            stop_writer,
            task,
        });
        info!("[StartMonitor] started, user_id={}", @public(user_id));
        true
    }

    pub(crate) fn stop_monitor(&self) {
        let worker = self
            .worker
            .lock()
            .unwrap_or_else(|err| err.into_inner())
            .take();
        Self::stop_worker(worker);
    }

    pub(crate) fn stop_if_idle(&self) -> bool {
        let mut worker = self.worker.lock().unwrap_or_else(|err| err.into_inner());
        let count = CloudDiskSyncFolder::get_instance().get_sync_folder_size();
        info!("[UnloadSa] sync_folder_count={}", @public(count));
        if count != 0 {
            return false;
        }
        let old = worker.take();
        drop(worker);
        Self::stop_worker(old);
        true
    }

    fn unload_if_idle_from_worker(&self, running: &AtomicBool) {
        let worker = self.worker.lock().unwrap_or_else(|err| err.into_inner());
        if worker
            .as_ref()
            .is_some_and(|w| std::ptr::eq(w.running.as_ref(), running))
            && CloudDiskSyncFolder::get_instance().get_sync_folder_size() == 0
        {
            // Signal our own exit. Queue the unload before OnStop can take this worker
            // and perform its final cancellation. Other callers can then safely join us.
            running.store(false, Ordering::Release);
            CloudDiskService::get_instance().delay_unload_sa();
        }
    }

    fn stop_worker(worker: Option<Worker>) {
        if let Some(worker) = worker {
            worker.running.store(false, Ordering::Release);
            // Closing the only writer wakes poll even if the collector has not entered it yet.
            drop(worker.stop_writer);
            worker.task.wait();
            info!("[StopMonitor] stopped");
        }
    }
}

fn collect_events(mut events: Fanotify, user_id: i32, running: &AtomicBool) {
    let mut buffer = [0; 4096];
    let mut changes = Changes::default();
    let docs = format!("/data/service/el2/{user_id}/hmdfs/account/files/Docs");
    while running.load(Ordering::Acquire) {
        let len = match events.read(&mut buffer) {
            Ok(0) => break,
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
            let tracked = match_folder(user_id, docs.as_bytes(), path);
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

fn match_folder(user_id: i32, docs: &[u8], path: PathBuf) -> Option<TrackedPath> {
    let suffix = path.as_os_str().as_bytes().strip_prefix(docs)?;
    if BLOCKED_PREFIXES
        .iter()
        .any(|prefix| suffix.starts_with(prefix))
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
        let Some(path) = self.path.to_str() else {
            return;
        };
        let logs = LogFileMgr::get_instance();
        let Some(guard) = logs.begin_invalid_root(&self.file) else {
            return;
        };
        self.file.notify_sync_folder_invalid();
        // Fanotify already knows the owning user. This task has no IPC caller identity.
        let service = CloudDiskService::get_instance();
        let ret = service.unregister_for_sa(path, self.user_id);
        if ret != E_OK {
            error!("[DiskMonitor] unregister invalid root failed, index={}, ret={}", @public(self.index), @public(ret));
            return;
        }
        let ret = guard.retire_cache();
        if ret != E_OK {
            error!("[DiskMonitor] retire invalid root cache failed, index={}, ret={}", @public(self.index), @public(ret));
        }
        DiskMonitor::get_instance().unload_if_idle_from_worker(running);
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
