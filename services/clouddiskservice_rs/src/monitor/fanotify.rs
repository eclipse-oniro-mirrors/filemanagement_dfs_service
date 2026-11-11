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

//! Linux fanotify setup and checked decoding of FID events.

use std::ffi::{c_char, OsStr};
use std::fs::{self, File};
use std::io::{self, Read};
use std::os::fd::{AsRawFd, FromRawFd, OwnedFd, RawFd};
use std::os::unix::ffi::OsStrExt;
use std::path::PathBuf;

const MOUNT_PATH: &str = "/data/service";
const METADATA_LEN: usize = 24;
const INFO_HEADER_LEN: usize = 4;
const HANDLE_OFFSET: usize = INFO_HEADER_LEN + 8; // fsid follows the info header
const HANDLE_HEADER_LEN: usize = 8;
const MAX_HANDLE_SIZE: usize = 128;
const POLL_TIMEOUT_MS: i32 = 200;

extern "C" {
    fn open_by_handle_at(mount_fd: i32, handle: *mut FileHandle, flags: i32) -> i32;
}

#[repr(C)]
struct FileHandle {
    length: u32,
    kind: i32,
    bytes: [u8; MAX_HANDLE_SIZE],
}

pub(super) struct Fanotify {
    events: File,
    mount: File,
}

impl Fanotify {
    pub(super) fn new() -> io::Result<Self> {
        let mount = File::open(MOUNT_PATH)?;
        let init_flags = libc::FAN_CLASS_NOTIF
            | libc::FAN_REPORT_DFID_NAME
            | libc::FAN_UNLIMITED_QUEUE
            | libc::FAN_CLOEXEC
            | libc::FAN_NONBLOCK;
        // SAFETY: fanotify_init takes only flag values and returns an owned fd on success.
        let fd =
            unsafe { libc::fanotify_init(init_flags, (libc::O_RDONLY | libc::O_LARGEFILE) as u32) };
        let events = File::from(owned_fd(fd)?);

        let mask = libc::FAN_CREATE
            | libc::FAN_DELETE
            | libc::FAN_MOVED_FROM
            | libc::FAN_MOVED_TO
            | libc::FAN_ONDIR
            | libc::FAN_CLOSE_WRITE
            | libc::FAN_MODIFY;
        // SAFETY: The descriptors are live. A null pathname with this mount fd marks its filesystem.
        let result = unsafe {
            libc::fanotify_mark(
                events.as_raw_fd(),
                libc::FAN_MARK_ADD | libc::FAN_MARK_FILESYSTEM,
                mask,
                mount.as_raw_fd(),
                std::ptr::null::<c_char>(),
            )
        };
        if result == -1 {
            return Err(io::Error::last_os_error());
        }
        Ok(Self { events, mount })
    }

    pub(super) fn read(&mut self, buffer: &mut [u8]) -> io::Result<usize> {
        let mut fd = libc::pollfd {
            fd: self.events.as_raw_fd(),
            events: libc::POLLIN,
            revents: 0,
        };
        // SAFETY: The pollfd is writable and its fd is owned by self for this call.
        let result = unsafe { libc::poll(&mut fd, 1, POLL_TIMEOUT_MS) };
        if result < 0 {
            return Err(io::Error::last_os_error());
        }
        if result == 0 {
            return Ok(0);
        }
        if fd.revents & (libc::POLLERR | libc::POLLHUP | libc::POLLNVAL) != 0 {
            return Err(io::Error::from_raw_os_error(libc::EIO));
        }
        self.events.read(buffer)
    }

