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

//! Per-root persistence for the initial scan and live monitor events.

use std::fs::{self, DirBuilder, File, Metadata, Permissions};
use std::io;
use std::os::fd::AsRawFd;
use std::os::unix::{
    ffi::OsStrExt,
    fs::{DirBuilderExt, MetadataExt, OpenOptionsExt, PermissionsExt},
};
use std::path::{Path, PathBuf};

use super::file_state;
use super::history::{History, Operation};
use super::metafile::{CloudDiskServiceMetaFile, Dentry, RecordId};
use crate::common::error_codes::E_OK;
use crate::common::types::{ChangeData, ChangesResult};
use crate::sync_folder_manager;

const MAX_CHANGE_DATAS_SIZE: usize = 20;

pub(super) struct FolderCache {
    directory: PathBuf,
    history: History,
    need_callback: bool,
    changes: Vec<ChangeData>,
    ready_batches: Vec<Vec<ChangeData>>,
    has_unpaired_close_modify: bool,
}

impl FolderCache {
    pub(super) fn open(directory: PathBuf) -> io::Result<Self> {
        let history = History::open(&directory)?;
        Ok(Self {
            directory,
            history,
            need_callback: false,
            changes: Vec::new(),
            ready_batches: Vec::new(),
            has_unpaired_close_modify: false,
        })
    }

    // C++ LogFileMgr::PraseRequest / CloudDiskServiceLogFile::PraseLog.
    pub(super) fn get_sync_folder_changes(
        &self,
        start: u64,
        count: u64,
        result: &mut ChangesResult,
    ) -> i32 {
        let start_line = if start == 0 {
            self.history.get_start_line()
        } else {
            start
        };
        // Match the C++ uint64_t range calculation without debug-build overflow.
        let end = start_line.wrapping_add(count);
        let mut next_usn = end;
        let mut ret = E_OK;
        for line in start_line..end {
            match self.parse_log(line) {
                Ok(Some(change)) => {
                    info!("[ParseLog] line={}, operation={}, relative_path={}, size={}, mtime={}, timestamp={}",
                        @public(change.update_sequence_number), @public(change.operation_type),
                        @private(&change.relative_path), @public(change.size), @public(change.mtime), @public(change.time_stamp));
                    result.changes_data.push(change);
                    ret = E_OK;
                }
                Ok(None) => {
                    result.is_eof = true;
                    next_usn = line;
                    ret = E_OK;
                    break;
                }
                Err(error) => {
                    error!("[ParseLog] failed, line={}, ret={}", @public(line), @public(error));
                    // C++ continues past a failed record and returns the last status.
                    ret = error;
                }
            }
        }
        if ret == E_OK {
            result.next_usn = next_usn;
        }
        ret
    }

    fn parse_log(&self, line: u64) -> Result<Option<ChangeData>, i32> {
        let Some(log) = self.history.read_log(line)? else {
            return Ok(None);
        };
        let parent = CloudDiskServiceMetaFile::open(&self.directory, log.parent_inode)
            .map_err(|err| err.raw_os_error().unwrap_or(libc::EIO))?;
        let relative_path = parent.get_relative_path(&self.directory).unwrap_or_else(|err| {
            // C++ keeps the basename when a deleted parent cannot be resolved.
            warn!("[ParseLog] get relative path failed, parent_inode={}, error={}", @public(log.parent_inode), @public(err));
            String::new()
        });
        let entry = parent
            .lookup_by_offset(log.position)
            .map_err(|err| err.raw_os_error().unwrap_or(libc::EIO))?;
        Ok(Some(ChangeData {
            update_sequence_number: log.line,
            file_id: entry.record_id.to_vec(),
            parent_file_id: log.parent_record_id.to_vec(),
            relative_path: relative_path + &entry.name,
            operation_type: log.operation,
            size: entry.size,
            mtime: entry.mtime,
            time_stamp: log.timestamp,
        }))
    }

    pub(super) fn start_callback(&mut self) {
        self.need_callback = true;
    }

    pub(super) fn stop_callback(&mut self) {
        self.need_callback = false;
        self.changes.clear();
        self.ready_batches.clear();
        self.has_unpaired_close_modify = false;
    }

