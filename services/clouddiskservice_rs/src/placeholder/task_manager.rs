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

//! Provider hydration task queue, cancellation records and idle deadlines.

use std::cmp::Reverse;
use std::collections::{HashMap, VecDeque};
use std::sync::{Arc, Condvar, Mutex, MutexGuard, OnceLock};
use std::time::{Duration, Instant};

use super::callback_manager::{CallbackEntry, PlaceholderCallbackManager};
use super::file::{ExistingFile, TaskKey};
use super::hydration::{HydrationFile, HydrationWriter};
use super::{HydrateProgressOwner, PlaceholderProgressManager};
use crate::common::error_codes::{
    E_CALLBACK_NOT_REGISTERED, E_CANCELLED, E_HYDRATE_IN_PROGRESS, E_INVALID_ARG,
    E_NO_HYDRATION_IN_PROGRESS, E_TRY_AGAIN,
};
use crate::common::parcel::MAX_CALLBACK_REQUEST_KEY_SIZE;
use crate::common::types::{CallbackExecuteRequest, HydrateProgressState};
use crate::ffi::ffrt;
use crate::framework::{CallbackRequest, CANCEL_FETCH_DATA, FETCH_DATA};

const ACTIVE_PER_APP_USER: usize = 5;
const IDLE_TIMEOUT: Duration = Duration::from_secs(5 * 60);
const CANCELLATION_TTL: Duration = Duration::from_secs(60);
const CANCELLED_PER_APP: usize = 16;
const CANCELLED_GLOBAL: usize = 32;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) enum PlaceholderTaskCancelReason {
    UserRequest,
    Timeout,
    InternalError,
    Unregister,
    CallbackDied,
    UserSwitch,
    ServiceStop,
    DispatchFailed,
}

enum Phase {
    Pending,
    Active(HydrationWriter),
    Completed,
    Cancelled,
}

struct Task {
    id: u64,
    order: u64,
    priority: i32,
    target: ExistingFile,
    identity: (u64, u64),
    initial_size: u64,
    absolute_path: String,
    owner: Option<HydrateProgressOwner>,
    callback: Arc<CallbackEntry>,
    phase: Mutex<Phase>,
}

impl Task {
    fn notify_progress(
        &self,
        state: HydrateProgressState,
        sizes: (u64, u64),
        recipient: Option<HydrateProgressOwner>,
    ) {
        // An explicit accessor cancellation is reported to its caller, including app-started tasks.
        if let Some(owner) = recipient.or(self.owner) {
            PlaceholderProgressManager::get_instance().on_task_progress(
                self.id,
                self.target.user_id,
                owner,
                &self.absolute_path,
                state,
                sizes,
            );
        }
    }

    fn send(&self, request: CallbackRequest) -> Result<(), i32> {
        self.callback.send(
            request,
            &self.target.key.sync_folder,
            &self.target.key.file_path,
        )
    }
}

struct Cancellation {
    id: u64,
    bundle: String,
    key: TaskKey,
    expires: Instant,
}

#[derive(Default)]
struct Queue {
    tasks: HashMap<u64, Arc<Task>>,
    by_path: HashMap<TaskKey, u64>,
    // Only active tasks have idle deadlines. Pending tasks consume no fd or active slot.
    deadlines: HashMap<u64, Instant>,
    cancellations: VecDeque<Cancellation>,
    next_id: u64,
    next_order: u64,
    running: bool,
    dispatching: bool,
    dispatch_worker: Option<ffrt::Task>,
}

impl Queue {
    fn purge_expired(&mut self) {
        let now = Instant::now();
        self.cancellations.retain(|record| record.expires > now);
    }

    fn allocate_id(&mut self) -> u64 {
        self.purge_expired();
        loop {
            let id = self.next_id.max(1);
            self.next_id = id.wrapping_add(1);
            if !self.tasks.contains_key(&id) && !self.cancellations.iter().any(|r| r.id == id) {
                return id;
            }
        }
    }

    fn allocate_order(&mut self) -> Result<u64, i32> {
        let mut order = self.next_order.max(1);
        if order == u64::MAX {
            if !self.tasks.is_empty() {
                return Err(E_TRY_AGAIN);
            }
            order = 1;
        }
        self.next_order = order + 1;
        Ok(order)
    }

