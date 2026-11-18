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

//! File I/O for provider hydration tasks and authorized dehydration.
use std::ffi::CString;
use std::fs::{self, File, Metadata};
use std::io;
use std::os::fd::{AsFd, AsRawFd, FromRawFd};
use std::os::unix::ffi::OsStrExt;
use std::os::unix::fs::{MetadataExt, OpenOptionsExt};

use super::file::{
    file_error, read_checked_state, relative_to_root, target_error, write_state, ExistingFile,
};
use super::hydration_temp::HydrationTempFile;
use super::PlaceholderCallbackManager;
use crate::common::error_codes::{
    E_ACCES, E_ALREADY_HYDRATED, E_FILE_TOO_LARGE, E_INVALID_ARG, E_INVALID_PLACEHOLDER_STATE,
    E_NOT_A_PLACEHOLDER, E_NO_SPACE_LEFT, E_PERM, E_TRY_AGAIN,
};
use crate::common::parcel::MAX_EXECUTE_DATA_SIZE;
use crate::common::types::CallbackExecuteRequest;
use crate::framework::CallbackRequest;
use crate::sync_folder_service::file_state::{
    self, FULLY_HYDRATED, NONE, PARTIALLY_HYDRATED, UNHYDRATED,
};

const EXTEND_ATTEMPTS: usize = 3;

impl ExistingFile {
    // Walk the canonical relative path from a pinned root descriptor. Replacement
    // of an intermediate directory by a symlink must not redirect a data write.
    fn open_under_root(&self) -> Result<File, i32> {
        let root =
            fs::canonicalize(&self.mount_root).map_err(|err| file_error("resolve_root", &err))?;
        let path =
            fs::canonicalize(&self.path).map_err(|err| target_error("resolve_file", &err))?;
        let relative = relative_to_root(&root, &path)?;
        let mut parent = File::options()
            .read(true)
            .custom_flags(libc::O_DIRECTORY | libc::O_NOFOLLOW)
            .open(&root)
            .map_err(|err| target_error("open_root", &err))?;
        let mut components = relative.iter().peekable();
        while let Some(name) = components.next() {
            let last = components.peek().is_none();
            let flags = if last {
                libc::O_RDWR
            } else {
                libc::O_RDONLY | libc::O_DIRECTORY
            };
            let name = CString::new(name.as_bytes()).map_err(|_| E_INVALID_ARG)?;
            // SAFETY: parent is a live directory fd and name is one NUL-terminated component.
            let fd = unsafe {
                libc::openat(
                    parent.as_raw_fd(),
                    name.as_ptr(),
                    flags | libc::O_NOFOLLOW | libc::O_CLOEXEC,
                )
            };
            if fd < 0 {
                return Err(target_error("open_component", &io::Error::last_os_error()));
            }
            // SAFETY: openat returned a fresh owned fd.
            let next = unsafe { File::from_raw_fd(fd) };
            if last {
                return Ok(next);
            }
            parent = next;
        }
        Err(E_INVALID_ARG)
    }

    pub(crate) fn dehydrate(&self) -> Result<(), i32> {
        let _guard = self.lock_idle()?;
        let file = self.open(true, target_error)?;
        let state = read_checked_state(&file)?;
        match state {
            NONE => return Err(E_NOT_A_PLACEHOLDER),
            UNHYDRATED => return Ok(()),
            _ => {}
        }
        let callback =
            PlaceholderCallbackManager::get_instance().callback(&self.bundle, self.key.index)?;
        callback.send(
            CallbackRequest::Dehydrate,
            &self.key.sync_folder,
            &self.key.file_path,
        )?;
        let metadata = file
            .metadata()
            .map_err(|err| file_error("dehydrate_stat", &err))?;
        file.set_len(0)
            .map_err(|err| file_error("dehydrate_truncate", &err))?;
        let mut extended = false;
        for attempt in 1..=EXTEND_ATTEMPTS {
            match file.set_len(metadata.len()) {
                Ok(()) => {
                    extended = true;
                    break;
                }
                Err(err) => {
                    warn!("[Dehydrate] restore size failed, attempt={}, error={}", @public(attempt), @public(err))
                }
            }
        }
        if !extended {
            restore_times(&file, &metadata);
            return Err(E_TRY_AGAIN);
        }
        if let Err(ret) = write_state(&file, UNHYDRATED) {
            restore_times(&file, &metadata);
            return Err(ret);
        }
        if let Err(err) = file.sync_all() {
            warn!("[Dehydrate] fsync failed, error={}", @public(err));
        }
        restore_times(&file, &metadata);
        if state == FULLY_HYDRATED {
            file_state::adjust_ancestors(&self.mount_root, &self.path, true);
        }
        self.refresh_dentry(UNHYDRATED);
        Ok(())
    }
}

fn restore_times(file: &File, saved: &Metadata) {
    let times = [
        libc::timespec {
            tv_sec: saved.atime() as _,
            tv_nsec: saved.atime_nsec() as _,
        },
        libc::timespec {
            tv_sec: saved.mtime() as _,
            tv_nsec: saved.mtime_nsec() as _,
        },
    ];
    // SAFETY: file owns the descriptor and times contains the two timespecs required by futimens.
    if unsafe { libc::futimens(file.as_raw_fd(), times.as_ptr()) } < 0 {
        warn!("[PlaceholderHydration] restore timestamps failed, error={}", @public(io::Error::last_os_error()));
    }
}