    pub(super) fn events<'a>(
        &'a self,
        mut bytes: &'a [u8],
    ) -> impl Iterator<Item = (u64, PathBuf)> + 'a {
        std::iter::from_fn(move || {
            while let Some((event, tail)) = next_event(bytes) {
                bytes = tail;
                let fd = i32::from_ne_bytes(event[16..20].try_into().ok()?);
                if fd >= 0 {
                    // FID events normally have FAN_NOFD. Own any fd the kernel supplies.
                    drop(owned_fd(fd));
                }
                if event[4] != libc::FANOTIFY_METADATA_VERSION {
                    continue;
                }
                let mask = u64::from_ne_bytes(event[8..16].try_into().ok()?);
                if mask & libc::FAN_Q_OVERFLOW != 0 {
                    warn!("[DiskMonitor] fanotify queue overflow");
                    continue;
                }
                let metadata_len = usize::from(u16::from_ne_bytes(event[6..8].try_into().ok()?));
                if metadata_len < METADATA_LEN {
                    continue;
                }
                if let Some(path) = event
                    .get(metadata_len..)
                    .and_then(|info| self.event_path(info))
                {
                    return Some((mask, path));
                }
            }
            None
        })
    }

    fn event_path(&self, mut info: &[u8]) -> Option<PathBuf> {
        while let Some(header) = info.get(..INFO_HEADER_LEN) {
            let len = usize::from(u16::from_ne_bytes([header[2], header[3]]));
            if len < INFO_HEADER_LEN {
                break;
            }
            let record = info.get(..len)?;
            if matches!(
                header[0],
                libc::FAN_EVENT_INFO_TYPE_FID
                    | libc::FAN_EVENT_INFO_TYPE_DFID_NAME
                    | libc::FAN_EVENT_INFO_TYPE_DFID
            ) {
                return self
                    .path_from_handle(record, header[0] == libc::FAN_EVENT_INFO_TYPE_DFID_NAME);
            }
            info = &info[len..];
        }
        None
    }

    fn path_from_handle(&self, record: &[u8], has_name: bool) -> Option<PathBuf> {
        let handle = record.get(HANDLE_OFFSET..)?;
        let length = u32::from_ne_bytes(handle.get(..4)?.try_into().ok()?);
        let kind = i32::from_ne_bytes(handle.get(4..HANDLE_HEADER_LEN)?.try_into().ok()?);
        let count = length as usize;
        if count > MAX_HANDLE_SIZE {
            return None;
        }
        let mut handle_arg = FileHandle {
            length,
            kind,
            bytes: [0; MAX_HANDLE_SIZE],
        };
        let end = HANDLE_HEADER_LEN + count;
        handle_arg.bytes[..count].copy_from_slice(handle.get(HANDLE_HEADER_LEN..end)?);
        // SAFETY: The aligned file_handle buffer contains the complete handle supplied by the
        // kernel and the filesystem descriptor stays live for the call.
        let fd = unsafe {
            open_by_handle_at(
                self.mount.as_raw_fd(),
                &mut handle_arg,
                libc::O_PATH | libc::O_CLOEXEC,
            )
        };
        let fd = owned_fd(fd).ok()?;
        let path = fs::read_link(format!("/proc/self/fd/{}", fd.as_raw_fd())).ok()?;
        if !has_name {
            return Some(path);
        }
        let name = handle.get(end..)?;
        let name = &name[..name.iter().position(|byte| *byte == 0)?];
        if name.contains(&b'/') || name == b".." {
            return None;
        }
        if name.is_empty() || name == b"." {
            return Some(path);
        }
        Some(path.join(OsStr::from_bytes(name)))
    }
}

fn next_event(bytes: &[u8]) -> Option<(&[u8], &[u8])> {
    let size = u32::from_ne_bytes(bytes.get(..4)?.try_into().ok()?) as usize;
    if size < METADATA_LEN || size > bytes.len() {
        return None;
    }
    Some(bytes.split_at(size))
}

fn owned_fd(fd: RawFd) -> io::Result<OwnedFd> {
    if fd < 0 {
        return Err(io::Error::last_os_error());
    }
    // SAFETY: The caller transfers a successful descriptor returned by the kernel exactly once.
    Ok(unsafe { OwnedFd::from_raw_fd(fd) })
}
