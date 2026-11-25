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

//! Per-user staging for hydration chunks. Only completion or a user cancellation copies data back.
use std::ffi::{CStr, CString};
use std::fs::{self, DirBuilder, File, Permissions};
use std::io::{self, ErrorKind, Seek};
use std::os::fd::{AsRawFd, FromRawFd};
use std::os::unix::ffi::OsStrExt;
use std::os::unix::fs::{DirBuilderExt, FileExt, OpenOptionsExt, PermissionsExt};
use std::path::{Path, PathBuf};

const COPY_SIZE: u64 = 4 * 1024 * 1024;

fn directory(user_id: i32) -> PathBuf {
    PathBuf::from(format!(
        "/data/service/el2/{user_id}/clouddiskservice/hydrate_tmp"
    ))
}

fn open_directory(path: &Path) -> io::Result<File> {
    File::options()
        .read(true)
        .custom_flags(libc::O_PATH | libc::O_DIRECTORY | libc::O_NOFOLLOW)
        .open(path)
}

// Only newly created directories get the service's mode, matching ForceCreateDirectory.
fn ensure_directory(path: &Path) -> io::Result<File> {
    match open_directory(path) {
        Err(err) if err.kind() == ErrorKind::NotFound => {}
        result => return result,
    }
    if let Some(parent) = path.parent() {
        ensure_directory(parent)?;
    }
    match DirBuilder::new().mode(0o711).create(path) {
        Ok(()) => {
            if let Err(err) = fs::set_permissions(path, Permissions::from_mode(0o711)) {
                warn!("[HydrationTemp] set directory mode failed, error={}", @public(err));
            }
        }
        Err(err) if err.kind() == ErrorKind::AlreadyExists => {}
        Err(err) => return Err(err),
    }
    open_directory(path)
}

fn unlink_file(dir: &File, name: &CStr) -> io::Result<()> {
    // SAFETY: dir owns a live directory fd. All callers pass one NUL-terminated filename.
    if unsafe { libc::unlinkat(dir.as_raw_fd(), name.as_ptr(), 0) } == 0 {
        return Ok(());
    }
    let err = io::Error::last_os_error();
    if err.kind() == ErrorKind::NotFound {
        Ok(())
    } else {
        Err(err)
    }
}

pub(crate) fn clean_orphans(user_id: i32) {
    if user_id < 0 {
        return;
    }
    let path = directory(user_id);
    let (dir, entries) = match open_directory(&path).and_then(|dir| Ok((dir, fs::read_dir(&path)?)))
    {
        Ok(entries) => entries,
        Err(err) if err.kind() == ErrorKind::NotFound => return,
        Err(err) => {
            warn!("[HydrationTemp] open directory for cleanup failed, error={}", @public(err));
            return;
        }
    };
    for entry in entries {
        let result = entry.and_then(|entry| {
            let name = CString::new(entry.file_name().as_bytes())?;
            unlink_file(&dir, &name)
        });
        if let Err(err) = result {
            if err.kind() != ErrorKind::NotFound {
                warn!("[HydrationTemp] remove orphan failed, error={}", @public(err));
            }
        }
    }
}

pub(super) struct HydrationTempFile {
    file: File,
    size: u64,
}

impl HydrationTempFile {
    pub(super) fn create(user_id: i32, identity: (u64, u64), request_id: u64) -> io::Result<Self> {
        let dir = ensure_directory(&directory(user_id))?;
        // Aliases of the same inode can have distinct request keys. Each needs its own file.
        let name = CString::new(format!("{}_{}_{}.tmp", identity.0, identity.1, request_id))?;
        let flags =
            libc::O_RDWR | libc::O_CREAT | libc::O_EXCL | libc::O_CLOEXEC | libc::O_NOFOLLOW;
        // SAFETY: dir is pinned, name contains one NUL-terminated component, and O_CREAT gets a mode.
        let fd =
            unsafe { libc::openat(dir.as_raw_fd(), name.as_ptr(), flags, 0o600 as libc::c_uint) };
        if fd < 0 {
            return Err(io::Error::last_os_error());
        }
        // SAFETY: openat returned a fresh, uniquely owned file descriptor.
        let file = unsafe { File::from_raw_fd(fd) };
        // Only the task uses this fd. Unlink now so process death also releases staged bytes.
        unlink_file(&dir, &name)?;
        Ok(Self { file, size: 0 })
    }

    pub(super) fn has_data(&self) -> bool {
        self.size != 0
    }

    pub(super) fn write_at(&mut self, mut bytes: &[u8], mut offset: u64) -> io::Result<()> {
        while !bytes.is_empty() {
            match self.file.write_at(bytes, offset) {
                Ok(0) => return Err(ErrorKind::WriteZero.into()),
                Ok(n) => {
                    bytes = &bytes[n..];
                    offset += n as u64;
                }
                Err(err) if err.kind() == ErrorKind::Interrupted => continue,
                Err(err) => return Err(err),
            }
        }
        self.size = self.size.max(offset);
        Ok(())
    }

    pub(super) fn copy_to(&mut self, output: &mut File) -> io::Result<()> {
        self.file.rewind()?;
        output.rewind()?;
        let mut remaining = self.size;
        while remaining != 0 {
            let count = remaining.min(COPY_SIZE) as usize;
            // SAFETY: Both files own live fds throughout the call. A null offset uses
            // the input fd position; the task mutex serializes copies and rewinds.
            let n = unsafe {
                libc::sendfile(
                    output.as_raw_fd(),
                    self.file.as_raw_fd(),
                    std::ptr::null_mut(),
                    count,
                )
            };
            if n < 0 {
                let err = io::Error::last_os_error();
                if err.kind() == ErrorKind::Interrupted {
                    continue;
                }
                return Err(err);
            }
            if n == 0 {
                return Err(ErrorKind::UnexpectedEof.into());
            }
            remaining -= n as u64;
        }
        Ok(())
    }
}
