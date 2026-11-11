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

use std::collections::{HashMap, HashSet};
use std::fs;
use std::io;
use std::os::unix::fs::MetadataExt;
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex, OnceLock};
use std::time::{SystemTime, UNIX_EPOCH};

use cxx::UniquePtr;

use super::cache::FolderCache;
use super::metafile::CloudDiskServiceMetaFile;
use super::CloudDiskServiceCallbackManager;
use crate::common::error_codes::E_OK;
use crate::common::types::ChangesResult;
use crate::ffi::ffrt;
use crate::sync_folder_manager::CloudDiskSyncFolder;

const CHANGE_DATA_TIMEOUT_MS: u64 = 1_000;

struct LogFileState {
    path: PathBuf,
    cache: FolderCache,
}

pub(crate) struct CloudDiskServiceLogFile {
    user_id: i32,
    index: u32,
    active: AtomicBool,
    state: Mutex<LogFileState>,
}

impl CloudDiskServiceLogFile {
    fn open(user_id: i32, index: u32, path: &Path) -> io::Result<Self> {
        let directory = cache_dir(user_id, index);
        info!("[LogFileMgr] open, user_id={}, index={}, cache_path={}",
            @public(user_id), @public(index), @private(directory.display()));
        // Opening an existing cache must preserve its root record ID and history.
        // Only explicit registration initializes the root metadata.
        let cache = FolderCache::open(directory)?;
        info!("[LogFileMgr] history ready, user_id={}, index={}", @public(user_id), @public(index));
        Ok(Self {
            user_id,
            index,
            active: AtomicBool::new(true),
            state: Mutex::new(LogFileState {
                path: path.to_path_buf(),
                cache,
            }),
        })
    }

    fn register_root(&self, path: &Path) -> io::Result<()> {
        info!("[RegisterSyncFolder][root-stat] begin, user_id={}, index={}, sync_folder={}",
            @public(self.user_id), @public(self.index), @private(path.display()));
        let stat = fs::metadata(path).map_err(|err| {
            error!("[RegisterSyncFolder][root-stat] failed, user_id={}, index={}, sync_folder={}, errno={:?}, error={}",
                @public(self.user_id), @public(self.index), @private(path.display()),
                @public(err.raw_os_error()), @public(&err));
            err
        })?;
        info!("[RegisterSyncFolder][root-stat] inode={}, mode={:o}, is_dir={}, uid={}, gid={}",
            @public(stat.ino()), @public(stat.mode()), @public(stat.is_dir()), @public(stat.uid()), @public(stat.gid()));
        let mut state = self.state.lock().unwrap_or_else(|err| err.into_inner());
        let directory = cache_dir(self.user_id, self.index);
        info!("[RegisterSyncFolder][root-meta] open, user_id={}, index={}, inode={:016x}, cache_path={}",
            @public(self.user_id), @public(self.index), @public(stat.ino()), @private(directory.display()));
        let meta = CloudDiskServiceMetaFile::open(&directory, stat.ino()).map_err(|err| {
            error!("[RegisterSyncFolder][root-meta] open failed, user_id={}, index={}, inode={}, errno={:?}, error={}",
                @public(self.user_id), @public(self.index), @public(stat.ino()),
                @public(err.raw_os_error()), @public(&err));
            err
        })?;
        meta.init_root().map_err(|err| {
            error!("[RegisterSyncFolder][root-meta] init header failed, user_id={}, index={}, inode={}, errno={:?}, error={}",
                @public(self.user_id), @public(self.index), @public(stat.ino()),
                @public(err.raw_os_error()), @public(&err));
            err
        })?;
        state.path = path.to_path_buf();
        info!("[RegisterSyncFolder][root-meta] ready, user_id={}, index={}, inode={}",
            @public(self.user_id), @public(self.index), @public(stat.ino()));
        Ok(())
    }