fn hydration_io_error(err: io::Error) -> i32 {
    let ret = match err.raw_os_error() {
        Some(libc::ENOSPC) | Some(libc::EDQUOT) => E_NO_SPACE_LEFT,
        Some(libc::EFBIG) => E_FILE_TOO_LARGE,
        Some(libc::EACCES) => E_ACCES,
        Some(libc::EPERM) => E_PERM,
        _ => E_TRY_AGAIN,
    };
    error!("[PlaceholderHydration] I/O failed, error={}, ret={}", @public(err), @public(ret));
    ret
}

fn check_download_state(state: u8) -> Result<(), i32> {
    match state {
        UNHYDRATED | PARTIALLY_HYDRATED => Ok(()),
        NONE | FULLY_HYDRATED => Err(E_TRY_AGAIN),
        _ => Err(E_INVALID_PLACEHOLDER_STATE),
    }
}

pub(super) struct HydrationFile {
    file: File,
    metadata: Metadata,
}

impl HydrationFile {
    pub(super) fn open(target: &ExistingFile) -> Result<Self, i32> {
        let file = target.open_under_root()?;
        let metadata = file
            .metadata()
            .map_err(|err| file_error("hydration_stat", &err))?;
        if metadata.is_dir() || metadata.len() > libc::off_t::MAX as u64 {
            return Err(E_INVALID_ARG);
        }
        match read_checked_state(&file)? {
            NONE => return Err(E_NOT_A_PLACEHOLDER),
            FULLY_HYDRATED => return Err(E_ALREADY_HYDRATED),
            _ => {}
        }
        Ok(Self { file, metadata })
    }

    pub(super) fn identity(&self) -> (u64, u64) {
        (self.metadata.dev(), self.metadata.ino())
    }

    pub(super) fn logical_size(&self) -> u64 {
        self.metadata.len()
    }

    pub(super) fn activate(self, user_id: i32, request_id: u64) -> Result<HydrationWriter, i32> {
        let state = read_checked_state(&self.file)?;
        check_download_state(state)?;
        if state == UNHYDRATED {
            self.set_download_state(PARTIALLY_HYDRATED)?;
        }
        let temp = HydrationTempFile::create(user_id, self.identity(), request_id)
            .map_err(hydration_io_error)?;
        Ok(HydrationWriter {
            output: self,
            temp,
            total_size: None,
            cached_size: 0,
        })
    }

    fn set_download_state(&self, state: u8) -> Result<(), i32> {
        let old = file_state::set_placeholder_state(self.file.as_fd(), state)
            .map_err(hydration_io_error)?;
        check_download_state(old)
    }
}

pub(super) struct HydrationWriter {
    output: HydrationFile,
    temp: HydrationTempFile,
    total_size: Option<u64>,
    cached_size: u64,
}

impl HydrationWriter {
    pub(super) fn progress_sizes(&self) -> (u64, u64) {
        (
            self.cached_size,
            self.total_size
                .unwrap_or_else(|| self.output.logical_size()),
        )
    }

    fn validate_chunk(&self, req: &CallbackExecuteRequest) -> Result<(), i32> {
        if req.size != req.data.len() as u64 || req.data.len() > MAX_EXECUTE_DATA_SIZE {
            return Err(E_INVALID_ARG);
        }
        let end = req.offset.checked_add(req.size).ok_or(E_INVALID_ARG)?;
        if end > req.total_size
            || end > libc::off_t::MAX as u64
            || (req.size == 0 && (!req.is_complete || req.total_size != 0))
            || self.cached_size > req.total_size
            || self.total_size.is_some_and(|size| size != req.total_size)
        {
            return Err(E_INVALID_ARG);
        }
        Ok(())
    }

    fn copy_back(&mut self) -> Result<(), i32> {
        let result = self.temp.copy_to(&mut self.output.file);
        self.restore_times();
        result.map_err(hydration_io_error)
    }

    pub(super) fn keep_partial_data(&mut self) {
        if self.temp.has_data() {
            if let Err(ret) = self.copy_back() {
                warn!("[CancelHydration] copy partial data failed, ret={}", @public(ret));
            }
        }
    }

    pub(super) fn restore_times(&self) {
        restore_times(&self.output.file, &self.output.metadata);
    }

    pub(super) fn execute(
        &mut self,
        target: &ExistingFile,
        req: &CallbackExecuteRequest,
    ) -> Result<(), i32> {
        self.validate_chunk(req)?;
        self.total_size = Some(req.total_size);
        self.temp
            .write_at(&req.data, req.offset)
            .map_err(hydration_io_error)?;
        if req.is_complete {
            self.copy_back()?;
            self.output.file.sync_all().map_err(hydration_io_error)?;
            check_download_state(read_checked_state(&self.output.file)?)?;
            self.output.set_download_state(FULLY_HYDRATED)?;
            file_state::adjust_ancestors(&target.mount_root, &target.path, false);
            target.refresh_dentry(FULLY_HYDRATED);
        }
        self.cached_size += req.size.min(req.total_size - self.cached_size);
        Ok(())
    }
}