    fn next_pending(&self) -> Option<Arc<Task>> {
        let mut active = HashMap::new();
        for task in self.deadlines.keys().filter_map(|id| self.tasks.get(id)) {
            *active
                .entry((task.target.bundle.as_str(), task.target.user_id))
                .or_insert(0usize) += 1;
        }
        self.by_path
            .values()
            .filter_map(|id| self.tasks.get(id))
            .filter(|task| {
                !self.deadlines.contains_key(&task.id)
                    && active
                        .get(&(task.target.bundle.as_str(), task.target.user_id))
                        .copied()
                        .unwrap_or(0)
                        < ACTIVE_PER_APP_USER
            })
            .min_by_key(|task| (Reverse(task.priority), task.order))
            .cloned()
    }

    fn remove_path(&mut self, task: &Task) {
        if self.by_path.get(&task.target.key) == Some(&task.id) {
            self.by_path.remove(&task.target.key);
        }
    }

    fn remember_cancellation(&mut self, task: &Task) {
        self.purge_expired();
        if self
            .cancellations
            .iter()
            .filter(|r| r.bundle == task.target.bundle)
            .count()
            >= CANCELLED_PER_APP
        {
            if let Some(oldest) = self
                .cancellations
                .iter()
                .position(|r| r.bundle == task.target.bundle)
            {
                self.cancellations.remove(oldest);
            }
        }
        if self.cancellations.len() >= CANCELLED_GLOBAL {
            self.cancellations.pop_front();
        }
        self.cancellations.push_back(Cancellation {
            id: task.id,
            bundle: task.target.bundle.clone(),
            key: task.target.key.clone(),
            expires: Instant::now() + CANCELLATION_TTL,
        });
    }

    fn cancellation_error(
        &mut self,
        id: u64,
        bundle: &str,
        index: u32,
        req: &CallbackExecuteRequest,
    ) -> i32 {
        self.purge_expired();
        let Some(record) = self.cancellations.iter().find(|r| r.id == id) else {
            return E_NO_HYDRATION_IN_PROGRESS;
        };
        match validate_provider(&record.bundle, &record.key, bundle, index, req) {
            Ok(()) => E_CANCELLED,
            Err(ret) => ret,
        }
    }
}

fn validate_provider(
    bundle: &str,
    key: &TaskKey,
    caller: &str,
    index: u32,
    req: &CallbackExecuteRequest,
) -> Result<(), i32> {
    if caller != bundle || index != key.index {
        return Err(E_CALLBACK_NOT_REGISTERED);
    }
    if req.sync_folder != key.sync_folder || req.file_path != key.file_path {
        return Err(E_INVALID_ARG);
    }
    Ok(())
}

pub(crate) struct PlaceholderTaskManager {
    // Never wait for a task, do file I/O or send a callback with the queue locked.
    queue: Mutex<Queue>,
    deadlines_changed: Condvar,
    monitor: Mutex<Option<ffrt::Task>>,
}

