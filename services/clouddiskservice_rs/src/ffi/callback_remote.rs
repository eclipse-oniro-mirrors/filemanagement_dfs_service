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

//! Binder identity and removable death notifications shared by provider and progress callbacks.
//! This IPC SDK requires the original strong reference to identify the object and detach its listener.

use cxx::UniquePtr;
use ipc::parcel::{Deserialize, MsgParcel};
use ipc::remote::RemoteObj;
use ipc::{IpcResult, IpcStatusCode};

pub(crate) struct CallbackRemote(UniquePtr<bridge::CallbackRemote>);

impl Deserialize for CallbackRemote {
    fn deserialize(data: &mut MsgParcel) -> IpcResult<Self> {
        let remote = bridge::read_callback_remote(data.as_msg_parcel_mut());
        if remote.is_null() {
            error!("[CallbackRemote] missing Binder object");
            return Err(IpcStatusCode::Failed);
        }
        Ok(Self(remote))
    }
}

impl CallbackRemote {
    pub(crate) fn resolve_table_key(&mut self) -> usize {
        self.0.pin_mut().resolve_table_key()
    }

    pub(crate) fn resolve_progress_key(&mut self) -> usize {
        self.0.pin_mut().resolve_progress_key()
    }

    pub(crate) fn object(&self) -> Option<RemoteObj> {
        RemoteObj::from_sptr(self.0.clone_object())
    }

    pub(crate) fn watch_death(&mut self, on_death: fn(usize)) -> bool {
        self.0.pin_mut().watch_death(on_death)
    }
}

#[cxx::bridge(namespace = "OHOS::FileManagement::CloudDiskService")]
mod bridge {
    // SAFETY: ReadCallbackRemote retains the Binder object from the parcel.
    // The identity is used only as a key. The native handle detaches its own
    // recipient on drop and the death callback is a static Rust function.
    unsafe extern "C++" {
        include!("callback_remote.h");

        #[namespace = "OHOS"]
        type MessageParcel = ipc::parcel::MessageParcel;
        #[namespace = "OHOS"]
        type SptrIRemoteObject = ipc::cxx_share::SptrIRemoteObject;
        type CallbackRemote;

        #[rust_name = "read_callback_remote"]
        fn ReadCallbackRemote(parcel: Pin<&mut MessageParcel>) -> UniquePtr<CallbackRemote>;
        #[rust_name = "resolve_table_key"]
        fn ResolveTableKey(self: Pin<&mut CallbackRemote>) -> usize;
        #[rust_name = "resolve_progress_key"]
        fn ResolveProgressKey(self: Pin<&mut CallbackRemote>) -> usize;
        #[rust_name = "clone_object"]
        fn CloneObject(self: &CallbackRemote) -> UniquePtr<SptrIRemoteObject>;
        #[rust_name = "watch_death"]
        fn WatchDeath(self: Pin<&mut CallbackRemote>, on_death: fn(usize)) -> bool;
    }
}

// SAFETY: The Binder strong references can move between threads. Only the
// registry mutex accesses a stored handle; Binder notifications never borrow it.
unsafe impl Send for bridge::CallbackRemote {}
