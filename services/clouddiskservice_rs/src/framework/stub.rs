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

//! C++ IDL stub equivalent: validate descriptor, read arguments, dispatch and
//! write the business status before any output. A nonzero business status is
//! returned inside a successful Binder reply, not as a transport failure.
//! Internal IPC errors use IpcResult and are normalized to Failed (-1) at the
//! RemoteStub boundary. Successful Binder transactions always return zero;
//! IpcStatusCode's discriminants are not the C++ transport status contract.

use ipc::parcel::{MsgParcel, Serialize};
use ipc::remote::RemoteStub;
use ipc::{IpcResult, IpcStatusCode, Skeleton};

use super::interface::{CloudDiskServiceIpcCode, ICloudDiskService, DESCRIPTOR};
use crate::common::error_codes::E_OK;
use crate::common::parcel::MAX_LIST_SIZE;
use crate::common::types::{ChangesResult, PlaceholderCustomInfo};
use crate::service::CloudDiskService;

impl CloudDiskService {
    fn handle_request(
        &self,
        code: u32,
        data: &mut MsgParcel,
        reply: &mut MsgParcel,
    ) -> IpcResult<()> {
        use CloudDiskServiceIpcCode::*;

        let token = data.read_interface_token()?;
        if token != DESCRIPTOR {
            error!(
                "[OnRemoteRequest] invalid interface token, code={}",
                @public(code),
            );
            return Err(IpcStatusCode::Failed);
        }
        let command = CloudDiskServiceIpcCode::try_from(code)?;
        match command {
            RegisterSyncFolderChangesInner => {
                let folder = data.read_string16()?;
                let remote = data.read_remote()?;
                write_status(
                    reply,
                    self.register_sync_folder_changes_inner(&folder, &remote),
                )
            }
            UnregisterSyncFolderChangesInner => {
                let folder = data.read_string16()?;
                write_status(reply, self.unregister_sync_folder_changes_inner(&folder))
            }
            RegisterCallbackTableInner => {
                let folder = data.read_string16()?;
                let remote = data.read_remote()?;
                write_status(reply, self.register_callback_table_inner(&folder, &remote))
            }
            UnregisterCallbackTableInner => {
                let folder = data.read_string16()?;
                write_status(reply, self.unregister_callback_table_inner(&folder))
            }
            GetSyncFolderChangesInner => {
                let folder = data.read_string16()?;
                let count = data.read()?;
                let start_usn = data.read()?;
                let mut result = ChangesResult::default();
                let ret =
                    self.get_sync_folder_changes_inner(&folder, count, start_usn, &mut result);
                write_result(reply, ret, &result)
            }
            SetFileSyncStatesInner => {
                let folder = data.read_string16()?;
                let mut states = Vec::new();
                for _ in 0..read_count(data)? {
                    states.push(data.read()?);
                }
                let mut result = Vec::new();
                let ret = self.set_file_sync_states_inner(&folder, &states, &mut result);
                write_list_result(reply, ret, &result)
            }
            GetFileSyncStatesInner => {
                let folder = data.read_string16()?;
                let mut paths = Vec::new();
                for _ in 0..read_count(data)? {
                    paths.push(data.read_string16()?);
                }
                let mut result = Vec::new();
                let ret = self.get_file_sync_states_inner(&folder, &paths, &mut result);
                write_list_result(reply, ret, &result)
            }
            CreatePlaceholderFileInner => {
                let folder = data.read_string16()?;
                let relative_path = data.read_string16()?;
                let info = data.read()?;
                let custom_info = data.read()?;
                write_status(
                    reply,
                    self.create_placeholder_file_inner(
                        &folder,
                        &relative_path,
                        &info,
                        &custom_info,
                    ),
                )
            }
            IsPlaceholderFileInner => {
                let folder = data.read_string16()?;
                let path = data.read_string16()?;
                let mut result = false;
                let ret = self.is_placeholder_file_inner(&folder, &path, &mut result);
                write_result(reply, ret, &result)
            }
            RegisterSyncFolderInner => {
                let user_id = data.read()?;
                let bundle_name = data.read_string16()?;
                let path = data.read_string16()?;
                write_status(
                    reply,
                    self.register_sync_folder_inner(user_id, &bundle_name, &path),
                )
            }
            UnregisterSyncFolderInner => {
                let user_id = data.read()?;
                let bundle_name = data.read_string16()?;
                let path = data.read_string16()?;
                write_status(
                    reply,
                    self.unregister_sync_folder_inner(user_id, &bundle_name, &path),
                )
            }
            UnregisterForSaInner => {
                let path = data.read_string16()?;
                write_status(reply, self.unregister_for_sa_inner(&path))
            }
            ConvertPlaceholderToFileInner => {
                let folder = data.read_string16()?;
                let relative_path = data.read_string16()?;
                write_status(
                    reply,
                    self.convert_placeholder_to_file_inner(&folder, &relative_path),
                )
            }
            MarkFileAsPlaceholderInner => {
                let folder = data.read_string16()?;
                let relative_path = data.read_string16()?;
                write_status(
                    reply,
                    self.mark_file_as_placeholder_inner(&folder, &relative_path),
                )
            }
            UnmarkPlaceholderFileInner => {
                let folder = data.read_string16()?;
                let relative_path = data.read_string16()?;
                write_status(
                    reply,
                    self.unmark_placeholder_file_inner(&folder, &relative_path),
                )
            }
            StartHydrationInner => {
                let folder = data.read_string16()?;
                let relative_path = data.read_string16()?;
                let priority = data.read()?;
                write_status(
                    reply,
                    self.start_hydration_inner(&folder, &relative_path, priority),
                )
            }
            CancelHydrationInner => {
                let folder = data.read_string16()?;
                let relative_path = data.read_string16()?;
                write_status(reply, self.cancel_hydration_inner(&folder, &relative_path))
            }
            ExecuteInner => {
                let request = data.read()?;
                write_status(reply, self.execute_inner(&request))
            }
            DehydrateInner => {
                let folder = data.read_string16()?;
                let relative_path = data.read_string16()?;
                write_status(reply, self.dehydrate_inner(&folder, &relative_path))
            }
            UpdatePlaceholderInner => {
                let folder = data.read_string16()?;
                let relative_path = data.read_string16()?;
                let metadata = data.read()?;
                let custom_info = data.read()?;
                write_status(
                    reply,
                    self.update_placeholder_inner(&folder, &relative_path, &metadata, &custom_info),
                )
            }
            GetPlaceholderCustomInfoInner => {
                let folder = data.read_string16()?;
                let relative_path = data.read_string16()?;
                let mut custom_info = PlaceholderCustomInfo::default();
                let ret = self.get_placeholder_custom_info_inner(
                    &folder,
                    &relative_path,
                    &mut custom_info,
                );
                write_result(reply, ret, &custom_info)
            }
            StartHydrationByPathInner => {
                let path = data.read_string16()?;
                let callback_type = data.read()?;
                let priority = data.read()?;
                let accessor_id = data.read()?;
                write_status(
                    reply,
                    self.start_hydration_by_path_inner(&path, callback_type, priority, accessor_id),
                )
            }
            DehydrateFileByPathInner => {
                let path = data.read_string16()?;
                write_status(reply, self.dehydrate_file_by_path_inner(&path))
            }
            RegisterProgressCallbackInner => {
                let accessor_id = data.read()?;
                let remote = data.read_remote()?;
                write_status(
                    reply,
                    self.register_progress_callback_inner(accessor_id, &remote),
                )
            }
            UnregisterProgressCallbackInner => {
                let accessor_id = data.read()?;
                write_status(reply, self.unregister_progress_callback_inner(accessor_id))
            }
            GetPlaceholderStateInner => {
                let folder = data.read_string16()?;
                let relative_path = data.read_string16()?;
                let mut result = 0;
                let ret = self.get_placeholder_state_inner(&folder, &relative_path, &mut result);
                write_result(reply, ret, &result)
            }
        }
    }
}

