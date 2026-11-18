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

//! File sync-state xattrs, preserving the placeholder bits and ancestor counts.

use std::ffi::{CStr, CString};
use std::io;
use std::os::fd::{AsRawFd, BorrowedFd};
use std::os::unix::ffi::OsStrExt;
use std::path::Path;
use std::sync::Mutex;

pub(crate) const SYNC_STATE: &CStr = xattr_name(b"user.clouddisk.filesyncstate\0");
pub(crate) const PLACEHOLDER_COUNT: &CStr = xattr_name(b"user.clouddisk.phcount\0");
const STATE_SHIFT: u8 = 5;
const SYNC_STATE_MASK: u8 = 0x1f;
pub(crate) const NONE: u8 = 0;
pub(crate) const UNHYDRATED: u8 = 1;
pub(crate) const PARTIALLY_HYDRATED: u8 = 2;
pub(crate) const FULLY_HYDRATED: u8 = 3;

// Corresponds to C++ GetPlaceholderStateMutex: both parts of the shared state
// byte must use the same lock across read/modify/write operations.
static FILE_SYNC_STATE_MUTEX: Mutex<()> = Mutex::new(());

const fn xattr_name(bytes: &[u8]) -> &CStr {
    match CStr::from_bytes_with_nul(bytes) {
        Ok(name) => name,
        Err(_) => panic!("xattr name must have a single trailing NUL"),
    }
}

pub(crate) fn state(path: &Path) -> io::Result<u8> {
    Ok(get_file_sync_state_byte(path, true)? >> STATE_SHIFT)
}

pub(crate) fn get_file_sync_state_byte(path: &Path, missing_as_zero: bool) -> io::Result<u8> {
    let _guard = FILE_SYNC_STATE_MUTEX
        .lock()
        .unwrap_or_else(|err| err.into_inner());
    match read_attr::<1>(path, SYNC_STATE)? {
        Some(value) => Ok(value[0]),
        None if missing_as_zero => Ok(0),
        None => Err(io::Error::from_raw_os_error(libc::ENODATA)),
    }
}

pub(crate) fn get_sync_state_from_file_sync_state(state: u8) -> u8 {
    state & SYNC_STATE_MASK
}

pub(crate) fn set_file_sync_state(path: &Path, sync_state: u8) -> io::Result<()> {
    if sync_state > SYNC_STATE_MASK {
        return Err(io::Error::from_raw_os_error(libc::EINVAL));
    }
    let _guard = FILE_SYNC_STATE_MUTEX
        .lock()
        .unwrap_or_else(|err| err.into_inner());
    let old = read_attr::<1>(path, SYNC_STATE)?.unwrap_or_default()[0];
    let new = (old & !SYNC_STATE_MASK) | sync_state;
    write_attr(path, SYNC_STATE, &[new])
}

pub(crate) fn set_new_placeholder_state(fd: BorrowedFd<'_>) -> io::Result<()> {
    let _guard = FILE_SYNC_STATE_MUTEX
        .lock()
        .unwrap_or_else(|err| err.into_inner());
    let state = [UNHYDRATED << STATE_SHIFT];
    set_fd_xattr(fd, SYNC_STATE, &state)
}

pub(crate) fn placeholder_state(fd: BorrowedFd<'_>) -> io::Result<u8> {
    read_placeholder_state(fd, true)
}

pub(crate) fn read_placeholder_state(fd: BorrowedFd<'_>, missing_as_zero: bool) -> io::Result<u8> {
    let _guard = FILE_SYNC_STATE_MUTEX
        .lock()
        .unwrap_or_else(|err| err.into_inner());
    Ok(read_fd_state(fd, missing_as_zero)? >> STATE_SHIFT)
}

pub(crate) fn set_placeholder_state(fd: BorrowedFd<'_>, state: u8) -> io::Result<u8> {
    if state > FULLY_HYDRATED {
        return Err(io::Error::from_raw_os_error(libc::EINVAL));
    }
    let _guard = FILE_SYNC_STATE_MUTEX
        .lock()
        .unwrap_or_else(|err| err.into_inner());
    let old = read_fd_state(fd, true)?;
    let new = (state << STATE_SHIFT) | (old & SYNC_STATE_MASK);
    set_fd_xattr(fd, SYNC_STATE, &[new])?;
    Ok(old >> STATE_SHIFT)
}

// Callers hold FILE_SYNC_STATE_MUTEX across placeholder-state reads and writes.
fn read_fd_state(fd: BorrowedFd<'_>, missing_as_zero: bool) -> io::Result<u8> {
    let mut state = [0];
    match get_fd_xattr(fd, SYNC_STATE, &mut state) {
        Ok(1) => Ok(state[0]),
        Ok(_) => Err(io::Error::from_raw_os_error(libc::EINVAL)),
        Err(err) if missing_as_zero && err.raw_os_error() == Some(libc::ENODATA) => Ok(0),
        Err(err) => Err(err),
    }
}

pub(crate) fn get_fd_xattr(fd: BorrowedFd<'_>, attr: &CStr, value: &mut [u8]) -> io::Result<usize> {
    let out = if value.is_empty() {
        std::ptr::null_mut()
    } else {
        value.as_mut_ptr().cast()
    };
    // SAFETY: The descriptor is borrowed and attr is terminated. A zero length
    // with a null pointer probes the size. Otherwise out holds value.len() writable bytes.
    let size = unsafe { libc::fgetxattr(fd.as_raw_fd(), attr.as_ptr(), out, value.len()) };
    if size < 0 {
        return Err(io::Error::last_os_error());
    }
    Ok(size as usize)
}

