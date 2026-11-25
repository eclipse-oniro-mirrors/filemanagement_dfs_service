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

//! Placeholder file creation, queries and state changes.

use std::collections::hash_map::DefaultHasher;
use std::ffi::{CStr, CString};
use std::fs::{self, File, OpenOptions};
use std::hash::{Hash, Hasher};
use std::io;
use std::os::fd::{AsFd, AsRawFd, FromRawFd};
use std::os::unix::fs::OpenOptionsExt;
use std::path::{Path, PathBuf};
use std::sync::{Mutex, MutexGuard, OnceLock};

use super::PlaceholderTaskManager;
use crate::common::error_codes::{
    E_ACCES, E_FILE_ALREADY_EXISTS, E_FILE_NOT_EXIST, E_FILE_TOO_LARGE, E_HYDRATE_IN_PROGRESS,
    E_INVALID_ARG, E_INVALID_PLACEHOLDER_STATE, E_IS_A_PLACEHOLDER, E_NAME_TOO_LONG,
    E_NOT_A_DIRECTORY, E_NOT_A_PLACEHOLDER, E_NOT_SUPPORTED, E_NO_SPACE_LEFT, E_PERM,
    E_PLACEHOLDER_CUSTOM_INFO_NOT_FOUND, E_PLACEHOLDER_NOT_FULLY_HYDRATED,
    E_SYNC_FOLDER_PATH_NOT_EXIST, E_TRY_AGAIN,
};
use crate::common::types::{PlaceholderCustomInfo, PlaceholderInfo};
use crate::sync_folder_manager;
use crate::sync_folder_service::file_state::{self, FULLY_HYDRATED, NONE, UNHYDRATED};
use crate::sync_folder_service::LogFileMgr;

const CUSTOM_INFO_MAX_SIZE: usize = 1024;
const FILE_MODE: libc::c_uint = 0o660;
const FILE_LOCK_BUCKETS: usize = 64;
const CUSTOM_INFO_XATTR: &CStr = match CStr::from_bytes_with_nul(b"user.clouddisk.custominfo\0") {
    Ok(name) => name,
    Err(_) => panic!("invalid custom info xattr name"),
};

pub(crate) fn validate_params(
    sync_folder: &str,
    relative_path: &str,
    info: &PlaceholderInfo,
    custom_info: &PlaceholderCustomInfo,
) -> Result<(), i32> {
    if sync_folder.is_empty()
        || relative_path.is_empty()
        || custom_info.data.len() > CUSTOM_INFO_MAX_SIZE
    {
        return Err(E_INVALID_ARG);
    }
    if info.logical_size > libc::off_t::MAX as u64 {
        return Err(E_FILE_TOO_LARGE);
    }
    Ok(())
}

pub(crate) fn create(
    sync_folder: &str,
    relative_path: &str,
    user_id: &str,
    info: &PlaceholderInfo,
    custom_info: &PlaceholderCustomInfo,
) -> Result<(), i32> {
    validate_relative_path(relative_path)?;

    let (parent_relative, name) = relative_path
        .rsplit_once('/')
        .unwrap_or(("", relative_path));
    let name = CString::new(name).map_err(|_| E_INVALID_ARG)?;
    let root = sync_folder_manager::path_to_mnt_path_by_sandbox_path(sync_folder, user_id)?;
    let root = Path::new(&root);
    let parent_path = if parent_relative.is_empty() {
        root.to_path_buf()
    } else {
        root.join(parent_relative)
    };
    info!("[CreatePlaceholderFile] open parent, user_id={}, parent={}, relative_path={}",
        @public(user_id), @private(parent_path.display()), @private(relative_path));
    let parent = File::options()
        .read(true)
        .custom_flags(libc::O_DIRECTORY | libc::O_NOFOLLOW)
        .open(&parent_path)
        .map_err(|err| file_error("open_parent", &err))?;
    // SAFETY: parent owns a live directory fd, name is NUL terminated, and the mode
    // is passed as the promoted integer expected by the variadic openat call.
    let fd = unsafe {
        libc::openat(
            parent.as_raw_fd(),
            name.as_ptr(),
            libc::O_CREAT | libc::O_EXCL | libc::O_RDWR | libc::O_NOFOLLOW | libc::O_CLOEXEC,
            FILE_MODE,
        )
    };
    if fd < 0 {
        return Err(file_error("openat", &io::Error::last_os_error()));
    }
    // SAFETY: A successful openat returns a new owned descriptor.
    let file = unsafe { File::from_raw_fd(fd) };
    if let Err(ret) = set_attributes(&file, info, &custom_info.data) {
        // SAFETY: The same parent descriptor and name are still live.
        if unsafe { libc::unlinkat(parent.as_raw_fd(), name.as_ptr(), 0) } < 0 {
            let err = io::Error::last_os_error();
            error!("[CreatePlaceholderFile] rollback unlinkat failed, path={}, error={}",
                @private(relative_path), @public(err));
        }
        return Err(ret);
    }

    let path = root.join(relative_path);
    file_state::adjust_ancestors(root, &path, true);
    Ok(())
}