    pub(super) fn has_ready_changes(&self) -> bool {
        !self.ready_batches.is_empty()
    }

    pub(super) fn take_change_batches(&mut self) -> Vec<Vec<ChangeData>> {
        let mut batches = std::mem::take(&mut self.ready_batches);
        if !self.has_unpaired_close_modify && !self.changes.is_empty() {
            batches.push(std::mem::take(&mut self.changes));
        }
        batches
    }

    pub(super) fn take_invalid_change_batches(&mut self, timestamp: u64) -> Vec<Vec<ChangeData>> {
        // C++ appends SYNC_FOLDER_INVALID to the pending changes before cleanup.
        self.changes.push(ChangeData {
            operation_type: 5,
            time_stamp: timestamp,
            ..ChangeData::default()
        });
        self.has_unpaired_close_modify = false;
        self.take_change_batches()
    }

    pub(super) fn create(&mut self, root: &Path, path: &Path) -> io::Result<bool> {
        let stat = fs::metadata(path)?;
        let state = file_state::state(path).unwrap_or_default();
        let (parent, _) = self.parent_and_name(root, path)?;
        self.store(root, path, &parent, &stat, state, None)?;
        Ok(stat.is_dir())
    }

    pub(super) fn delete(&mut self, root: &Path, path: &Path) -> io::Result<()> {
        let (parent, _) = self.parent_and_name(root, path)?;
        let dentry = self.invalidate(root, path, &parent, Operation::Delete)?;
        if file_state::is_counted(dentry.old_placeholder) {
            file_state::adjust_ancestors(root, path, false);
        }
        Ok(())
    }

    pub(super) fn move_within(&mut self, root: &Path, from: &Path, to: &Path) -> io::Result<()> {
        let stat = fs::metadata(to)?;
        let (old_parent, _) = self.parent_and_name(root, from)?;
        let old = self.invalidate(root, from, &old_parent, Operation::MoveFrom)?;
        if file_state::is_counted(old.old_placeholder) {
            file_state::adjust_ancestors(root, from, false);
        }

        let (new_parent, _) = self.parent_and_name(root, to)?;
        let state = file_state::state(to).unwrap_or(old.old_placeholder);
        let new = self.store(root, to, &new_parent, &stat, state, Some(old.record_id))?;
        if file_state::is_counted(new.old_placeholder) {
            file_state::adjust_ancestors(root, to, false);
        }
        if file_state::is_counted(state) {
            file_state::adjust_ancestors(root, to, true);
        } else if state != 0 {
            file_state::track_ancestors(root, to);
        }
        Ok(())
    }

    pub(super) fn close_write(
        &mut self,
        root: &Path,
        path: &Path,
        modified: bool,
    ) -> io::Result<()> {
        let stat = fs::metadata(path)?;
        let (parent, name) = self.parent_and_name(root, path)?;
        let hash = sync_folder_manager::dentry_hash(name);
        let dentry = parent.meta.update(name, hash, &stat)?;
        if modified {
            self.append_change(root, path, &parent, &dentry, Operation::CloseModify)?;
        }
        self.append_change(root, path, &parent, &dentry, Operation::CloseWrite)
    }

