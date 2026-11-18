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

//! Wire contract of the provider's ICloudDiskServiceCallbackTable.
use ipc::parcel::{MsgOption, MsgParcel};
use ipc::remote::RemoteObj;
use ipc::IpcResult;

use crate::common::error_codes::{E_DEHYDRATE_DENIED, E_INVALID_ARG, E_IPC_FAILED};

const DESCRIPTOR: &str = "OHOS.Filemanagement.Dfs.ICloudDiskServiceCallbackTable";
const SERVICE_CMD_ON_CALLBACK: u32 = 0;
pub(crate) const FETCH_DATA: i32 = 0;
pub(crate) const CANCEL_FETCH_DATA: i32 = 1;
const DEHYDRATE: i32 = 2;

#[derive(Clone, Copy)]
pub(crate) enum CallbackRequest {
    Fetch { key: u64, priority: i32 },
    Cancel { key: u64 },
    Dehydrate,
}

impl CallbackRequest {
    fn write(self, parcel: &mut MsgParcel, folder: &str, path: &str) -> IpcResult<()> {
        let (kind, key) = match self {
            Self::Fetch { key, .. } => (FETCH_DATA, Some(key)),
            Self::Cancel { key } => (CANCEL_FETCH_DATA, Some(key)),
            Self::Dehydrate => (DEHYDRATE, None),
        };
        parcel.write_interface_token(DESCRIPTOR)?;
        parcel.write(folder)?;
        parcel.write(&kind)?;
        let bytes = key.map(u64::to_le_bytes);
        let key_bytes: &[u8] = bytes.as_ref().map_or(&[], |v| &v[..]);
        parcel.write(key_bytes)?;
        parcel.write(path)?;
        if let Self::Fetch { priority, .. } = self {
            parcel.write(&priority)?;
        }
        Ok(())
    }

    pub(crate) fn send(self, remote: &RemoteObj, folder: &str, path: &str) -> Result<(), i32> {
        let mut data = MsgParcel::new();
        self.write(&mut data, folder, path)
            .map_err(|_| E_INVALID_ARG)?;
        let mut option = MsgOption::new();
        let needs_reply = matches!(self, Self::Dehydrate);
        if !needs_reply {
            option.set_async();
        }
        let mut reply = remote
            .send_request_ext(SERVICE_CMD_ON_CALLBACK, &mut data, option)
            .map_err(|_| E_IPC_FAILED)?;
        if needs_reply && !reply.read::<bool>().map_err(|_| E_IPC_FAILED)? {
            return Err(E_DEHYDRATE_DENIED);
        }
        Ok(())
    }
}