    fn get_sync_folder_changes(&self, start: u64, count: u64, result: &mut ChangesResult) -> i32 {
        let state = self.state.lock().unwrap_or_else(|err| err.into_inner());
        if !self.active.load(Ordering::Acquire) {
            return libc::ECANCELED;
        }
        // Keep this bounded read consistent with history and dentry writes.
        state.cache.get_sync_folder_changes(start, count, result)
    }

    fn with_cache<T>(
        &self,
        action: impl FnOnce(&mut FolderCache, &Path) -> io::Result<T>,
    ) -> io::Result<T> {
        let mut state = self.state.lock().unwrap_or_else(|err| err.into_inner());
        if !self.active.load(Ordering::Acquire) {
            return Err(io::Error::from_raw_os_error(libc::ECANCELED));
        }
        let LogFileState { path, cache } = &mut *state;
        let result = action(cache, path);
        let ready = cache.has_ready_changes();
        drop(state);
        if ready {
            LogFileMgr::get_instance().schedule_callbacks();
        }
        result
    }

    fn stop(&self) {
        let mut state = self.state.lock().unwrap_or_else(|err| err.into_inner());
        self.active.store(false, Ordering::Release);
        state.cache.stop_callback();
    }

    fn start_callback(&self) {
        let mut state = self.state.lock().unwrap_or_else(|err| err.into_inner());
        if self.active.load(Ordering::Acquire) {
            state.cache.start_callback();
        }
        info!("[StartCallback] user_id={}, index={}", @public(self.user_id), @public(self.index));
    }

    fn stop_callback(&self) {
        self.state
            .lock()
            .unwrap_or_else(|err| err.into_inner())
            .cache
            .stop_callback();
        info!("[StopCallback] user_id={}, index={}", @public(self.user_id), @public(self.index));
    }

    fn on_data_change(&self) {
        let batches = {
            let mut state = self.state.lock().unwrap_or_else(|err| err.into_inner());
            state.cache.take_change_batches()
        };
        for changes in batches {
            CloudDiskServiceCallbackManager::get_instance().on_change_data(self.index, &changes);
        }
    }

    pub(crate) fn notify_sync_folder_invalid(&self) {
        let Some((callback, sandbox)) =
            CloudDiskServiceCallbackManager::get_instance().callback_for_index(self.index)
        else {
            return;
        };
        let batches = {
            let mut state = self.state.lock().unwrap_or_else(|err| err.into_inner());
            let timestamp = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap_or_default()
                .as_millis() as u64;
            state.cache.take_invalid_change_batches(timestamp)
        };
        // Capture the callback before root cleanup removes its map entry. Dispatch
        // outside the monitor thread so a client may unregister from its callback.
        let _ = ffrt::spawn(move || {
            for changes in batches {
                if let Err(err) = callback.on_change_data(&sandbox, &changes) {
                    error!("[SyncFolderInvalid] callback failed, error={}", @public(format!("{err:?}")));
                }
            }
        });
    }

