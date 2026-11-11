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

//! HMDFS dentry pages and directory headers shared with the C++ service.

use std::collections::HashSet;
use std::fs::{File, Metadata};
use std::io;
use std::os::fd::AsRawFd;
use std::os::unix::fs::{FileExt, MetadataExt};
use std::path::Path;

use super::cache::open_cache_file;
use crate::sync_folder_manager;

const PAGE_SIZE: usize = 4096;
const HEADER_SIZE: usize = 36;
const ROOT_PARENT: [u8; 16] = *b"................";
const LEVELS: u32 = 32;
const PAGES_PER_BUCKET: u64 = 2;
const SLOTS_PER_PAGE: usize = 60;
const BYTES_PER_NAME_SLOT: usize = 16;
const DENTRY_SIZE: usize = 52;
const DENTRY_START: usize = 1 + 8;
const NAMES_START: usize = DENTRY_START + SLOTS_PER_PAGE * DENTRY_SIZE;
const VALID: u8 = 1;
const INVALID: u8 = 2;

pub(super) type RecordId = [u8; 16];

extern "C" {
    fn uuid_generate(out: *mut u8);
}

fn new_record_id() -> RecordId {
    let mut id = [0; 16];
    // SAFETY: uuid_generate writes exactly 16 bytes to the live output buffer.
    unsafe { uuid_generate(id.as_mut_ptr()) };
    id
}

#[derive(Clone, Copy)]
pub(super) struct DentryPosition {
    pub(super) group: u64,
    pub(super) slot: usize,
}

pub(super) struct Dentry {
    pub(super) position: DentryPosition,
    pub(super) record_id: RecordId,
    pub(super) old_placeholder: u8,
    pub(super) inserted: bool,
}

pub(super) struct MetaBase {
    pub(super) name: String,
    pub(super) record_id: RecordId,
    pub(super) size: u64,
    pub(super) mtime: u64,
}

pub(super) struct CloudDiskServiceMetaFile(File);

impl CloudDiskServiceMetaFile {
    pub(super) fn open(directory: &Path, inode: u64) -> io::Result<Self> {
        let inode = format!("{inode:016x}");
        let bucket = inode_bucket(inode.as_bytes());
        let path = directory.join(bucket.to_string()).join(&inode);
        debug!("[MetaFile][open] inode={}, bucket={}, path={}",
            @public(&inode), @public(bucket), @private(path.display()));
        let file = open_cache_file(&path)?;
        debug!("[MetaFile][open] completed, fd={}, inode={}, bucket={}",
            @public(file.as_raw_fd()), @public(&inode), @public(bucket));
        Ok(Self(file))
    }

    pub(super) fn init_root(&self) -> io::Result<()> {
        // RegisterSyncFolder assigns a new root record ID on every registration.
        let id = new_record_id();
        let hash = sync_folder_manager::dentry_hash(ROOT_PARENT);
        info!("[MetaFile][init-root] begin, fd={}, parent=................, record_id={}, hash={}",
            @public(self.0.as_raw_fd()), @private(format!("{id:02x?}")), @public(hash));
        self.write_header(ROOT_PARENT, id, hash)?;
        info!("[MetaFile][init-root] completed, fd={}", @public(self.0.as_raw_fd()));
        Ok(())
    }

    pub(super) fn init_child(&self, parent_inode: u64, id: RecordId, hash: u32) -> io::Result<()> {
        let mut parent = [0; 16];
        parent.copy_from_slice(format!("{parent_inode:016x}").as_bytes());
        self.write_header(parent, id, hash)
    }

    pub(super) fn record_id(&self) -> io::Result<RecordId> {
        let header = self
            .read_header()?
            .ok_or_else(|| io::Error::from_raw_os_error(libc::ENOENT))?;
        let mut id = [0; 16];
        id.copy_from_slice(&header[16..32]);
        if id == [0; 16] {
            return Err(io::Error::from_raw_os_error(libc::ENOENT));
        }
        Ok(id)
    }

