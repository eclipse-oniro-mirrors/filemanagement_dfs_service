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

//! history.log ring and superblock shared with the C++ service.

use std::fs::File;
use std::io;
use std::os::fd::AsRawFd;
use std::os::unix::fs::{FileExt, MetadataExt};
use std::path::Path;
use std::time::{SystemTime, UNIX_EPOCH};

use super::cache::open_cache_file;
use super::metafile::{DentryPosition, RecordId};
use crate::common::error_codes::E_INVALID_CHANGE_SEQUENCE;

const PAGE_SIZE: usize = 4096;
const PAGE_COUNT: u64 = 1024;
const ENTRIES_PER_PAGE: u64 = 64;
const ENTRY_SIZE: usize = 63;
const LOG_CAPACITY: u64 = PAGE_COUNT * ENTRIES_PER_PAGE;
const FILE_SIZE: u64 = (1 + PAGE_COUNT) * PAGE_SIZE as u64;
const SUPERBLOCK_MAGIC: u32 = 0x4344_5342;
const SUPERBLOCK_VERSION: u32 = 1;
const SUPERBLOCK_SIZE: usize = 64;

#[derive(Clone, Copy)]
#[repr(u8)]
pub(super) enum Operation {
    Create = 0,
    Delete = 1,
    MoveFrom = 2,
    MoveTo = 3,
    CloseWrite = 4,
    CloseModify = 6,
}

pub(super) struct LogBlock {
    pub(super) line: u64,
    pub(super) timestamp: u64,
    pub(super) parent_inode: u64,
    pub(super) parent_record_id: RecordId,
    pub(super) operation: u8,
    pub(super) position: DentryPosition,
}

pub(super) struct History {
    file: File,
    next_line: u64,
    reversed: bool,
}

impl History {
    pub(super) fn open(directory: &Path) -> io::Result<Self> {
        let path = directory.join("history.log");
        info!("[History][open] begin, path={}", @private(path.display()));
        let file = open_cache_file(&path)?;
        let mut history = Self {
            file,
            next_line: 0,
            reversed: false,
        };
        let metadata = history.file.metadata().map_err(|err| {
            error!("[History][fstat] failed, path={}, fd={}, errno={:?}, error={}",
                @private(path.display()), @public(history.file.as_raw_fd()),
                @public(err.raw_os_error()), @public(&err));
            err
        })?;
        let size = metadata.len();
        info!("[History][open] fd={}, size={}, expected_size={}, mode={:o}, uid={}, gid={}, path={}",
            @public(history.file.as_raw_fd()), @public(size), @public(FILE_SIZE),
            @public(metadata.mode()), @public(metadata.uid()), @public(metadata.gid()), @private(path.display()));
        if size < FILE_SIZE {
            info!("[History][resize] begin, old_size={}, new_size={}, path={}",
                @public(size), @public(FILE_SIZE), @private(path.display()));
            history.file.set_len(FILE_SIZE).map_err(|err| {
                error!("[History][resize] failed, path={}, errno={:?}, error={}",
                    @private(path.display()), @public(err.raw_os_error()), @public(&err));
                err
            })?;
        }
        if size == 0 {
            history.write_superblock()?;
            info!("[History][open] initialized empty file, next_line=0, reversed=false, path={}", @private(path.display()));
            return Ok(history);
        }

        let mut header = [0; SUPERBLOCK_SIZE];
        history.file.read_exact_at(&mut header, 0).map_err(|err| {
            error!("[History][read-superblock] failed, path={}, bytes={}, errno={:?}, error={}",
                @private(path.display()), @public(SUPERBLOCK_SIZE), @public(err.raw_os_error()), @public(&err));
            err
        })?;
        let magic = u32::from_le_bytes([header[0], header[1], header[2], header[3]]);
        let version = u32::from_le_bytes([header[4], header[5], header[6], header[7]]);
        info!("[History][read-superblock] magic={:#x}, version={}, next_line={}, reversed={}, path={}",
            @public(magic), @public(version), @public(read_u64(&header, 8)),
            @public(header[16]), @private(path.display()));
        if header[..4] == SUPERBLOCK_MAGIC.to_le_bytes()
            && header[4..8] == SUPERBLOCK_VERSION.to_le_bytes()
        {
            history.next_line = read_u64(&header, 8);
            history.reversed = header[16] != 0;
        } else {
            warn!("[History][recover] invalid superblock, expected_magic={:#x}, expected_version={}, path={}",
                @public(SUPERBLOCK_MAGIC), @public(SUPERBLOCK_VERSION), @private(path.display()));
            history.recover_superblock()?;
        }
        info!("[History][open] existing file loaded, next_line={}, reversed={}, path={}",
            @public(history.next_line), @public(history.reversed), @private(path.display()));
        Ok(history)
    }