    fn fill_child_for_dir(&self, path: &Path) {
        info!("[FillChildForDir] begin, user_id={}, index={}, path={}",
            @public(self.user_id), @public(self.index), @private(path.display()));
        let mut directories = vec![path.to_path_buf()];
        let mut visited = HashSet::new();
        let mut entry_count = 0u64;
        let mut error_count = 0u64;
        let mut skipped_count = 0u64;
        let mut cancelled = false;
        'scan: while let Some(directory) = directories.pop() {
            if !self.active.load(Ordering::Acquire) {
                cancelled = true;
                break;
            }
            let stat = match fs::metadata(&directory) {
                Ok(stat) => stat,
                Err(err) => {
                    error_count += 1;
                    self.log_error("scan_stat", &directory, &err);
                    continue;
                }
            };
            if !stat.is_dir() || !visited.insert((stat.dev(), stat.ino())) {
                skipped_count += 1;
                debug!("[FillChildForDir] skipped non-directory or visited inode, path={}, inode={}, is_dir={}",
                    @private(directory.display()), @public(stat.ino()), @public(stat.is_dir()));
                continue;
            }
            info!("[FillChildForDir] read directory, user_id={}, index={}, inode={}, path={}",
                @public(self.user_id), @public(self.index), @public(stat.ino()), @private(directory.display()));
            let entries = match fs::read_dir(&directory) {
                Ok(entries) => entries,
                Err(err) => {
                    error_count += 1;
                    self.log_error("scan_read_dir", &directory, &err);
                    continue;
                }
            };
            for entry in entries {
                if !self.active.load(Ordering::Acquire) {
                    cancelled = true;
                    break 'scan;
                }
                let child = match entry {
                    Ok(entry) => entry.path(),
                    Err(err) => {
                        error_count += 1;
                        self.log_error("scan_read_entry", &directory, &err);
                        continue;
                    }
                };
                match self.with_cache(|cache, root| cache.create(root, &child)) {
                    Ok(is_dir) => {
                        entry_count += 1;
                        debug!("[FillChildForDir] entry processed, user_id={}, index={}, path={}, is_dir={}, entries={}",
                            @public(self.user_id), @public(self.index), @private(child.display()),
                            @public(is_dir), @public(entry_count));
                        if is_dir {
                            directories.push(child);
                        }
                    }
                    Err(err) => {
                        error_count += 1;
                        cancelled |= err.raw_os_error() == Some(libc::ECANCELED);
                        self.log_error("scan_persist_entry", &child, &err);
                    }
                }
            }
        }
        info!("[FillChildForDir] finished, user_id={}, index={}, entries={}, directories={}, errors={}, skipped={}, cancelled={}",
            @public(self.user_id), @public(self.index), @public(entry_count), @public(visited.len()),
            @public(error_count), @public(skipped_count), @public(cancelled));
    }

    pub(crate) fn record_create(&self, path: &Path) {
        match self.with_cache(|cache, root| cache.create(root, path)) {
            Ok(true) => self.fill_child_for_dir(path),
            Ok(false) => {}
            Err(err) => self.log_error("event_create", path, &err),
        }
    }

    pub(crate) fn record_delete(&self, path: &Path) {
        if let Err(err) = self.with_cache(|cache, root| cache.delete(root, path)) {
            self.log_error("event_delete", path, &err);
        }
    }

    pub(crate) fn record_move(&self, from: &Path, to: &Path) {
        if let Err(err) = self.with_cache(|cache, root| cache.move_within(root, from, to)) {
            self.log_error("event_move", to, &err);
        }
    }

    pub(crate) fn record_close_write(&self, path: &Path, modified: bool) {
        if let Err(err) = self.with_cache(|cache, root| cache.close_write(root, path, modified)) {
            self.log_error("event_close_write", path, &err);
        }
    }

    fn log_error(&self, stage: &str, path: &Path, error: &io::Error) {
        error!("[RegistrationEvent] stage={}, user_id={}, index={}, path={}, errno={:?}, error={}, ret={}",
            @public(stage), @public(self.user_id), @public(self.index), @private(path.display()),
            @public(error.raw_os_error()), @public(error), @public(error.raw_os_error().unwrap_or(libc::EIO)));
    }
}

#[derive(Default)]
struct LogFiles {
    files: HashMap<(i32, u32), Arc<CloudDiskServiceLogFile>>,
    scans: Vec<ffrt::Task>,
    listeners: HashSet<(i32, u32)>,
    timer: Option<UniquePtr<ffrt::FfrtTimer>>,
}

pub(crate) struct LogFileMgr {
    files: Mutex<LogFiles>,
    callback_task: Mutex<Option<ffrt::Task>>,
}