pub(crate) fn set_fd_xattr(fd: BorrowedFd<'_>, attr: &CStr, value: &[u8]) -> io::Result<()> {
    // SAFETY: The descriptor is borrowed, attr is terminated, and value is readable for its length.
    let ret = unsafe {
        libc::fsetxattr(
            fd.as_raw_fd(),
            attr.as_ptr(),
            value.as_ptr().cast(),
            value.len(),
            0,
        )
    };
    if ret == -1 {
        return Err(io::Error::last_os_error());
    }
    Ok(())
}

pub(crate) fn is_counted(state: u8) -> bool {
    matches!(state, UNHYDRATED | PARTIALLY_HYDRATED)
}

pub(crate) fn adjust_ancestors(root: &Path, path: &Path, increase: bool) {
    for dir in ancestors(root, path) {
        match adjust_directory_count(dir, increase) {
            Ok(true) => {}
            Ok(false) => break,
            Err(err) => log_error(dir, &err),
        }
    }
}

pub(super) fn track_ancestors(root: &Path, path: &Path) {
    for dir in ancestors(root, path) {
        let result = {
            let _guard = FILE_SYNC_STATE_MUTEX
                .lock()
                .unwrap_or_else(|err| err.into_inner());
            read_attr::<1>(dir, SYNC_STATE).and_then(|value| {
                let old = value.unwrap_or_default()[0];
                if old >> STATE_SHIFT != 0 {
                    return Ok(false);
                }
                write_state(dir, old, FULLY_HYDRATED).map(|()| true)
            })
        };
        match result {
            Ok(true) => {}
            Ok(false) => break,
            Err(err) => log_error(dir, &err),
        }
    }
}

fn ancestors<'a>(root: &'a Path, path: &'a Path) -> impl Iterator<Item = &'a Path> {
    path.ancestors()
        .skip(1)
        .take_while(move |dir| dir.starts_with(root))
}

fn adjust_directory_count(path: &Path, increase: bool) -> io::Result<bool> {
    let _guard = FILE_SYNC_STATE_MUTEX
        .lock()
        .unwrap_or_else(|err| err.into_inner());
    let old = u32::from_le_bytes(read_attr(path, PLACEHOLDER_COUNT)?.unwrap_or_default());
    let new = if increase {
        old.checked_add(1)
            .ok_or_else(|| io::Error::from_raw_os_error(libc::EOVERFLOW))?
    } else {
        old.saturating_sub(1)
    };
    if new == 0 {
        remove_attr(path, PLACEHOLDER_COUNT)?;
    } else {
        write_attr(path, PLACEHOLDER_COUNT, &new.to_le_bytes())?;
    }
    if (old == 0) == (new == 0) {
        return Ok(false);
    }
    let old_state = read_attr::<1>(path, SYNC_STATE)?.unwrap_or_default()[0];
    write_state(
        path,
        old_state,
        if new == 0 { FULLY_HYDRATED } else { UNHYDRATED },
    )?;
    Ok(true)
}

fn write_state(path: &Path, old: u8, state: u8) -> io::Result<()> {
    write_attr(
        path,
        SYNC_STATE,
        &[(state << STATE_SHIFT) | (old & SYNC_STATE_MASK)],
    )
}

fn log_error(path: &Path, err: &io::Error) {
    if err.kind() != io::ErrorKind::NotFound {
        warn!("[SyncFolder][placeholder] path={}, error={}", path.display(), @public(err));
    }
}

fn c_path(path: &Path) -> io::Result<CString> {
    CString::new(path.as_os_str().as_bytes())
        .map_err(|_| io::Error::from_raw_os_error(libc::EINVAL))
}

fn read_attr<const N: usize>(path: &Path, attr: &CStr) -> io::Result<Option<[u8; N]>> {
    let path = c_path(path)?;
    let mut value = [0u8; N];
    // SAFETY: The path and attribute are terminated and value provides N writable bytes.
    let size =
        unsafe { libc::getxattr(path.as_ptr(), attr.as_ptr(), value.as_mut_ptr().cast(), N) };
    if size == N as isize {
        return Ok(Some(value));
    }
    if size >= 0 {
        return Err(io::Error::from_raw_os_error(libc::EINVAL));
    }
    let err = io::Error::last_os_error();
    if err.raw_os_error() == Some(libc::ENODATA) {
        Ok(None)
    } else {
        Err(err)
    }
}

fn write_attr(path: &Path, attr: &CStr, value: &[u8]) -> io::Result<()> {
    let path = c_path(path)?;
    // SAFETY: The path and attribute are terminated and value is readable for value.len() bytes.
    let result = unsafe {
        libc::setxattr(
            path.as_ptr(),
            attr.as_ptr(),
            value.as_ptr().cast(),
            value.len(),
            0,
        )
    };
    if result == -1 {
        return Err(io::Error::last_os_error());
    }
    Ok(())
}

fn remove_attr(path: &Path, attr: &CStr) -> io::Result<()> {
    let path = c_path(path)?;
    // SAFETY: Both pointers refer to valid terminated strings for this call.
    if unsafe { libc::removexattr(path.as_ptr(), attr.as_ptr()) } == -1 {
        let err = io::Error::last_os_error();
        if err.raw_os_error() != Some(libc::ENODATA) {
            return Err(err);
        }
    }
    Ok(())
}