    fn parent_and_name<'a>(
        &mut self,
        root: &Path,
        path: &'a Path,
    ) -> io::Result<(Directory, &'a [u8])> {
        let name = path
            .file_name()
            .ok_or_else(|| io::Error::from_raw_os_error(libc::EINVAL))?;
        let parent = path
            .parent()
            .filter(|dir| dir.starts_with(root))
            .ok_or_else(|| io::Error::from_raw_os_error(libc::EINVAL))?;
        Ok((self.ensure_directory(root, parent)?, name.as_bytes()))
    }

    fn ensure_directory(&mut self, root: &Path, mut path: &Path) -> io::Result<Directory> {
        // A live event can arrive before the initial scan reaches its parent.
        let mut missing = Vec::new();
        let mut parent = loop {
            let stat = fs::metadata(path)?;
            if !stat.is_dir() {
                return Err(io::Error::from_raw_os_error(libc::ENOTDIR));
            }
            let inode = stat.ino();
            let meta = CloudDiskServiceMetaFile::open(&self.directory, inode)?;
            match meta.record_id() {
                Ok(id) => break Directory { inode, id, meta },
                Err(err) if err.raw_os_error() == Some(libc::ENOENT) && path != root => {
                    missing.push((path, stat));
                    path = path
                        .parent()
                        .ok_or_else(|| io::Error::from_raw_os_error(libc::EINVAL))?;
                }
                Err(err) => return Err(err),
            }
        };
        for (path, stat) in missing.into_iter().rev() {
            let state = file_state::state(path).unwrap_or_default();
            let entry = self.store(root, path, &parent, &stat, state, None)?;
            let inode = stat.ino();
            parent = Directory {
                inode,
                id: entry.record_id,
                meta: CloudDiskServiceMetaFile::open(&self.directory, inode)?,
            };
        }
        Ok(parent)
    }

    fn store(
        &mut self,
        root: &Path,
        path: &Path,
        parent: &Directory,
        stat: &Metadata,
        state: u8,
        replacement_id: Option<RecordId>,
    ) -> io::Result<Dentry> {
        let name = path
            .file_name()
            .ok_or_else(|| io::Error::from_raw_os_error(libc::EINVAL))?
            .as_bytes();
        let hash = sync_folder_manager::dentry_hash(name);
        let dentry = parent
            .meta
            .upsert(name, hash, stat, state, replacement_id)?;
        debug!("[CacheCreate] path={}, parent_inode={}, inode={}, hash={}, inserted={}, group={}, slot={}",
            @private(path.display()), @public(parent.inode), @public(stat.ino()), @public(hash),
            @public(dentry.inserted), @public(dentry.position.group), @public(dentry.position.slot));
        if stat.is_dir() {
            CloudDiskServiceMetaFile::open(&self.directory, stat.ino())?.init_child(
                parent.inode,
                dentry.record_id,
                hash,
            )?;
        }
        let operation = if replacement_id.is_some() {
            Operation::MoveTo
        } else if dentry.inserted {
            Operation::Create
        } else {
            // A scan and a fanotify event can describe the same entry.
            debug!("[CacheCreate] existing dentry reused, no duplicate CREATE log, path={}", @private(path.display()));
            return Ok(dentry);
        };
        self.append_change(root, path, parent, &dentry, operation)?;
        Ok(dentry)
    }

    fn invalidate(
        &mut self,
        root: &Path,
        path: &Path,
        parent: &Directory,
        operation: Operation,
    ) -> io::Result<Dentry> {
        let name = path
            .file_name()
            .ok_or_else(|| io::Error::from_raw_os_error(libc::EINVAL))?
            .as_bytes();
        let hash = sync_folder_manager::dentry_hash(name);
        let dentry = parent.meta.invalidate(name, hash)?;
        self.append_change(root, path, parent, &dentry, operation)?;
        Ok(dentry)
    }

    fn append_change(
        &mut self,
        root: &Path,
        path: &Path,
        parent: &Directory,
        dentry: &Dentry,
        operation: Operation,
    ) -> io::Result<()> {
        let name = path
            .file_name()
            .ok_or_else(|| io::Error::from_raw_os_error(libc::EINVAL))?
            .as_bytes();
        let hash = sync_folder_manager::dentry_hash(name);
        let (line, timestamp) =
            self.history
                .append(operation, parent.inode, parent.id, hash, dentry.position)?;
        if !self.need_callback {
            return Ok(());
        }
        let full_path = path
            .to_str()
            .ok_or_else(|| io::Error::from_raw_os_error(libc::EILSEQ))?;
        let root_path = root
            .to_str()
            .ok_or_else(|| io::Error::from_raw_os_error(libc::EILSEQ))?;
        let stat = fs::metadata(path).ok();
        let change = ChangeData {
            update_sequence_number: line,
            file_id: dentry.record_id.to_vec(),
            parent_file_id: parent.id.to_vec(),
            // C++ substr(root.size()) retains the leading '/' of a relative path.
            relative_path: full_path
                .strip_prefix(root_path)
                .unwrap_or(full_path)
                .to_owned(),
            operation_type: operation as u8,
            size: stat.as_ref().map_or(0, MetadataExt::size),
            mtime: stat.as_ref().map_or(0, |stat| stat.mtime() as u64),
            time_stamp: timestamp,
        };
        if matches!(operation, Operation::CloseModify) {
            if self.changes.len() == MAX_CHANGE_DATAS_SIZE - 1 {
                self.ready_batches.push(std::mem::take(&mut self.changes));
            }
            self.has_unpaired_close_modify = true;
            self.changes.push(change);
            return Ok(());
        }
        if matches!(operation, Operation::CloseWrite) {
            self.has_unpaired_close_modify = false;
        }
        self.changes.push(change);
        if self.changes.len() >= MAX_CHANGE_DATAS_SIZE {
            self.ready_batches.push(std::mem::take(&mut self.changes));
        }
        Ok(())
    }
}