impl PlaceholderTaskManager {
    pub(crate) fn get_instance() -> &'static Self {
        static INSTANCE: OnceLock<PlaceholderTaskManager> = OnceLock::new();
        INSTANCE.get_or_init(|| Self {
            queue: Mutex::default(),
            deadlines_changed: Condvar::new(),
            monitor: Mutex::new(None),
        })
    }

    fn queue(&self) -> MutexGuard<'_, Queue> {
        self.queue.lock().unwrap_or_else(|e| e.into_inner())
    }

    pub(crate) fn start_scheduler(&self) {
        let mut monitor = self.monitor.lock().unwrap_or_else(|e| e.into_inner());
        if monitor.is_some() {
            return;
        }
        self.queue().running = true;
        *monitor = Some(ffrt::spawn(|| Self::get_instance().deadline_loop()));
        info!("[PlaceholderTaskManager] scheduler started");
    }

    pub(crate) fn stop_scheduler(&self) {
        let mut monitor = self.monitor.lock().unwrap_or_else(|e| e.into_inner());
        let dispatch = {
            let mut q = self.queue();
            q.running = false;
            self.deadlines_changed.notify_all();
            q.dispatch_worker.take()
        };
        self.cancel_all_tasks(PlaceholderTaskCancelReason::ServiceStop);
        if let Some(worker) = dispatch {
            worker.wait();
        }
        if let Some(worker) = monitor.take() {
            worker.wait();
        }
        self.clear_cancellation_records();
        info!("[PlaceholderTaskManager] scheduler stopped");
    }

    pub(super) fn has_outstanding(&self, key: &TaskKey) -> bool {
        self.queue().by_path.contains_key(key)
    }

    pub(crate) fn create_task(
        &self,
        target: ExistingFile,
        priority: i32,
        accessor: Option<(HydrateProgressOwner, &str)>,
    ) -> Result<(), i32> {
        let _guard = target.lock_idle()?;
        // Keep only the identity in the pending queue. Activation reopens the file.
        let (identity, initial_size) = {
            let file = HydrationFile::open(&target)?;
            (file.identity(), file.logical_size())
        };
        let callback = PlaceholderCallbackManager::get_instance()
            .callback(&target.bundle, target.key.index)?;
        let mut q = self.queue();
        if !q.running {
            return Err(E_TRY_AGAIN);
        }
        // Unregister deactivates this exact entry before taking a snapshot of its tasks.
        if !callback.is_registered() {
            return Err(E_CALLBACK_NOT_REGISTERED);
        }
        if q.by_path.contains_key(&target.key) {
            return Err(E_HYDRATE_IN_PROGRESS);
        }
        let order = q.allocate_order()?;
        let id = q.allocate_id();
        info!("[StartHydration] queued, index={}, key={}, priority={}",
            @private(target.key.index), @private(id), @public(priority));
        let (owner, absolute_path) = match accessor {
            Some((owner, path)) => (Some(owner), path.to_owned()),
            None => (
                None,
                format!("{}/{}", target.key.sync_folder, target.key.file_path),
            ),
        };
        let task = Arc::new(Task {
            id,
            order,
            priority,
            target,
            identity,
            initial_size,
            absolute_path,
            owner,
            callback,
            phase: Mutex::new(Phase::Pending),
        });
        q.by_path.insert(task.target.key.clone(), id);
        task.notify_progress(HydrateProgressState::Pending, (0, initial_size), None);
        q.tasks.insert(id, task);
        Self::schedule(&mut q);
        Ok(())
    }

    fn schedule(q: &mut Queue) {
        if q.running && !q.dispatching && !q.tasks.is_empty() {
            q.dispatching = true;
            q.dispatch_worker = Some(ffrt::spawn(|| Self::get_instance().dispatch_loop()));
        }
    }

    fn dispatch_loop(&self) {
        loop {
            let task = {
                let mut q = self.queue();
                if let Some(task) = q.running.then(|| q.next_pending()).flatten() {
                    task
                } else {
                    q.dispatching = false;
                    return;
                }
            };
            self.activate(&task);
        }
    }

    fn activate(&self, task: &Task) {
        let _file = task.target.lock();
        let mut phase = task.phase.lock().unwrap_or_else(|e| e.into_inner());
        if !matches!(*phase, Phase::Pending) {
            return;
        }
        let file = match HydrationFile::open(&task.target) {
            Ok(file) if file.identity() == task.identity => file,
            Ok(_) => {
                warn!("[Hydration] file identity changed before dispatch, index={}", @private(task.target.key.index));
                self.cancel_locked(
                    task,
                    &mut phase,
                    PlaceholderTaskCancelReason::InternalError,
                    None,
                );
                return;
            }
            Err(ret) => {
                warn!("[Hydration] activation validation failed, index={}, ret={}",
                    @private(task.target.key.index), @public(ret));
                self.cancel_locked(
                    task,
                    &mut phase,
                    PlaceholderTaskCancelReason::InternalError,
                    None,
                );
                return;
            }
        };
        let writer = match file.activate(task.target.user_id, task.id) {
            Ok(writer) => writer,
            Err(ret) => {
                warn!("[Hydration] prepare output failed, ret={}", @public(ret));
                self.cancel_locked(
                    task,
                    &mut phase,
                    PlaceholderTaskCancelReason::InternalError,
                    None,
                );
                return;
            }
        };
        if let Err(ret) = task.send(CallbackRequest::Fetch {
            key: task.id,
            priority: task.priority,
        }) {
            warn!("[Hydration] fetch dispatch failed, ret={}", @public(ret));
            self.cancel_locked(
                task,
                &mut phase,
                PlaceholderTaskCancelReason::DispatchFailed,
                None,
            );
            return;
        }
        let sizes = writer.progress_sizes();
        *phase = Phase::Active(writer);
        self.refresh_deadline(task.id);
        task.notify_progress(HydrateProgressState::InProgress, sizes, None);
        info!("[Hydration] FETCH_DATA sent, key={}, priority={}", @private(task.id), @public(task.priority));
    }

    fn refresh_deadline(&self, id: u64) {
        self.queue()
            .deadlines
            .insert(id, Instant::now() + IDLE_TIMEOUT);
        self.deadlines_changed.notify_one();
    }

    fn expired_tasks(&self) -> Option<Vec<Arc<Task>>> {
        let mut q = self.queue();
        while q.running {
            let Some(earliest) = q.deadlines.values().min().copied() else {
                q = self
                    .deadlines_changed
                    .wait(q)
                    .unwrap_or_else(|e| e.into_inner());
                continue;
            };
            let now = Instant::now();
            if earliest > now {
                let (next, _) = self
                    .deadlines_changed
                    .wait_timeout(q, earliest - now)
                    .unwrap_or_else(|e| e.into_inner());
                q = next;
                continue;
            }
            return Some(
                q.deadlines
                    .iter()
                    .filter(|(_, deadline)| **deadline <= now)
                    .filter_map(|(id, _)| q.tasks.get(id).cloned())
                    .collect(),
            );
        }
        None
    }

    fn deadline_loop(&self) {
        while let Some(expired) = self.expired_tasks() {
            for task in expired {
                let mut phase = task.phase.lock().unwrap_or_else(|e| e.into_inner());
                // A successful Execute may have refreshed this deadline while we waited.
                let expired = self
                    .queue()
                    .deadlines
                    .get(&task.id)
                    .is_some_and(|d| *d <= Instant::now());
                if expired {
                    self.cancel_locked(
                        &task,
                        &mut phase,
                        PlaceholderTaskCancelReason::Timeout,
                        None,
                    );
                }
            }
        }
    }

    pub(crate) fn execute(
        &self,
        bundle: &str,
        index: u32,
        req: &CallbackExecuteRequest,
    ) -> Result<(), i32> {
        if req.req_key.is_empty()
            || req.req_key.len() > MAX_CALLBACK_REQUEST_KEY_SIZE
            || !matches!(req.callback_type, FETCH_DATA | CANCEL_FETCH_DATA)
        {
            return Err(E_INVALID_ARG);
        }
        // Short nonempty keys are valid on the wire, but never identify an issued task.
        let bytes = req
            .req_key
            .as_slice()
            .try_into()
            .map_err(|_| E_NO_HYDRATION_IN_PROGRESS)?;
        let id = u64::from_le_bytes(bytes);
        let task = {
            let mut q = self.queue();
            match q.tasks.get(&id).cloned() {
                Some(task) => task,
                None => return Err(q.cancellation_error(id, bundle, index, req)),
            }
        };
        validate_provider(&task.target.bundle, &task.target.key, bundle, index, req)?;
        // A user cancellation may copy staged bytes back after releasing the path index.
        let _copy_back_guard = (req.callback_type == CANCEL_FETCH_DATA).then(|| task.target.lock());
        let mut phase = task.phase.lock().unwrap_or_else(|e| e.into_inner());
        match &mut *phase {
            Phase::Pending => Err(E_TRY_AGAIN),
            Phase::Cancelled => Err(E_CANCELLED),
            Phase::Completed => Err(E_NO_HYDRATION_IN_PROGRESS),
            Phase::Active(_) if !task.callback.is_registered() => Err(E_CALLBACK_NOT_REGISTERED),
            Phase::Active(_) if req.callback_type == CANCEL_FETCH_DATA => {
                self.cancel_locked(
                    &task,
                    &mut phase,
                    PlaceholderTaskCancelReason::UserRequest,
                    None,
                );
                Ok(())
            }
            Phase::Active(writer) => {
                writer.execute(&task.target, req)?;
                let sizes = writer.progress_sizes();
                if req.is_complete {
                    writer.restore_times();
                    *phase = Phase::Completed;
                    self.erase(&task, Some(sizes));
                    info!("[Hydration] completed, key={}, total_size={}", @private(id), @public(req.total_size));
                } else {
                    self.refresh_deadline(id);
                    task.notify_progress(HydrateProgressState::InProgress, sizes, None);
                }
                Ok(())
            }
        }
    }

    pub(crate) fn cancel_task(
        &self,
        folder: &str,
        path: &str,
        index: u32,
        owner: Option<HydrateProgressOwner>,
    ) -> Result<(), i32> {
        let key = TaskKey::new(index, folder, path);
        let task = {
            let q = self.queue();
            q.by_path.get(&key).and_then(|id| q.tasks.get(id)).cloned()
        }
        .ok_or(E_NO_HYDRATION_IN_PROGRESS)?;
        let _copy_back_guard = task.target.lock();
        let mut phase = task.phase.lock().unwrap_or_else(|e| e.into_inner());
        if !self.cancel_locked(
            &task,
            &mut phase,
            PlaceholderTaskCancelReason::UserRequest,
            owner,
        ) {
            return Err(E_NO_HYDRATION_IN_PROGRESS);
        }
        Ok(())
    }

    pub(super) fn cancel_tasks_for_callback(
        &self,
        entry: &Arc<CallbackEntry>,
        reason: PlaceholderTaskCancelReason,
    ) {
        self.cancel_matching(reason, |task| Arc::ptr_eq(&task.callback, entry));
    }

    pub(crate) fn cancel_all_tasks(&self, reason: PlaceholderTaskCancelReason) {
        self.cancel_matching(reason, |_| true);
    }

    fn cancel_matching(
        &self,
        reason: PlaceholderTaskCancelReason,
        matches: impl Fn(&Task) -> bool,
    ) {
        let tasks: Vec<_> = self
            .queue()
            .tasks
            .values()
            .filter(|t| matches(t))
            .cloned()
            .collect();
        for task in tasks {
            let mut phase = task.phase.lock().unwrap_or_else(|e| e.into_inner());
            self.cancel_locked(&task, &mut phase, reason, None);
        }
    }

    fn cancel_locked(
        &self,
        task: &Task,
        phase: &mut Phase,
        reason: PlaceholderTaskCancelReason,
        owner: Option<HydrateProgressOwner>,
    ) -> bool {
        let (dispatched, sizes) = match phase {
            Phase::Pending => (false, (0, task.initial_size)),
            Phase::Active(writer) => (true, writer.progress_sizes()),
            _ => return false,
        };
        let previous = std::mem::replace(phase, Phase::Cancelled);
        let notify = matches!(
            reason,
            PlaceholderTaskCancelReason::UserRequest
                | PlaceholderTaskCancelReason::Timeout
                | PlaceholderTaskCancelReason::InternalError
        );
        let remember = !matches!(
            reason,
            PlaceholderTaskCancelReason::UserSwitch
                | PlaceholderTaskCancelReason::ServiceStop
                | PlaceholderTaskCancelReason::DispatchFailed
        );
        {
            let mut q = self.queue();
            // A new request for this path may arrive while CANCEL_FETCH_DATA is in flight.
            q.remove_path(task);
            task.notify_progress(HydrateProgressState::Cancelled, sizes, owner);
            if remember && (dispatched || notify) {
                q.remember_cancellation(task);
            }
        }
        if notify {
            if let Err(ret) = task.send(CallbackRequest::Cancel { key: task.id }) {
                warn!("[CancelHydration] notify failed, ret={}", @public(ret));
            }
        }
        if let Phase::Active(mut writer) = previous {
            if reason == PlaceholderTaskCancelReason::UserRequest {
                writer.keep_partial_data();
            }
            writer.restore_times();
        }
        self.erase(task, None);
        info!("[Hydration] cancelled, index={}, key={}, reason={:?}",
            @private(task.target.key.index), @private(task.id), @public(reason));
        true
    }

    fn erase(&self, task: &Task, completed_sizes: Option<(u64, u64)>) {
        let mut q = self.queue();
        q.tasks.remove(&task.id);
        q.remove_path(task);
        q.deadlines.remove(&task.id);
        if let Some(sizes) = completed_sizes {
            task.notify_progress(HydrateProgressState::Completed, sizes, None);
        }
        self.deadlines_changed.notify_one();
        Self::schedule(&mut q);
    }

    pub(crate) fn clear_cancellation_records(&self) {
        self.queue().cancellations.clear();
    }
}
