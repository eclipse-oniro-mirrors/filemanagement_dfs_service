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

//! Wire-compatible proxy for the existing C++ ICloudDiskServiceCallback stub.

use ipc::parcel::MsgParcel;
use ipc::remote::RemoteObj;
use ipc::{IpcResult, IpcStatusCode};

use crate::common::parcel::MAX_LIST_SIZE;
use crate::common::types::ChangeData;

const DESCRIPTOR: &str = "OHOS.Filemanagement.Dfs.ICloudDiskServiceCallback";
const SERVICE_CMD_ON_CHANGE_DATA: u32 = 0;

pub(crate) struct CloudDiskServiceCallbackProxy {
    pub(crate) remote: RemoteObj,
}

impl CloudDiskServiceCallbackProxy {
    pub(crate) fn new(remote: &RemoteObj) -> Self {
        Self {
            remote: remote.clone(),
        }
    }

    pub(crate) fn on_change_data(
        &self,
        sandbox_path: &str,
        changes: &[ChangeData],
    ) -> IpcResult<()> {
        if changes.len() > MAX_LIST_SIZE as usize {
            return Err(IpcStatusCode::InvalidValue);
        }
        info!("[OnChangeData] sync_folder={}, count={}, changes={}",
            @private(sandbox_path), @public(changes.len()), @private(format!("{changes:?}")));
        let mut data = MsgParcel::new();
        data.write_interface_token(DESCRIPTOR)?;
        // C++ callback arguments use WriteString, unlike IDL's UTF-16 parameters.
        data.write(sandbox_path)?;
        data.write(&(changes.len() as i32))?;
        for change in changes {
            data.write(change)?;
        }
        // Match MessageOption's default TF_SYNC. The callback has no reply payload.
        self.remote
            .send_request(SERVICE_CMD_ON_CHANGE_DATA, &mut data)?;
        Ok(())
    }
}
