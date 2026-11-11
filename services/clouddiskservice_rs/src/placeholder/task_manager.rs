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

//! Lifecycle skeleton corresponding to PlaceholderTaskManager.

// Keep the C++ cancellation reasons available while task APIs are being ported.
#[allow(dead_code)]
#[derive(Clone, Copy, Debug)]
#[repr(i32)]
pub(crate) enum PlaceholderTaskCancelReason {
    UserRequest = 0,
    Timeout = 1,
    InternalError = 2,
    Unregister = 3,
    CallbackDied = 4,
    UserSwitch = 5,
    ServiceStop = 6,
    DispatchFailed = 7,
}

pub(crate) struct PlaceholderTaskManager {
    _private: (),
}

impl PlaceholderTaskManager {
    pub(crate) fn get_instance() -> &'static Self {
        static INSTANCE: PlaceholderTaskManager = PlaceholderTaskManager { _private: () };
        &INSTANCE
    }

    pub(crate) fn start_scheduler(&self) {
        info!("[PlaceholderTaskManager::StartScheduler] pending implementation");
        // TODO: Start task dispatch and timeout monitoring.
    }

    pub(crate) fn stop_scheduler(&self) {
        info!("[PlaceholderTaskManager::StopScheduler] pending implementation");
        // TODO: Stop scheduling, cancel outstanding tasks and wait for workers.
    }

    pub(crate) fn cancel_all_tasks(&self, reason: PlaceholderTaskCancelReason) {
        info!("[PlaceholderTaskManager::CancelAllTasks] pending implementation, reason={:?}",
            @public(reason));
        // TODO: Cancel all pending and running tasks with the supplied reason.
    }

    pub(crate) fn clear_cancellation_records(&self) {
        info!("[PlaceholderTaskManager::ClearCancellationRecords] pending implementation");
        // TODO: Clear cancellation records retained for late task replies.
    }
}
