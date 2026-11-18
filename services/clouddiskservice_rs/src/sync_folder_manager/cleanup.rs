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

use std::ffi::CStr;
use std::fs::{self, File, Metadata, ReadDir};
use std::io;
use std::os::fd::{AsFd, AsRawFd};
use std::os::unix::fs::{MetadataExt, OpenOptionsExt};
use std::path::{Path, PathBuf};

use crate::sync_folder_service::file_state;

struct Directory {
    file: File,
    path: PathBuf,
    entries: ReadDir,
}

pub(crate) fn remove_placeholder_files_batch(path: &str) {
    info!("[RemovePlaceholderFilesBatch] path={}", @private(path));
    if let Err(err) = clean_root(path) {
        log_error("cleanup root", Path::new(path), &err);
    }
}

fn clean_root(path: &str) -> io::Result<()> {
    let (parent, leaf) = open_parent_no_follow(path)?;
    let Some(directory) = clean_entry(&fd_path(&parent).join(leaf), Path::new(path))? else {
        return Ok(());
    };
    // Keep only the current chain of parent descriptors alive. Directory xattrs
    // are removed after visiting the children, through the same opened directory.
    let mut stack = vec![directory];
    while let Some(dir) = stack.last_mut() {
        match dir.entries.next() {
            Some(Ok(entry)) => {
                let child = dir.path.join(entry.file_name());
                match clean_entry(&entry.path(), &child) {
                    Ok(Some(directory)) => stack.push(directory),
                    Ok(None) => {}
                    Err(err) => log_error("cleanup entry", &child, &err),
                }
            }
            Some(Err(err)) => log_error("read directory", &dir.path, &err),
            None => {
                remove_attr(&dir.file, &dir.path, file_state::PLACEHOLDER_COUNT);
                remove_attr(&dir.file, &dir.path, file_state::SYNC_STATE);
                stack.pop();
            }
        }
    }
    Ok(())
}

fn open_parent_no_follow(path: &str) -> io::Result<(File, &str)> {
    let invalid = || io::Error::from_raw_os_error(libc::EINVAL);
    let relative = path
        .trim_end_matches('/')
        .strip_prefix('/')
        .ok_or_else(invalid)?;
    let mut components = relative.split('/');
    let leaf = components.next_back().ok_or_else(invalid)?;
    if matches!(leaf, "" | "." | "..") {
        return Err(invalid());
    }
    let flags = libc::O_PATH | libc::O_DIRECTORY | libc::O_NOFOLLOW;
    let mut parent = File::options().read(true).custom_flags(flags).open("/")?;
    for name in components {
        if matches!(name, "" | "." | "..") {
            return Err(invalid());
        }
        parent = File::options()
            .read(true)
            .custom_flags(flags)
            .open(fd_path(&parent).join(name))?;
    }
    Ok((parent, leaf))
}

// The parent descriptor remains open throughout lookup, revalidation and unlink.
// This also lets read_dir enumerate the pinned directory on rk3568.
fn fd_path(file: &File) -> PathBuf {
    PathBuf::from(format!("/proc/self/fd/{}", file.as_raw_fd()))
}

fn clean_entry(anchored: &Path, path: &Path) -> io::Result<Option<Directory>> {
    let entry = fs::symlink_metadata(anchored)?;
    if entry.is_symlink() {
        warn!("[SyncFolderCleanup] skip symlink, path={}", @private(path.display()));
    } else if entry.is_dir() {
        let file = File::options()
            .read(true)
            .custom_flags(libc::O_DIRECTORY | libc::O_NOFOLLOW)
            .open(anchored)?;
        if same_inode(&entry, &file.metadata()?) {
            let entries = fs::read_dir(fd_path(&file))?;
            return Ok(Some(Directory {
                file,
                path: path.to_owned(),
                entries,
            }));
        }
        warn!("[SyncFolderCleanup] skip replaced directory, path={}", @private(path.display()));
    } else if entry.is_file() {
        clean_file(anchored, path, &entry)?;
    }
    Ok(None)
}

fn clean_file(anchored: &Path, path: &Path, entry: &Metadata) -> io::Result<()> {
    let file = File::options()
        .read(true)
        .custom_flags(libc::O_NOFOLLOW | libc::O_NONBLOCK)
        .open(anchored)?;
    let opened = file.metadata()?;
    if !valid_cleanup_file(entry, &opened, path) {
        return Ok(());
    }
    // A missing state is distinct from a valid NONE state. A failed read,
    // invalid state or failed unlink must leave the inode's attributes intact.
    match file_state::read_placeholder_state(file.as_fd(), false)? {
        file_state::UNHYDRATED | file_state::PARTIALLY_HYDRATED => {
            let current = fs::symlink_metadata(anchored)?;
            if valid_cleanup_file(&opened, &current, path) {
                fs::remove_file(anchored)?;
            }
        }
        file_state::NONE | file_state::FULLY_HYDRATED => {
            remove_attr(&file, path, file_state::SYNC_STATE);
            remove_attr(&file, path, file_state::PLACEHOLDER_COUNT);
        }
        state => warn!("[SyncFolderCleanup] skip invalid placeholder state, path={}, state={}",
            @private(path.display()), @public(state)),
    }
    Ok(())
}

fn same_inode(first: &Metadata, second: &Metadata) -> bool {
    first.dev() == second.dev() && first.ino() == second.ino()
}

fn valid_cleanup_file(expected: &Metadata, current: &Metadata, path: &Path) -> bool {
    if !current.is_file() || !same_inode(expected, current) {
        warn!("[SyncFolderCleanup] skip replaced entry, path={}", @private(path.display()));
        return false;
    }
    if current.nlink() > 1 {
        warn!("[SyncFolderCleanup] skip linked file, path={}, nlink={}",
            @private(path.display()), @public(current.nlink()));
        return false;
    }
    true
}

fn remove_attr(file: &File, path: &Path, attr: &CStr) {
    // SAFETY: file owns a live descriptor and attr is a NUL-terminated name.
    if unsafe { libc::fremovexattr(file.as_raw_fd(), attr.as_ptr()) } == -1 {
        let err = io::Error::last_os_error();
        if !matches!(err.raw_os_error(), Some(libc::ENODATA | libc::ENOENT)) {
            log_error("remove xattr", path, &err);
        }
    }
}

fn log_error(operation: &str, path: &Path, err: &io::Error) {
    warn!("[SyncFolderCleanup] operation={}, path={}, error={}",
        @public(operation), @private(path.display()), @public(err));
}