    pub(super) fn append(
        &mut self,
        operation: Operation,
        parent_inode: u64,
        parent_id: RecordId,
        hash: u32,
        position: DentryPosition,
    ) -> io::Result<(u64, u64)> {
        let line = self.next_line;
        let slot = self.next_line % LOG_CAPACITY;
        let page = slot / ENTRIES_PER_PAGE;
        let start = entry_offset(slot % ENTRIES_PER_PAGE);
        let mut bytes = self.read_page(page)?;
        if read_u64(&bytes, start + 8) != 0 {
            bytes.fill(0);
        }
        let entry = &mut bytes[start..start + ENTRY_SIZE];
        entry.fill(0);
        let time = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default();
        entry[0..8].copy_from_slice(&self.next_line.to_le_bytes());
        let timestamp = time.as_millis().max(1) as u64;
        entry[8..16].copy_from_slice(&timestamp.to_le_bytes());
        entry[16..24].copy_from_slice(&parent_inode.to_le_bytes());
        entry[24..28].copy_from_slice(&hash.to_le_bytes());
        entry[28] = operation as u8;
        entry[29..37].copy_from_slice(&position.group.to_le_bytes());
        entry[37..41].copy_from_slice(&(position.slot as u32).to_le_bytes());
        entry[41..57].copy_from_slice(&parent_id);
        bytes[1] = bytes[1].saturating_add(1);
        self.file.write_all_at(&bytes, page_offset(page))?;
        self.next_line = self.next_line.wrapping_add(1);
        self.reversed |= self.next_line > LOG_CAPACITY;
        self.write_superblock()?;
        debug!("[History][append] fd={}, line={}, timestamp={}, operation={}, parent_inode={}, group={}, slot={}",
            @public(self.file.as_raw_fd()), @public(line), @public(timestamp), @public(operation as u8),
            @public(parent_inode), @public(position.group), @public(position.slot));
        Ok((line, timestamp))
    }

    pub(super) fn get_start_line(&self) -> u64 {
        if self.next_line <= LOG_CAPACITY && !self.reversed {
            return 0;
        }
        // C++ discards the whole oldest page when its first slot is reused.
        // Wrapping arithmetic also preserves its uint64_t sequence rollover.
        (self.next_line.wrapping_sub(1) & !(ENTRIES_PER_PAGE - 1))
            .wrapping_sub(LOG_CAPACITY)
            .wrapping_add(ENTRIES_PER_PAGE)
    }

