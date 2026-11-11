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

pub(crate) use bridge::{cancel_queue_task, clone_task_handle, wait_task, FfrtTaskHandle};
pub(crate) use bridge::{start_timer, FfrtTimer};

use std::sync::mpsc::{self, Receiver, TryRecvError};

// The completion channel lets OnStop drain tasks submitted through the CXX bridge.
pub(crate) struct Task {
    finished: Receiver<()>,
}

pub(crate) fn spawn(task: impl FnOnce() + Send + 'static) -> Task {
    let (finished, receiver) = mpsc::channel();
    bridge::submit_task(Box::new(TaskClosure {
        inner: Some(Box::new(move || {
            task();
            let _ = finished.send(());
        })),
    }));
    Task { finished: receiver }
}

pub(crate) fn submit_task_with_handle(
    task: impl FnOnce() + Send + 'static,
) -> cxx::UniquePtr<FfrtTaskHandle> {
    bridge::submit_task_with_handle(Box::new(TaskClosure {
        inner: Some(Box::new(task)),
    }))
}

pub(crate) fn submit_task_after(dependency: &FfrtTaskHandle, task: impl FnOnce() + Send + 'static) {
    bridge::submit_task_after(
        Box::new(TaskClosure {
            inner: Some(Box::new(task)),
        }),
        dependency,
    );
}

pub(crate) fn submit_delayed_task(
    task: impl FnOnce() + Send + 'static,
    delay_us: u64,
) -> cxx::UniquePtr<FfrtTaskHandle> {
    bridge::submit_delayed_task(
        Box::new(TaskClosure {
            inner: Some(Box::new(task)),
        }),
        delay_us,
    )
}

impl Task {
    pub(crate) fn is_finished(&self) -> bool {
        match self.finished.try_recv() {
            Ok(()) | Err(TryRecvError::Disconnected) => true,
            Err(TryRecvError::Empty) => false,
        }
    }

    pub(crate) fn wait(self) {
        let _ = self.finished.recv();
    }
}

// Only Send + 'static closures may cross into the FFRT worker pool.
// CXX requires its opaque Rust types to be public; fields remain private.
pub struct TaskClosure {
    inner: Option<Box<dyn FnOnce() + Send + 'static>>,
}

impl TaskClosure {
    fn run(&mut self) {
        if let Some(task) = self.inner.take() {
            task();
        }
    }
}

#[cxx::bridge(namespace = "OHOS::FileManagement::CloudDiskService")]
mod bridge {
    extern "Rust" {
        type TaskClosure;
        fn run(self: &mut TaskClosure);
    }

    // SAFETY: Submission transfers a Send + 'static closure to FFRT, which calls
    // it at most once and releases it when the task is destroyed. FFRT retains its
    // dependencies, and CloneTaskHandle adds a native task reference. WaitTask and
    // CancelQueueTask only borrow live handles for the duration of the call.
    // StartTimer retains a static function pointer until timer destruction,
    // which stops ticks and waits for a running callback before releasing storage.
    unsafe extern "C++" {
        include!("ffrt_bridge.h");

        // The function pointer has no borrowed environment. The timer's destructor
        // stops ticks and waits for an active tick before releasing native storage.
        type FfrtTimer;

        #[rust_name = "start_timer"]
        fn StartTimer(interval_ms: u64, callback: fn()) -> UniquePtr<FfrtTimer>;

        #[rust_name = "submit_task"]
        fn SubmitTask(task: Box<TaskClosure>);

        #[rust_name = "submit_task_with_handle"]
        fn SubmitTaskWithHandle(task: Box<TaskClosure>) -> UniquePtr<FfrtTaskHandle>;

        #[rust_name = "submit_task_after"]
        fn SubmitTaskAfter(task: Box<TaskClosure>, dependency: &FfrtTaskHandle);

        #[rust_name = "clone_task_handle"]
        fn CloneTaskHandle(handle: &FfrtTaskHandle) -> UniquePtr<FfrtTaskHandle>;

        #[rust_name = "wait_task"]
        fn WaitTask(handle: &FfrtTaskHandle);

        #[rust_name = "submit_delayed_task"]
        fn SubmitDelayedTask(task: Box<TaskClosure>, delay_us: u64) -> UniquePtr<FfrtTaskHandle>;

        #[namespace = "ffrt"]
        #[cxx_name = "task_handle"]
        type FfrtTaskHandle;

        #[rust_name = "cancel_queue_task"]
        fn CancelQueueTask(handle: &FfrtTaskHandle) -> i32;
    }

    impl UniquePtr<FfrtTaskHandle> {}
}

// SAFETY: FFRT task handles own a reference-counted task and can move between
// threads. CloudDiskService serializes access and destruction with its mutex.
unsafe impl Send for FfrtTaskHandle {}

// SAFETY: LogFileMgr serializes ownership of the timer. The callback is a static
// function; FFRT synchronizes destruction with callbacks running on its workers.
unsafe impl Send for FfrtTimer {}