    pub(super) fn upsert(
        &self,
        name: &[u8],
        hash: u32,
        metadata: &Metadata,
        placeholder: u8,
        replacement_id: Option<RecordId>,
    ) -> io::Result<Dentry> {
        let slots = slots_for(name.len());
        if slots == 0 || slots > SLOTS_PER_PAGE {
            return Err(io::Error::from_raw_os_error(libc::ENAMETOOLONG));
        }
        let (mut page, position, inserted) = match self.find(name, hash)? {
            Some((page, position)) => (page, position, false),
            None => {
                let (page, position) = self.reserve(hash, slots)?;
                (page, position, true)
            }
        };

        let old_placeholder = if inserted {
            0
        } else {
            page.placeholder(position.slot)
        };
        let id = match replacement_id {
            Some(id) => id,
            None if inserted => new_record_id(),
            None => page.record_id(position.slot),
        };

        page.set_name(position.slot, name, slots);
        let entry = page.entry_mut(position.slot);
        entry[0] = VALID;
        entry[1..17].copy_from_slice(&id);
        entry[17..21].copy_from_slice(&hash.to_le_bytes());
        entry[23..25].copy_from_slice(&(name.len() as u16).to_le_bytes());
        entry[49] = if placeholder <= 3 { placeholder } else { 0 };
        update_metadata(entry, metadata);
        self.write_page(&page, position.group)?;
        Ok(Dentry {
            position,
            record_id: id,
            old_placeholder,
            inserted,
        })
    }

    pub(super) fn invalidate(&self, name: &[u8], hash: u32) -> io::Result<Dentry> {
        let (mut page, position) = self.lookup(name, hash)?;
        // Keep the slot occupied so older history records can still reference it.
        page.entry_mut(position.slot)[0] = INVALID;
        self.write_page(&page, position.group)?;
        Ok(Dentry {
            position,
            record_id: page.record_id(position.slot),
            old_placeholder: page.placeholder(position.slot),
            inserted: false,
        })
    }

    pub(super) fn update(&self, name: &[u8], hash: u32, metadata: &Metadata) -> io::Result<Dentry> {
        let (mut page, position) = self.lookup(name, hash)?;
        update_metadata(page.entry_mut(position.slot), metadata);
        self.write_page(&page, position.group)?;
        Ok(Dentry {
            position,
            record_id: page.record_id(position.slot),
            old_placeholder: page.placeholder(position.slot),
            inserted: false,
        })
    }

    pub(super) fn lookup_by_offset(&self, position: DentryPosition) -> io::Result<MetaBase> {
        if position.slot >= SLOTS_PER_PAGE {
            return Err(io::Error::from_raw_os_error(libc::EINVAL));
        }
        if position.group >= self.group_count()? {
            return Err(io::Error::from_raw_os_error(libc::ENOENT));
        }
        let page = self.read_page(position.group).map_err(|err| {
            error!("[LookupByOffset] read page failed, group={}, error={}", @public(position.group), @public(err));
            io::Error::from_raw_os_error(libc::ENOENT)
        })?;
        // Deleted/renamed entries remain occupied and are still referenced by
        // history.log. Do not restrict this lookup to VALID dentries.
        if !page.occupied(position.slot) {
            return Err(io::Error::from_raw_os_error(libc::ENOENT));
        }
        let entry = page.entry(position.slot);
        let mut size = [0; 8];
        size.copy_from_slice(&entry[25..33]);
        let mut mtime = [0; 8];
        mtime.copy_from_slice(&entry[33..41]);
        Ok(MetaBase {
            name: page.name(position.slot)?,
            record_id: page.record_id(position.slot),
            size: u64::from_le_bytes(size),
            mtime: u64::from_le_bytes(mtime),
        })
    }

    // Corresponds to MetaFileMgr::GetRelativePath. Parent dentries are found by
    // record ID in the cache, so the queried child need not exist on disk.
    pub(super) fn get_relative_path(&self, directory: &Path) -> io::Result<String> {
        let mut header = self
            .read_header()?
            .ok_or_else(|| io::Error::from_raw_os_error(libc::ENOENT))?;
        let mut names = Vec::new();
        let mut visited = HashSet::new();
        while header[..16] != ROOT_PARENT {
            let parent_name = std::str::from_utf8(&header[..16])
                .map_err(|_| io::Error::from_raw_os_error(libc::EINVAL))?;
            let inode = u64::from_str_radix(parent_name, 16)
                .map_err(|_| io::Error::from_raw_os_error(libc::EINVAL))?;
            if !visited.insert(inode) {
                return Err(io::Error::from_raw_os_error(libc::ELOOP));
            }
            let parent = Self::open(directory, inode)?;
            let mut id = [0; 16];
            id.copy_from_slice(&header[16..32]);
            let mut hash = [0; 4];
            hash.copy_from_slice(&header[32..36]);
            names.push(parent.lookup_name_by_record_id(&id, u32::from_le_bytes(hash))?);
            header = parent
                .read_header()?
                .ok_or_else(|| io::Error::from_raw_os_error(libc::ENOENT))?;
        }
        let mut path = String::from("/");
        for name in names.into_iter().rev() {
            path.push_str(&name);
            path.push('/');
        }
        Ok(path)
    }