    pub(super) fn read_log(&self, line: u64) -> Result<Option<LogBlock>, i32> {
        if line == self.next_line {
            return Ok(None);
        }
        let start = self.get_start_line();
        let valid = (line >= start && line < self.next_line)
            || (self.reversed
                && self.next_line < LOG_CAPACITY
                && (line >= start || line < self.next_line));
        if !valid {
            warn!("[ReadLogFile] invalid sequence, line={}, oldest={}, next_line={}",
                @public(line), @public(start), @public(self.next_line));
            return Err(E_INVALID_CHANGE_SEQUENCE);
        }
        let slot = line % LOG_CAPACITY;
        let bytes = self.read_page(slot / ENTRIES_PER_PAGE).map_err(|err| {
            error!("[ReadLogFile] read page failed, line={}, error={}", @public(line), @public(err));
            -1 // C++ ReadLogFile returns -1 when LoadCurrentPage fails.
        })?;
        let offset = entry_offset(slot % ENTRIES_PER_PAGE);
        let entry = &bytes[offset..offset + ENTRY_SIZE];
        if read_u64(entry, 0) != line || read_u64(entry, 8) == 0 {
            error!("[ReadLogFile] log slot does not match sequence, line={}", @public(line));
            return Err(E_INVALID_CHANGE_SEQUENCE);
        }
        let mut parent_record_id = [0; 16];
        parent_record_id.copy_from_slice(&entry[41..57]);
        let mut position = [0; 4];
        position.copy_from_slice(&entry[37..41]);
        Ok(Some(LogBlock {
            line: read_u64(entry, 0),
            timestamp: read_u64(entry, 8),
            parent_inode: read_u64(entry, 16),
            parent_record_id,
            operation: entry[28],
            position: DentryPosition {
                group: read_u64(entry, 29),
                slot: u32::from_le_bytes(position) as usize,
            },
        }))
    }

    fn read_page(&self, page: u64) -> io::Result<[u8; PAGE_SIZE]> {
        let mut bytes = [0; PAGE_SIZE];
        self.file.read_exact_at(&mut bytes, page_offset(page)).map_err(|err| {
            error!("[History][read-page] failed, fd={}, page={}, offset={}, errno={:?}, error={}",
                @public(self.file.as_raw_fd()), @public(page), @public(page_offset(page)),
                @public(err.raw_os_error()), @public(&err));
            err
        })?;
        Ok(bytes)
    }

    fn write_superblock(&self) -> io::Result<()> {
        let mut header = [0; SUPERBLOCK_SIZE];
        header[..4].copy_from_slice(&SUPERBLOCK_MAGIC.to_le_bytes());
        header[4..8].copy_from_slice(&SUPERBLOCK_VERSION.to_le_bytes());
        header[8..16].copy_from_slice(&self.next_line.to_le_bytes());
        header[16] = u8::from(self.reversed);
        self.file.write_all_at(&header, 0).map_err(|err| {
            error!("[History][write-superblock] failed, fd={}, next_line={}, reversed={}, errno={:?}, error={}",
                @public(self.file.as_raw_fd()), @public(self.next_line), @public(self.reversed),
                @public(err.raw_os_error()), @public(&err));
            err
        })
    }

    fn recover_superblock(&mut self) -> io::Result<()> {
        info!("[History][recover] scan pages, fd={}, page_count={}", @public(self.file.as_raw_fd()), @public(PAGE_COUNT));
        let first = entry_offset(0);
        let mut next_line = 0u64;
        for page in 0..PAGE_COUNT {
            let bytes = self.read_page(page)?;
            let line = read_u64(&bytes, first);
            if page != 0 && (read_u64(&bytes, first + 8) == 0 || line != next_line) {
                break;
            }
            let count = u64::from(bytes[1]).min(ENTRIES_PER_PAGE);
            next_line = line.wrapping_add(count);
        }
        let last = self.read_page(PAGE_COUNT - 1)?;
        self.next_line = next_line;
        self.reversed = next_line > LOG_CAPACITY
            || read_u64(&last, entry_offset(ENTRIES_PER_PAGE - 1) + 8) != 0;
        self.write_superblock()?;
        info!("[History][recover] completed, fd={}, next_line={}, reversed={}",
            @public(self.file.as_raw_fd()), @public(self.next_line), @public(self.reversed));
        Ok(())
    }
}

fn page_offset(page: u64) -> u64 {
    (page + 1) * PAGE_SIZE as u64
}

fn entry_offset(slot: u64) -> usize {
    2 + slot as usize * ENTRY_SIZE
}

fn read_u64(bytes: &[u8], offset: usize) -> u64 {
    let mut value = [0; 8];
    value.copy_from_slice(&bytes[offset..offset + 8]);
    u64::from_le_bytes(value)
}