impl RemoteStub for CloudDiskService {
    fn descriptor(&self) -> &'static str {
        DESCRIPTOR
    }

    fn on_remote_request(&self, code: u32, data: &mut MsgParcel, reply: &mut MsgParcel) -> i32 {
        info!(
            "[OnRemoteRequest] code={}, pid={}, uid={}",
            @public(code),
            @public(Skeleton::calling_pid()),
            @public(Skeleton::calling_uid()),
        );
        match self.handle_request(code, data, reply) {
            Ok(()) => 0,
            Err(status) => {
                error!(
                    "[OnRemoteRequest] IPC failed, code={}, status={:?}",
                    @public(code),
                    @public(status),
                );
                // InvalidValue has discriminant 0 in the public Rust IPC
                // crate. Never cast arbitrary errors into a Binder status.
                IpcStatusCode::Failed as i32
            }
        }
    }
}

fn read_count(data: &mut MsgParcel) -> IpcResult<i32> {
    let count = data.read::<i32>()?;
    if !(0..=MAX_LIST_SIZE).contains(&count) {
        return Err(IpcStatusCode::InvalidValue);
    }
    Ok(count)
}

fn write_status(reply: &mut MsgParcel, status: i32) -> IpcResult<()> {
    reply.write(&status)
}

fn write_result<T: Serialize>(reply: &mut MsgParcel, status: i32, result: &T) -> IpcResult<()> {
    write_status(reply, status)?;
    if status == E_OK {
        reply.write(result)?;
    }
    Ok(())
}

fn write_list_result<T: Serialize>(
    reply: &mut MsgParcel,
    status: i32,
    result: &[T],
) -> IpcResult<()> {
    write_status(reply, status)?;
    if status == E_OK {
        if result.len() > MAX_LIST_SIZE as usize {
            return Err(IpcStatusCode::InvalidValue);
        }
        reply.write(&(result.len() as i32))?;
        for item in result {
            reply.write(item)?;
        }
    }
    Ok(())
}