    fn lookup_name_by_record_id(&self, id: &RecordId, hash: u32) -> io::Result<String> {
        let groups = self.group_count()?;
        for level in 0..LEVELS {
            let first = bucket_start(level, hash);
            if first >= groups {
                break;
            }
            for group in first..(first + PAGES_PER_BUCKET).min(groups) {
                let page = self.read_page(group)?;
                if let Some(slot) = page.find_by_record_id(id) {
                    return page.name(slot);
                }
            }
        }
        Err(io::Error::from_raw_os_error(libc::ENOENT))
    }

    fn read_header(&self) -> io::Result<Option<[u8; HEADER_SIZE]>> {
        let mut header = [0; HEADER_SIZE];
        match self.0.read_exact_at(&mut header, 0) {
            Ok(()) => Ok(Some(header)),
            Err(err) if err.kind() == io::ErrorKind::UnexpectedEof => Ok(None),
            Err(err) => Err(err),
        }
    }

    fn write_header(&self, parent: [u8; 16], id: RecordId, hash: u32) -> io::Result<()> {
        let metadata = self.0.metadata().map_err(|err| {
            error!("[MetaFile][fstat] failed, fd={}, errno={:?}, error={}",
                @public(self.0.as_raw_fd()), @public(err.raw_os_error()), @public(&err));
            err
        })?;
        if metadata.len() < PAGE_SIZE as u64 {
            info!("[MetaFile][resize] fd={}, old_size={}, new_size={}",
                @public(self.0.as_raw_fd()), @public(metadata.len()), @public(PAGE_SIZE));
            self.0.set_len(PAGE_SIZE as u64).map_err(|err| {
                error!("[MetaFile][resize] failed, fd={}, errno={:?}, error={}",
                    @public(self.0.as_raw_fd()), @public(err.raw_os_error()), @public(&err));
                err
            })?;
        }
        let mut header = [0; HEADER_SIZE];
        header[..16].copy_from_slice(&parent);
        header[16..32].copy_from_slice(&id);
        header[32..36].copy_from_slice(&hash.to_le_bytes());
        self.0.write_all_at(&header, 0).map_err(|err| {
            error!("[MetaFile][write-header] failed, fd={}, bytes={}, errno={:?}, error={}",
                @public(self.0.as_raw_fd()), @public(HEADER_SIZE), @public(err.raw_os_error()), @public(&err));
            err
        })?;
        debug!("[MetaFile][write-header] completed, fd={}, bytes={}, hash={}",
            @public(self.0.as_raw_fd()), @public(HEADER_SIZE), @public(hash));
        Ok(())
    }

    fn lookup(&self, name: &[u8], hash: u32) -> io::Result<(Page, DentryPosition)> {
        self.find(name, hash)?
            .ok_or_else(|| io::Error::from_raw_os_error(libc::ENOENT))
    }

    fn find(&self, name: &[u8], hash: u32) -> io::Result<Option<(Page, DentryPosition)>> {
        let groups = self.group_count()?;
        for level in 0..LEVELS {
            let first = bucket_start(level, hash);
            if first >= groups {
                break;
            }
            for group in first..(first + PAGES_PER_BUCKET).min(groups) {
                let page = self.read_page(group)?;
                if let Some(slot) = page.find(name, hash) {
                    return Ok(Some((page, DentryPosition { group, slot })));
                }
            }
        }
        Ok(None)
    }

    fn reserve(&self, hash: u32, slots: usize) -> io::Result<(Page, DentryPosition)> {
        let mut groups = self.group_count()?;
        for level in 0..LEVELS {
            let first = bucket_start(level, hash);
            let end = first + PAGES_PER_BUCKET;
            if end > groups {
                groups = ((1u64 << (level + 1)) - 1) * PAGES_PER_BUCKET;
                self.0.set_len(group_offset(groups))?;
            }
            for group in first..end {
                let page = self.read_page(group)?;
                if let Some(slot) = page.available(slots) {
                    return Ok((page, DentryPosition { group, slot }));
                }
            }
        }
        Err(io::Error::from_raw_os_error(libc::ENOSPC))
    }

    fn group_count(&self) -> io::Result<u64> {
        Ok((self.0.metadata()?.len() / PAGE_SIZE as u64).saturating_sub(1))
    }

    fn read_page(&self, group: u64) -> io::Result<Page> {
        let mut bytes = [0; PAGE_SIZE];
        self.0.read_exact_at(&mut bytes, group_offset(group))?;
        Ok(Page(bytes))
    }

    fn write_page(&self, page: &Page, group: u64) -> io::Result<()> {
        self.0.write_all_at(&page.0, group_offset(group))
    }
}

struct Page([u8; PAGE_SIZE]);

