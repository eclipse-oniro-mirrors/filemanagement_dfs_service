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

//! Parcelable layouts from frameworks/native/clouddiskservice_kit_inner/src/
//! cloud_disk_common.cpp. Method arguments use UTF-16; strings inside these
//! objects use C++ WriteString (UTF-8). Each WriteParcelable adds an i32 marker,
//! whereas ChangeData items nested in ChangesResult have no individual marker.

use ipc::parcel::{Deserialize, MsgParcel, Serialize};
use ipc::{IpcResult, IpcStatusCode};

use super::types::{
    CallbackExecuteRequest, ChangeData, ChangesResult, FailedList, FileSyncState,
    PlaceholderCustomInfo, PlaceholderInfo, ResultList,
};

// Shared by Parcelable codecs and the IPC stub's list parameters/results.
pub(crate) const MAX_LIST_SIZE: i32 = 102_400;

const INVALID: IpcStatusCode = IpcStatusCode::InvalidValue;
const MAX_CALLBACK_REQUEST_KEY_SIZE: usize = 8;
const MAX_EXECUTE_DATA_SIZE: usize = 128 * 1024;

trait ParcelFields: Sized {
    fn write_fields(&self, parcel: &mut MsgParcel) -> IpcResult<()>;
    fn read_fields(parcel: &mut MsgParcel) -> IpcResult<Self>;
}

macro_rules! fields {
    ($ty:ident, $($field:ident: $field_ty:ty),+ $(,)?) => {
        impl ParcelFields for $ty {
            fn write_fields(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
                $(parcel.write(&self.$field)?;)+
                Ok(())
            }
            fn read_fields(parcel: &mut MsgParcel) -> IpcResult<Self> {
                Ok(Self { $($field: parcel.read::<$field_ty>()?,)+ })
            }
        }
    };
}

macro_rules! parcelable {
    ($ty:ident) => {
        impl Serialize for $ty {
            fn serialize(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
                parcel.write(&1_i32)?;
                self.write_fields(parcel)
            }
        }
        impl Deserialize for $ty {
            fn deserialize(parcel: &mut MsgParcel) -> IpcResult<Self> {
                if parcel.read::<i32>()? == 0 {
                    return Err(INVALID);
                }
                Self::read_fields(parcel)
            }
        }
    };
}

fields!(FileSyncState, path: String, state: i32);
parcelable!(FileSyncState);

fields!(ChangeData, update_sequence_number: u64, file_id: String,
    parent_file_id: String, relative_path: String, operation_type: u8,
    size: u64, mtime: u64, time_stamp: u64);

impl ParcelFields for ChangesResult {
    fn write_fields(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        if self.changes_data.len() > MAX_LIST_SIZE as usize {
            return Err(INVALID);
        }
        parcel.write(&self.next_usn)?;
        parcel.write(&i32::from(self.is_eof))?;
        parcel.write(&(self.changes_data.len() as i32))?;
        for change in &self.changes_data {
            change.write_fields(parcel)?;
        }
        Ok(())
    }

    fn read_fields(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let next_usn = parcel.read()?;
        let is_eof = parcel.read::<i32>()? != 0;
        let count = parcel.read::<i32>()?;
        if !(0..=MAX_LIST_SIZE).contains(&count) {
            return Err(INVALID);
        }
        let mut changes_data = Vec::new();
        for _ in 0..count {
            changes_data.push(ChangeData::read_fields(parcel)?);
        }
        Ok(Self {
            next_usn,
            is_eof,
            changes_data,
        })
    }
}
parcelable!(ChangesResult);

fields!(FailedList, path: String, error: i32);
parcelable!(FailedList);

impl ParcelFields for ResultList {
    fn write_fields(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        parcel.write(&i32::from(self.is_success))?;
        parcel.write(&self.state)?;
        parcel.write(&self.path)?;
        parcel.write(&self.error)
    }

    fn read_fields(parcel: &mut MsgParcel) -> IpcResult<Self> {
        Ok(Self {
            is_success: parcel.read::<i32>()? != 0,
            state: parcel.read()?,
            path: parcel.read()?,
            error: parcel.read()?,
        })
    }
}
parcelable!(ResultList);

fields!(PlaceholderInfo, logical_size: u64, atime_ms: u64, mtime_ms: u64);
parcelable!(PlaceholderInfo);

fields!(PlaceholderCustomInfo, data: Vec<u8>);
parcelable!(PlaceholderCustomInfo);

impl ParcelFields for CallbackExecuteRequest {
    fn write_fields(&self, parcel: &mut MsgParcel) -> IpcResult<()> {
        if self.req_key.is_empty()
            || self.req_key.len() > MAX_CALLBACK_REQUEST_KEY_SIZE
            || self.data.len() > MAX_EXECUTE_DATA_SIZE
        {
            return Err(INVALID);
        }
        parcel.write(&self.req_key)?;
        parcel.write(&self.sync_folder)?;
        parcel.write(&self.file_path)?;
        parcel.write(&self.callback_type)?;
        parcel.write(&self.offset)?;
        parcel.write(&self.size)?;
        parcel.write(&self.total_size)?;
        parcel.write(&self.data)?;
        parcel.write(&self.is_complete)
    }

    fn read_fields(parcel: &mut MsgParcel) -> IpcResult<Self> {
        let req_key = read_bounded_bytes(parcel, MAX_CALLBACK_REQUEST_KEY_SIZE)?;
        if req_key.is_empty() {
            return Err(INVALID);
        }
        Ok(Self {
            req_key,
            sync_folder: parcel.read()?,
            file_path: parcel.read()?,
            callback_type: parcel.read()?,
            offset: parcel.read()?,
            size: parcel.read()?,
            total_size: parcel.read()?,
            data: read_bounded_bytes(parcel, MAX_EXECUTE_DATA_SIZE)?,
            is_complete: parcel.read()?,
        })
    }
}
parcelable!(CallbackExecuteRequest);

fn read_bounded_bytes(parcel: &mut MsgParcel, max_size: usize) -> IpcResult<Vec<u8>> {
    let position = parcel.read_position();
    let size = parcel.read::<i32>()?;
    if size < 0 || size as usize > max_size {
        return Err(INVALID);
    }
    parcel.set_read_position(position).map_err(|_| INVALID)?;
    parcel.read()
}