impl LogFileMgr {
    pub(crate) fn get_instance() -> &'static Self {
        static INSTANCE: OnceLock<LogFileMgr> = OnceLock::new();
        INSTANCE.get_or_init(|| Self {
            files: Mutex::new(LogFiles::default()),
            callback_task: Mutex::new(None),
        })
    }

    pub(crate) fn register_sync_folder(&self, user_id: i32, index: u32, path: &str) -> i32 {
        info!("[RegisterSyncFolder] user_id={}, index={}, path={}",
            @public(user_id), @public(index), @private(path));
        let path = PathBuf::from(path);
        let mut files = self.files.lock().unwrap_or_else(|err| err.into_inner());
        let result = match files.files.get(&(user_id, index)) {
            Some(file) => {
                info!("[RegisterSyncFolder] reuse in-memory log file, user_id={}, index={}", @public(user_id), @public(index));
                Ok(Arc::clone(file))
            }
            None => CloudDiskServiceLogFile::open(user_id, index, &path).map(Arc::new),
        };
        let file = match result {
            Ok(file) => file,
            Err(err) => {
                let ret = err.raw_os_error().unwrap_or(libc::EIO);
                error!("[RegisterSyncFolder] stage=history, user_id={}, index={}, sync_folder={}, cache_path={}, errno={:?}, error={}, ret={}",
                    @public(user_id), @public(index), @private(path.display()), @private(cache_dir(user_id, index).display()),
                    @public(err.raw_os_error()), @public(err), @public(ret));
                return ret;
            }
        };
        if let Err(err) = file.register_root(&path) {
            let ret = err.raw_os_error().unwrap_or(libc::EIO);
            error!("[RegisterSyncFolder] stage=root-meta, user_id={}, index={}, sync_folder={}, errno={:?}, error={}, ret={}",
                @public(user_id), @public(index), @private(path.display()),
                @public(err.raw_os_error()), @public(err), @public(ret));
            return ret;
        }
        files.files.insert((user_id, index), Arc::clone(&file));
        files.scans.retain(|task| !task.is_finished());
        info!("[RegisterSyncFolder] submit initial scan, user_id={}, index={}, pending_scans={}, path={}",
            @public(user_id), @public(index), @public(files.scans.len()), @private(path.display()));
        files
            .scans
            .push(ffrt::spawn(move || file.fill_child_for_dir(&path)));
        info!("[RegisterSyncFolder] initial scan submitted asynchronously, user_id={}, index={}, ret={}",
            @public(user_id), @public(index), @public(E_OK));
        E_OK
    }

    pub(crate) fn get_log_file(
        &self,
        user_id: i32,
        index: u32,
    ) -> Option<Arc<CloudDiskServiceLogFile>> {
        let mut files = self.files.lock().unwrap_or_else(|err| err.into_inner());
        if let Some(file) = files.files.get(&(user_id, index)) {
            return Some(Arc::clone(file));
        }
        // OnStart restores the registry without registering/scanning the roots again.
        let folder = CloudDiskSyncFolder::get_instance().get_sync_folder_value_by_index(index)?;
        match CloudDiskServiceLogFile::open(user_id, index, Path::new(&folder.path)) {
            Ok(file) => {
                let file = Arc::new(file);
                files.files.insert((user_id, index), Arc::clone(&file));
                Some(file)
            }
            Err(err) => {
                error!("[LogFileMgr] reopen failed, user_id={}, index={}, path={}, error={}",
                    @public(user_id), @public(index), @private(&folder.path), @public(err));
                None
            }
        }
    }

    pub(crate) fn register_sync_folder_changes(&self, user_id: i32, index: u32) {
        let Some(file) = self.get_log_file(user_id, index) else {
            error!("[RegisterSyncFolderChanges] log file unavailable, user_id={}, index={}", @public(user_id), @public(index));
            return;
        };
        let mut files = self.files.lock().unwrap_or_else(|err| err.into_inner());
        if files.listeners.insert((user_id, index)) {
            file.start_callback();
        }
        if files.timer.is_none() {
            let timer = ffrt::start_timer(CHANGE_DATA_TIMEOUT_MS, change_timer_tick);
            if timer.is_null() {
                error!("[RegisterSyncFolderChanges] start FFRT timer failed");
            } else {
                files.timer = Some(timer);
            }
        }
        info!("[RegisterSyncFolderChanges] listener_count={}", @public(files.listeners.len()));
    }

    pub(crate) fn parse_request(
        &self,
        user_id: i32,
        index: u32,
        start: u64,
        count: u64,
        result: &mut ChangesResult,
    ) -> i32 {
        info!("[ParseRequest] user_id={}, index={}, start={}, count={}",
            @public(user_id), @public(index), @public(start), @public(count));
        let Some(file) = self.get_log_file(user_id, index) else {
            error!("[ParseRequest] log file unavailable, user_id={}, index={}", @public(user_id), @public(index));
            return libc::EIO;
        };
        file.get_sync_folder_changes(start, count, result)
    }

    pub(crate) fn unregister_sync_folder_changes(&self, user_id: i32, index: u32) {
        let timer = {
            let mut files = self.files.lock().unwrap_or_else(|err| err.into_inner());
            if !files.listeners.remove(&(user_id, index)) {
                return;
            }
            if let Some(file) = files.files.get(&(user_id, index)) {
                file.stop_callback();
            }
            info!("[UnregisterSyncFolderChanges] user_id={}, index={}, listener_count={}",
                @public(user_id), @public(index), @public(files.listeners.len()));
            if files.listeners.is_empty() {
                files.timer.take()
            } else {
                None
            }
        };
        // FFRT waits for an active tick. Never stop the timer with files locked.
        drop(timer);
    }

    fn schedule_callbacks(&self) {
        let mut task = self
            .callback_task
            .lock()
            .unwrap_or_else(|err| err.into_inner());
        if task.as_ref().is_some_and(|task| !task.is_finished()) {
            return;
        }
        *task = Some(ffrt::spawn(|| Self::get_instance().on_data_change()));
    }

    fn on_data_change(&self) {
        let files = {
            let files = self.files.lock().unwrap_or_else(|err| err.into_inner());
            files
                .listeners
                .iter()
                .filter_map(|key| files.files.get(key).cloned())
                .collect::<Vec<_>>()
        };
        // No registry/cache/manager mutex is held while calling the client.
        for file in files {
            file.on_data_change();
        }
    }

    pub(crate) fn unregister_sync_folder(&self, user_id: i32, index: u32) -> i32 {
        info!("[UnregisterSyncFolder] user_id={}, index={}", @public(user_id), @public(index));
        // Keep cache removal serialized with registration, as in C++ LogFileMgr.
        let mut files = self.files.lock().unwrap_or_else(|err| err.into_inner());
        if let Some(file) = files.files.remove(&(user_id, index)) {
            file.stop();
        }
        files.listeners.remove(&(user_id, index));
        let timer = if files.listeners.is_empty() {
            files.timer.take()
        } else {
            None
        };
        let directory = cache_dir(user_id, index);
        if let Err(err) = fs::remove_dir_all(&directory) {
            if err.kind() != io::ErrorKind::NotFound {
                warn!("[LogFileMgr] remove cache failed, path={}, error={}",
                    @private(directory.display()), @public(err));
            }
        }
        drop(files);
        drop(timer);
        // C++ logs cache-removal failures but still completes unregistration.
        E_OK
    }

    pub(crate) fn stop(&self) {
        let mut files = {
            let mut files = self.files.lock().unwrap_or_else(|err| err.into_inner());
            std::mem::take(&mut *files)
        };
        drop(files.timer.take());
        for file in files.files.values() {
            file.stop();
        }
        for task in files.scans {
            task.wait();
        }
        let callback_task = self
            .callback_task
            .lock()
            .unwrap_or_else(|err| err.into_inner())
            .take();
        if let Some(task) = callback_task {
            task.wait();
        }
    }
}

fn change_timer_tick() {
    // The timer only submits work. Sending synchronous IPC from its callback
    // could deadlock when the client unregisters and stops this same timer.
    LogFileMgr::get_instance().schedule_callbacks();
}

fn cache_dir(user_id: i32, index: u32) -> PathBuf {
    PathBuf::from(format!(
        "/data/service/el2/{user_id}/hmdfs/cache/account_cache/dentry_cache/clouddisk_service_cache/{index:016x}"
    ))
}