fn set_attributes(file: &File, info: &PlaceholderInfo, custom_data: &[u8]) -> Result<(), i32> {
    file.set_len(info.logical_size)
        .map_err(|err| file_error("set_size", &err))?;
    file_state::set_new_placeholder_state(file.as_fd())
        .map_err(|err| file_error("set_state", &err))?;
    set_custom_info(file, custom_data)?;
    set_times(file, info)
}

fn set_custom_info(file: &File, custom_data: &[u8]) -> Result<(), i32> {
    if !custom_data.is_empty() {
        file_state::set_fd_xattr(file.as_fd(), CUSTOM_INFO_XATTR, custom_data)
            .map_err(|err| file_error("set_custom_info", &err))?;
    }
    Ok(())
}

fn set_times(file: &File, info: &PlaceholderInfo) -> Result<(), i32> {
    let times = [timespec(info.atime_ms), timespec(info.mtime_ms)];
    // SAFETY: file owns a live descriptor and times contains the two required values.
    if unsafe { libc::futimens(file.as_raw_fd(), times.as_ptr()) } < 0 {
        return Err(file_error("set_times", &io::Error::last_os_error()));
    }
    Ok(())
}

fn timespec(ms: u64) -> libc::timespec {
    libc::timespec {
        tv_sec: (ms / 1000) as libc::time_t,
        tv_nsec: ((ms % 1000) * 1_000_000) as _,
    }
}

pub(super) fn file_error(stage: &str, err: &io::Error) -> i32 {
    log_io_error(stage, err, convert_errno)
}

pub(super) fn target_error(stage: &str, err: &io::Error) -> i32 {
    log_io_error(stage, err, convert_target_errno)
}

fn state_error(stage: &str, err: &io::Error) -> i32 {
    log_io_error(stage, err, |errno| match errno {
        libc::EINVAL | libc::ERANGE => E_INVALID_PLACEHOLDER_STATE,
        _ => convert_target_errno(errno),
    })
}

fn custom_info_error(stage: &str, err: &io::Error) -> i32 {
    log_io_error(stage, err, |errno| match errno {
        libc::ENODATA => E_PLACEHOLDER_CUSTOM_INFO_NOT_FOUND,
        _ => convert_target_errno(errno),
    })
}

fn log_io_error(stage: &str, err: &io::Error, convert: fn(i32) -> i32) -> i32 {
    let ret = convert(err.raw_os_error().unwrap_or(libc::EIO));
    error!("[PlaceholderFile] stage={}, error={}, ret={}", @public(stage), @public(err), @public(ret));
    ret
}

fn convert_target_errno(errno: i32) -> i32 {
    if errno == libc::ENOENT {
        E_FILE_NOT_EXIST
    } else {
        convert_errno(errno)
    }
}

