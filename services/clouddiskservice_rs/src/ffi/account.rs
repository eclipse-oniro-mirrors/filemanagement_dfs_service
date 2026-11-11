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

use crate::account_status_listener::AccountStatusCallback;

pub(crate) use bridge::{subscribe_account_status, AccountStatusSubscription};

#[cxx::bridge(namespace = "OHOS::FileManagement::CloudDiskService")]
mod bridge {
    extern "Rust" {
        type AccountStatusCallback;
        fn on_state_changed(self: &AccountStatusCallback, switched: bool, user_id: i32);
        fn deactivate(self: &AccountStatusCallback);
    }

    // SAFETY: The native subscription owns the callback box. Stop deactivates the
    // callback before unsubscribing; in-flight notifications retain shared ownership.
    unsafe extern "C++" {
        include!("account_status_listener.h");

        type AccountStatusSubscription;

        #[rust_name = "subscribe_account_status"]
        fn SubscribeAccountStatus(
            callback: Box<AccountStatusCallback>,
            result: &mut i32,
        ) -> UniquePtr<AccountStatusSubscription>;

        #[rust_name = "stop"]
        fn Stop(self: Pin<&mut AccountStatusSubscription>) -> i32;
    }
}

// SAFETY: Only AccountStatusListener's mutex accesses the subscription. Native
// notifications retain their own shared subscriber and use a synchronized Rust callback.
unsafe impl Send for AccountStatusSubscription {}