impl Page {
    fn name(&self, slot: usize) -> io::Result<String> {
        let entry = self.entry(slot);
        let len = usize::from(u16::from_le_bytes([entry[23], entry[24]]));
        let start = NAMES_START + slot * BYTES_PER_NAME_SLOT;
        if len == 0 || start + len > NAMES_START + SLOTS_PER_PAGE * BYTES_PER_NAME_SLOT {
            return Err(io::Error::from_raw_os_error(libc::EINVAL));
        }
        String::from_utf8(self.0[start..start + len].to_vec())
            .map_err(|_| io::Error::from_raw_os_error(libc::EILSEQ))
    }

    fn find_by_record_id(&self, id: &RecordId) -> Option<usize> {
        let mut slot = 0;
        while slot < SLOTS_PER_PAGE {
            if !self.occupied(slot) {
                slot += 1;
                continue;
            }
            let entry = self.entry(slot);
            if entry[0] & VALID != 0 && entry[1..17] == *id {
                return Some(slot);
            }
            let len = usize::from(u16::from_le_bytes([entry[23], entry[24]]));
            slot += slots_for(len).max(1);
        }
        None
    }

    fn find(&self, name: &[u8], hash: u32) -> Option<usize> {
        let mut slot = 0;
        while slot < SLOTS_PER_PAGE {
            if !self.occupied(slot) {
                slot += 1;
                continue;
            }
            let entry = self.entry(slot);
            let len = usize::from(u16::from_le_bytes([entry[23], entry[24]]));
            let start = NAMES_START + slot * BYTES_PER_NAME_SLOT;
            if entry[0] & VALID != 0
                && entry[17..21] == hash.to_le_bytes()
                && len == name.len()
                && start + len <= NAMES_START + SLOTS_PER_PAGE * BYTES_PER_NAME_SLOT
                && &self.0[start..start + len] == name
            {
                return Some(slot);
            }
            slot += slots_for(len).max(1);
        }
        None
    }

    fn available(&self, slots: usize) -> Option<usize> {
        let mut free = 0;
        for slot in 0..SLOTS_PER_PAGE {
            if self.occupied(slot) {
                free = 0;
            } else {
                free += 1;
                if free == slots {
                    return Some(slot + 1 - slots);
                }
            }
        }
        None
    }

    fn occupied(&self, slot: usize) -> bool {
        self.0[1 + slot / 8] & (1 << (slot % 8)) != 0
    }

    fn set_name(&mut self, slot: usize, name: &[u8], slots: usize) {
        let start = NAMES_START + slot * BYTES_PER_NAME_SLOT;
        self.0[start..start + name.len()].copy_from_slice(name);
        for bit in slot..slot + slots {
            self.0[1 + bit / 8] |= 1 << (bit % 8);
            if bit != slot {
                self.entry_mut(bit)[23..25].fill(0);
            }
        }
    }

    fn entry(&self, slot: usize) -> &[u8] {
        let start = DENTRY_START + slot * DENTRY_SIZE;
        &self.0[start..start + DENTRY_SIZE]
    }

    fn entry_mut(&mut self, slot: usize) -> &mut [u8] {
        let start = DENTRY_START + slot * DENTRY_SIZE;
        &mut self.0[start..start + DENTRY_SIZE]
    }

    fn record_id(&self, slot: usize) -> RecordId {
        let mut id = [0; 16];
        id.copy_from_slice(&self.entry(slot)[1..17]);
        id
    }

    fn placeholder(&self, slot: usize) -> u8 {
        let state = self.entry(slot)[49];
        if state <= 3 {
            state
        } else {
            0
        }
    }
}

fn bucket_start(level: u32, hash: u32) -> u64 {
    let buckets = 1u64 << level;
    (buckets - 1 + u64::from(hash) % buckets) * PAGES_PER_BUCKET
}

fn group_offset(group: u64) -> u64 {
    (1 + group) * PAGE_SIZE as u64
}

fn slots_for(len: usize) -> usize {
    len / BYTES_PER_NAME_SLOT + usize::from(len % BYTES_PER_NAME_SLOT != 0)
}

fn update_metadata(entry: &mut [u8], metadata: &Metadata) {
    entry[21..23].copy_from_slice(&(metadata.mode() as u16).to_le_bytes());
    entry[25..33].copy_from_slice(&metadata.size().to_le_bytes());
    entry[33..41].copy_from_slice(&(metadata.mtime() as u64).to_le_bytes());
    entry[41..49].copy_from_slice(&(metadata.atime() as u64).to_le_bytes());
}

fn inode_bucket(inode: &[u8]) -> u32 {
    (u32::from(inode[0]) + u32::from(inode[inode.len() / 2]) + u32::from(inode[inode.len() - 1]))
        % 32
}