fn convert_errno(errno: i32) -> i32 {
    match errno {
        libc::EEXIST => E_FILE_ALREADY_EXISTS,
        libc::ENOTDIR => E_NOT_A_DIRECTORY,
        libc::ENOSPC | libc::EDQUOT => E_NO_SPACE_LEFT,
        libc::EINVAL | libc::EISDIR | libc::ELOOP | libc::E2BIG => E_INVALID_ARG,
        libc::ENAMETOOLONG => E_NAME_TOO_LONG,
        libc::EFBIG => E_FILE_TOO_LARGE,
        libc::ENOENT => E_SYNC_FOLDER_PATH_NOT_EXIST,
        libc::EACCES => E_ACCES,
        libc::EPERM => E_PERM,
        libc::ENOTTY | libc::EOPNOTSUPP => E_NOT_SUPPORTED,
        _ => E_TRY_AGAIN,
    }
}

pub(crate) fn validate_relative_path(path: &str) -> Result<(), i32> {
    if path.is_empty()
        || path.as_bytes().contains(&0)
        || path.starts_with('/')
        || path.ends_with('/')
        || path.split('/').any(|segment| matches!(segment, "." | ".."))
    {
        error!("[PlaceholderFile] invalid relative path, path={}", @private(path));
        return Err(E_INVALID_ARG);
    }
    Ok(())
}

pub(super) fn relative_to_root<'a>(root: &Path, path: &'a Path) -> Result<&'a Path, i32> {
    match path.strip_prefix(root) {
        Ok(relative) if !relative.as_os_str().is_empty() => Ok(relative),
        _ => {
            error!("[PlaceholderFile] path outside sync root, root={}, path={}",
                @private(root.display()), @private(path.display()));
            Err(E_INVALID_ARG)
        }
    }
}

pub(crate) fn query_state(root: &str, relative_path: &str) -> Result<u8, i32> {
    validate_relative_path(relative_path)?;
    let root = Path::new(root);
    let path = fs::canonicalize(root.join(relative_path))
        .map_err(|err| target_error("resolve_query_path", &err))?;
    // A symlink may resolve to a different sync root in the same HMDFS mount.
    relative_to_root(root, &path)?;
    if fs::metadata(&path).is_ok_and(|stat| stat.is_dir()) {
        return Err(E_INVALID_ARG);
    }
    file_state::state(&path).map_err(|err| state_error("query_state", &err))
}

pub(crate) enum StateTransition {
    Mark,
    Unmark,
}

#[derive(Clone, PartialEq, Eq, Hash)]
pub(super) struct TaskKey {
    pub(super) index: u32,
    pub(super) sync_folder: String,
    pub(super) file_path: String,
}

impl TaskKey {
    fn mounted_file(&self, user: &str) -> Result<PathBuf, i32> {
        validate_relative_path(&self.file_path)?;
        let path = Path::new(&self.sync_folder).join(&self.file_path);
        let path = path.to_str().ok_or(E_INVALID_ARG)?;
        sync_folder_manager::path_to_mnt_path_by_sandbox_path(path, user)
            .map(PathBuf::from)
            .map_err(|ret| {
                if ret == E_SYNC_FOLDER_PATH_NOT_EXIST {
                    E_FILE_NOT_EXIST
                } else {
                    ret
                }
            })
    }

    pub(super) fn new(index: u32, sync_folder: &str, file_path: &str) -> Self {
        Self {
            index,
            sync_folder: sync_folder.to_owned(),
            file_path: file_path.to_owned(),
        }
    }
}

pub(crate) struct ExistingFile {
    pub(super) mount_root: PathBuf,
    pub(super) path: PathBuf,
    relative: PathBuf,
    pub(super) user_id: i32,
    pub(super) bundle: String,
    pub(super) key: TaskKey,
}

impl ExistingFile {
    pub(crate) fn resolve(
        sync_folder: &str,
        relative_path: &str,
        user_id: i32,
        index: u32,
        bundle: String,
    ) -> Result<Self, i32> {
        let key = TaskKey::new(index, sync_folder, relative_path);
        let user = user_id.to_string();
        let path = key.mounted_file(&user)?;
        let root = sync_folder_manager::path_to_mnt_path_by_sandbox_path(sync_folder, &user)?;
        Self::from_mount_paths(key, user_id, bundle, root, path)
    }

