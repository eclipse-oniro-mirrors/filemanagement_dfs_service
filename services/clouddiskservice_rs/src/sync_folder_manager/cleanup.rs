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

//! Best-effort file cleanup when a sync folder is unregistered.

use std::collections::HashSet;
use std::ffi::{CStr, CString};
use std::fs;
use std::io;
use std::os::unix::ffi::OsStrExt;
use std::path::{Path, PathBuf};

pub(crate) const CLOUD_DISK_FILE_SYNC_STATE_XATTR: &CStr =
    xattr_name(b"user.clouddisk.filesyncstate\0");
pub(crate) const CLOUD_DISK_PLACEHOLDER_COUNT_XATTR: &CStr =
    xattr_name(b"user.clouddisk.phcount\0");
const PLACEHOLDER_STATE_SHIFT: u8 = 5;
const UNHYDRATED: u8 = 1;
const PARTIALLY_HYDRATED: u8 = 2;
const FULLY_HYDRATED: u8 = 3;

const fn xattr_name(bytes: &[u8]) -> &CStr {
    match CStr::from_bytes_with_nul(bytes) {
        Ok(name) => name,
        Err(_) => panic!("xattr name must have a single trailing NUL"),
    }
}

pub(crate) fn remove_placeholder_files_batch(path: &str) {
    info!("[RemovePlaceholderFilesBatch] path={}", @private(path));
    let Some(root) = resolve_path(Path::new(path)) else {
        return;
    };
    // Process attributes after the children, matching the C++ recursive walk.
    let mut pending = vec![(root.clone(), false)];
    while let Some((directory, finish)) = pending.pop() {
        if finish {
            for attr in [
                CLOUD_DISK_PLACEHOLDER_COUNT_XATTR,
                CLOUD_DISK_FILE_SYNC_STATE_XATTR,
            ] {
                if let Err(err) = remove_attr(&directory, attr) {
                    log_error("directory xattr", &directory, &err);
                }
            }
            continue;
        }
        let entries = match fs::read_dir(&directory) {
            Ok(entries) => entries,
            Err(err) => {
                log_error("read directory", &directory, &err);
                continue;
            }
        };
        pending.push((directory.clone(), true));
        for entry in entries {
            let child = match entry {
                Ok(entry) => entry.path(),
                Err(err) => {
                    log_error("read entry", &directory, &err);
                    continue;
                }
            };
            match fs::symlink_metadata(&child) {
                Ok(stat) if stat.is_dir() => pending.push((child, false)),
                Ok(stat) if stat.is_file() => remove_placeholder_file(&root, &child),
                Ok(_) => {} // C++ lstat skips symlinks and other special files here.
                Err(err) => log_error("lstat", &child, &err),
            }
        }
    }
    info!("[RemovePlaceholderFilesBatch] finished, path={}", @private(path));
}

pub(crate) fn remove_xattr(path: &str, attr_name: &CStr) {
    info!("[RemoveXattr] path={}, attribute={}", @private(path), @public(attr_name.to_string_lossy()));
    let Some(root) = resolve_path(Path::new(path)) else {
        return;
    };
    let mut pending = vec![root.clone()];
    let mut visited = HashSet::new();
    while let Some(directory) = pending.pop() {
        if !visited.insert(directory.clone()) {
            continue;
        }
        let entries = match fs::read_dir(&directory) {
            Ok(entries) => entries,
            Err(err) => {
                log_error("read directory", &directory, &err);
                continue;
            }
        };
        for entry in entries {
            let child = match entry {
                Ok(entry) => entry.path(),
                Err(err) => {
                    log_error("read entry", &directory, &err);
                    continue;
                }
            };
            let Some(real_path) = resolve_inside_root(&root, &child) else {
                continue;
            };
            let stat = match fs::symlink_metadata(&real_path) {
                Ok(stat) => stat,
                Err(err) => {
                    log_error("lstat", &real_path, &err);
                    continue;
                }
            };
            if let Err(err) = remove_attr(&real_path, attr_name) {
                log_error("remove xattr", &real_path, &err);
            }
            if stat.is_dir() {
                pending.push(real_path);
            }
        }
    }
}

fn remove_placeholder_file(root: &Path, path: &Path) {
    let Some(real_path) = resolve_inside_root(root, path) else {
        return;
    };
    let state = match placeholder_state(&real_path) {
        Ok(state) => state,
        Err(err) => {
            log_error("read placeholder state", &real_path, &err);
            return;
        }
    };
    if state == FULLY_HYDRATED {
        info!("[RemovePlaceholderFilesSingle] keep fully hydrated file, path={}", @private(real_path.display()));
    } else if matches!(state, UNHYDRATED | PARTIALLY_HYDRATED) {
        if let Err(err) = fs::remove_file(&real_path) {
            log_error("unlink placeholder", &real_path, &err);
        } else {
            info!("[RemovePlaceholderFilesSingle] removed, state={}, path={}", @public(state), @private(real_path.display()));
        }
    }
}

fn resolve_path(path: &Path) -> Option<PathBuf> {
    match fs::canonicalize(path) {
        Ok(path) => Some(path),
        Err(err) => {
            log_error("realpath", path, &err);
            None
        }
    }
}

fn resolve_inside_root(root: &Path, path: &Path) -> Option<PathBuf> {
    let real_path = resolve_path(path)?;
    // Follow links only within this sync root; do not change unrelated files.
    if !real_path.starts_with(root) {
        warn!("[SyncFolderCleanup] resolved path outside root, path={}", @private(path.display()));
        return None;
    }
    Some(real_path)
}

fn c_path(path: &Path) -> io::Result<CString> {
    CString::new(path.as_os_str().as_bytes())
        .map_err(|_| io::Error::from_raw_os_error(libc::EINVAL))
}

fn placeholder_state(path: &Path) -> io::Result<u8> {
    let path = c_path(path)?;
    let mut value = 0u8;
    // SAFETY: Both strings are terminated and value is writable for one byte.
    let size = unsafe {
        libc::getxattr(
            path.as_ptr(),
            CLOUD_DISK_FILE_SYNC_STATE_XATTR.as_ptr(),
            (&mut value as *mut u8).cast(),
            1,
        )
    };
    if size == 1 {
        return Ok(value >> PLACEHOLDER_STATE_SHIFT);
    }
    if size >= 0 {
        return Err(io::Error::from_raw_os_error(libc::EINVAL));
    }
    let err = io::Error::last_os_error();
    if err.raw_os_error() == Some(libc::ENODATA) {
        Ok(0)
    } else {
        Err(err)
    }
}

fn remove_attr(path: &Path, attr_name: &CStr) -> io::Result<()> {
    let path = c_path(path)?;
    // SAFETY: Both pointers refer to valid terminated strings for this call.
    if unsafe { libc::removexattr(path.as_ptr(), attr_name.as_ptr()) } == -1 {
        let err = io::Error::last_os_error();
        if err.raw_os_error() != Some(libc::ENODATA) {
            return Err(err);
        }
    }
    Ok(())
}

fn log_error(operation: &str, path: &Path, err: &io::Error) {
    warn!("[SyncFolderCleanup] operation={}, path={}, error={}",
        @public(operation), @private(path.display()), @public(err));
}
