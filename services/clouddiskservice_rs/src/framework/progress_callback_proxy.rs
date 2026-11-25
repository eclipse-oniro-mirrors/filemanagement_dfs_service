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

//! Outbound ICloudDiskProgressCallback proxy.
use ipc::parcel::{MsgOption, MsgParcel};
use ipc::remote::RemoteObj;
use ipc::IpcResult;

use crate::common::types::HydrateProgress;

const DESCRIPTOR: &str = "OHOS.Filemanagement.Dfs.ICloudDiskProgressCallback";
const ON_PROGRESS: u32 = 0;

pub(crate) fn send_progress(remote: &RemoteObj, progress: &HydrateProgress) -> IpcResult<()> {
    let mut data = MsgParcel::new();
    data.write_interface_token(DESCRIPTOR)?;
    data.write(progress)?;
    let mut option = MsgOption::new();
    option.set_async();
    remote.send_request_ext(ON_PROGRESS, &mut data, option)?;
    Ok(())
}