    pub(super) fn resolve_for_accessor(
        key: TaskKey,
        user_id: i32,
        bundle: String,
    ) -> Result<Self, i32> {
        let user = user_id.to_string();
        let root = sync_folder_manager::path_to_mnt_path_by_sandbox_path(&key.sync_folder, &user)?;
        let path = key.mounted_file(&user)?;
        Self::from_mount_paths(key, user_id, bundle, root, path)
    }

    fn from_mount_paths(
        key: TaskKey,
        user_id: i32,
        bundle: String,
        root: String,
        path: PathBuf,
    ) -> Result<Self, i32> {
        let mount_root = PathBuf::from(root);
        let relative = relative_to_root(&mount_root, &path)?.to_path_buf();
        info!("[PlaceholderFile] resolved, user_id={}, index={}, path={}",
            @public(user_id), @public(key.index), @private(path.display()));
        Ok(Self {
            mount_root,
            path,
            relative,
            user_id,
            bundle,
            key,
        })
    }

    pub(super) fn lock(&self) -> MutexGuard<'static, ()> {
        lock_placeholder_file(&self.path)
    }

    pub(super) fn lock_idle(&self) -> Result<MutexGuard<'static, ()>, i32> {
        let guard = self.lock();
        if PlaceholderTaskManager::get_instance().has_outstanding(&self.key) {
            return Err(E_HYDRATE_IN_PROGRESS);
        }
        Ok(guard)
    }

    pub(crate) fn change_state(&self, transition: StateTransition) -> Result<(), i32> {
        let _guard = self.lock_idle()?;
        let file = self.open(true, target_error)?;
        let old = read_checked_state(&file)?;
        let new = match transition {
            StateTransition::Mark if old == NONE => FULLY_HYDRATED,
            StateTransition::Mark => return Err(E_IS_A_PLACEHOLDER),
            StateTransition::Unmark if old == NONE => return Err(E_NOT_A_PLACEHOLDER),
            StateTransition::Unmark if old == FULLY_HYDRATED => NONE,
            StateTransition::Unmark => return Err(E_PLACEHOLDER_NOT_FULLY_HYDRATED),
        };
        write_state(&file, new)?;
        // NONE and FULLY_HYDRATED do not contribute to the ancestor counts.
        self.refresh_dentry(new);
        Ok(())
    }

    pub(crate) fn convert_to_file(&self) -> Result<(), i32> {
        let _guard = self.lock_idle()?;
        let file = self.open(true, file_error)?;
        require_placeholder(&file)?;
        file.set_len(0)
            .map_err(|err| file_error("truncate", &err))?;
        let old = write_state(&file, NONE)?;
        if file_state::is_counted(old) {
            file_state::adjust_ancestors(&self.mount_root, &self.path, false);
            self.refresh_dentry(NONE);
        }
        Ok(())
    }

    pub(crate) fn update(
        &self,
        info: &PlaceholderInfo,
        custom: &PlaceholderCustomInfo,
    ) -> Result<(), i32> {
        let _guard = self.lock_idle()?;
        let file = self.open(true, target_error)?;
        require_placeholder(&file)?;
        // Persist the lowered state before discarding any hydrated content.
        let old = write_state(&file, UNHYDRATED)?;
        if old > FULLY_HYDRATED {
            error!("[PlaceholderFile] invalid previous state={}", @public(old));
            return Err(E_INVALID_PLACEHOLDER_STATE);
        }
        if !file_state::is_counted(old) {
            file_state::adjust_ancestors(&self.mount_root, &self.path, true);
        }
        if self.user_id >= 0 {
            self.refresh_dentry(UNHYDRATED);
        }
        file.sync_all()
            .map_err(|err| file_error("sync_state", &err))?;

        file.set_len(0)
            .map_err(|err| file_error("truncate", &err))?;
        file.set_len(info.logical_size)
            .map_err(|err| file_error("set_size", &err))?;
        file.sync_all()
            .map_err(|err| file_error("sync_content", &err))?;

        set_custom_info(&file, &custom.data)?;
        set_times(&file, info)
    }

    pub(crate) fn custom_info(&self) -> Result<Vec<u8>, i32> {
        let file = self.open(false, target_error)?;
        require_placeholder(&file)?;

        let size = file_state::get_fd_xattr(file.as_fd(), CUSTOM_INFO_XATTR, &mut [])
            .map_err(|err| custom_info_error("probe_custom_info", &err))?;
        if size > CUSTOM_INFO_MAX_SIZE {
            error!("[PlaceholderFile] stored custom info too large, size={}", @public(size));
            return Err(E_TRY_AGAIN);
        }
        let mut data = vec![0; size];
        if size != 0 {
            let read = file_state::get_fd_xattr(file.as_fd(), CUSTOM_INFO_XATTR, &mut data)
                .map_err(|err| custom_info_error("read_custom_info", &err))?;
            if read != size {
                error!("[PlaceholderFile] custom info changed while reading, expected={}, actual={}",
                    @public(size), @public(read));
                return Err(E_TRY_AGAIN);
            }
        }
        Ok(data)
    }

    pub(super) fn open(
        &self,
        write: bool,
        open_error: fn(&str, &io::Error) -> i32,
    ) -> Result<File, i32> {
        let stat = fs::metadata(&self.path).map_err(|err| target_error("stat", &err))?;
        if stat.is_dir() {
            return Err(E_INVALID_ARG);
        }
        OpenOptions::new()
            .read(true)
            .write(write)
            .custom_flags(libc::O_NOFOLLOW)
            .open(&self.path)
            .map_err(|err| open_error("open", &err))
    }

    pub(super) fn refresh_dentry(&self, state: u8) {
        let file = LogFileMgr::get_instance().get_log_file(self.user_id, self.key.index);
        let result = file
            .ok_or_else(|| io::Error::from_raw_os_error(libc::ENOENT))
            .and_then(|file| file.set_dentry_placeholder_state(&self.relative, state));
        if let Err(err) = result {
            warn!("[PlaceholderFile] dentry update failed, index={}, state={}, path={}, error={}",
                @public(self.key.index), @public(state), @private(self.path.display()), @public(err));
        }
    }
}