struct Directory {
    inode: u64,
    id: RecordId,
    meta: CloudDiskServiceMetaFile,
}

pub(super) fn open_cache_file(path: &Path) -> io::Result<File> {
    debug!("[CacheFile] open begin, path={}, flags=O_RDWR|O_CREAT|O_NOFOLLOW, mode=0660, truncate=false",
        @private(path.display()));
    if let Some(parent) = path.parent() {
        create_cache_directory(parent)?;
    }
    let file = File::options()
        .read(true)
        .write(true)
        .create(true)
        .truncate(false)
        .mode(0o660)
        .custom_flags(libc::O_NOFOLLOW)
        .open(path)
        .map_err(|err| {
            error!("[CacheFile] open failed, path={}, errno={:?}, error={}",
                @private(path.display()), @public(err.raw_os_error()), @public(&err));
            err
        })?;
    debug!("[CacheFile] open completed, path={}, fd={}", @private(path.display()), @public(file.as_raw_fd()));
    Ok(file)
}

fn create_cache_directory(path: &Path) -> io::Result<()> {
    if cache_directory_exists(path)? {
        debug!("[CacheDirectory] existing directory reused, path={}", @private(path.display()));
        return Ok(());
    }
    info!("[CacheDirectory] missing, prepare parent and mkdir, path={}", @private(path.display()));
    if let Some(parent) = path.parent() {
        create_cache_directory(parent)?;
    }
    match DirBuilder::new().mode(0o771).create(path) {
        Ok(()) => {
            info!("[CacheDirectory] mkdir completed, apply mode=0771, path={}", @private(path.display()));
            fs::set_permissions(path, Permissions::from_mode(0o771)).map_err(|err| {
                error!("[CacheDirectory] chmod failed, path={}, mode=0771, errno={:?}, error={}",
                    @private(path.display()), @public(err.raw_os_error()), @public(&err));
                err
            })?;
            info!("[CacheDirectory] ready, path={}, mode=0771", @private(path.display()));
            Ok(())
        }
        Err(err) if err.raw_os_error() == Some(libc::EEXIST) && cache_directory_exists(path)? => {
            info!("[CacheDirectory] mkdir returned EEXIST, confirmed directory and reused, path={}", @private(path.display()));
            Ok(())
        }
        Err(err) => {
            error!("[CacheDirectory] mkdir failed, path={}, errno={:?}, error={}",
                @private(path.display()), @public(err.raw_os_error()), @public(&err));
            Err(err)
        }
    }
}

fn cache_directory_exists(path: &Path) -> io::Result<bool> {
    // C++ checks existence with access(F_OK), without reading directory metadata.
    // O_PATH | O_DIRECTORY also verifies the type without requiring SELinux getattr.
    // Path::is_dir() would hide a metadata error and lead to a misleading mkdir EEXIST.
    match File::options()
        .read(true)
        .custom_flags(libc::O_PATH | libc::O_DIRECTORY)
        .open(path)
    {
        Ok(_) => Ok(true),
        Err(err) if err.raw_os_error() == Some(libc::ENOENT) => Ok(false),
        Err(err) => {
            error!("[CacheDirectory] lookup failed, flags=O_PATH|O_DIRECTORY, path={}, errno={:?}, error={}",
                @private(path.display()), @public(err.raw_os_error()), @public(&err));
            Err(err)
        }
    }
}