// Matches the striped GetPlaceholderFileMutex used by placeholder mutations and activation.
fn lock_placeholder_file(path: &Path) -> MutexGuard<'static, ()> {
    static LOCKS: OnceLock<[Mutex<()>; FILE_LOCK_BUCKETS]> = OnceLock::new();
    let locks = LOCKS.get_or_init(|| std::array::from_fn(|_| Mutex::new(())));
    let mut hash = DefaultHasher::new();
    path.hash(&mut hash);
    locks[hash.finish() as usize % locks.len()]
        .lock()
        .unwrap_or_else(|err| err.into_inner())
}

pub(super) fn read_checked_state(file: &File) -> Result<u8, i32> {
    let state = file_state::placeholder_state(file.as_fd())
        .map_err(|err| state_error("read_state", &err))?;
    if state > FULLY_HYDRATED {
        error!("[PlaceholderFile] invalid stored state={}", @public(state));
        return Err(E_INVALID_PLACEHOLDER_STATE);
    }
    Ok(state)
}

fn require_placeholder(file: &File) -> Result<(), i32> {
    if read_checked_state(file)? == NONE {
        return Err(E_NOT_A_PLACEHOLDER);
    }
    Ok(())
}

pub(super) fn write_state(file: &File, state: u8) -> Result<u8, i32> {
    let old = file_state::set_placeholder_state(file.as_fd(), state)
        .map_err(|err| state_error("write_state", &err))?;
    debug!("[PlaceholderFile] state updated, old={}, new={}", @public(old), @public(state));
    Ok(old)
}
