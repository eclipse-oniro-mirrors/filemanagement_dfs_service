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

//! System accessor subscriptions and ordered hydration progress delivery.
use std::collections::HashMap;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex, MutexGuard, OnceLock};
use std::time::{Duration, Instant};

use ipc::remote::RemoteObj;
use ipc::Skeleton;

use crate::common::error_codes::{
    E_CALLBACK_ALREADY_REGISTERED, E_CALLBACK_NOT_REGISTERED, E_INVALID_ARG, E_IPC_FAILED,
};
use crate::common::types::{HydrateProgress, HydrateProgressState};
use crate::ffi::{callback_remote::CallbackRemote, ffrt};
use crate::framework;

const MIN_INTERVAL: Duration = Duration::from_millis(500);

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub(crate) struct HydrateProgressOwner {
    token_id: u64,
    pid: i32,
    accessor_id: u64,
}

impl HydrateProgressOwner {
    pub(crate) fn calling(accessor_id: u64) -> Result<Self, i32> {
        if accessor_id == 0 {
            return Err(E_INVALID_ARG);
        }
        Ok(Self {
            token_id: Skeleton::calling_full_token_id(),
            pid: Skeleton::calling_pid() as i32,
            accessor_id,
        })
    }
}

struct Subscriber {
    remote: RemoteObj,
    active: AtomicBool,
}

struct Registration {
    user_id: i32,
    remote_id: usize,
    subscriber: Arc<Subscriber>,
    // Dropping the native handle detaches this subscription's death recipient.
    _watch: CallbackRemote,
}

#[derive(Default)]
struct Subscriptions {
    by_owner: HashMap<HydrateProgressOwner, Registration>,
    last_progress: HashMap<u64, (HydrateProgressState, Instant)>,
}

impl Subscriptions {
    fn remove(&mut self, owner: &HydrateProgressOwner) -> Option<Registration> {
        let registration = self.by_owner.remove(owner)?;
        registration
            .subscriber
            .active
            .store(false, Ordering::Release);
        Some(registration)
    }
}

pub(crate) struct PlaceholderProgressManager {
    subscriptions: Mutex<Subscriptions>,
}

impl PlaceholderProgressManager {
    pub(crate) fn get_instance() -> &'static Self {
        static INSTANCE: OnceLock<PlaceholderProgressManager> = OnceLock::new();
        INSTANCE.get_or_init(|| Self {
            subscriptions: Mutex::default(),
        })
    }

    fn subscriptions(&self) -> MutexGuard<'_, Subscriptions> {
        self.subscriptions
            .lock()
            .unwrap_or_else(|err| err.into_inner())
    }

    pub(crate) fn register(
        &self,
        owner: HydrateProgressOwner,
        user_id: i32,
        mut callback: CallbackRemote,
    ) -> Result<(), i32> {
        let remote_id = callback.resolve_progress_key();
        if user_id < 0 || remote_id == 0 {
            return Err(E_INVALID_ARG);
        }
        let mut state = self.subscriptions();
        if let Some(existing) = state.by_owner.get(&owner) {
            return if existing.remote_id == remote_id && existing.user_id == user_id {
                Ok(())
            } else {
                Err(E_CALLBACK_ALREADY_REGISTERED)
            };
        }
        let remote = callback.object().ok_or(E_INVALID_ARG)?;
        if remote.is_proxy() && !callback.watch_death(Self::on_remote_died) {
            return Err(E_IPC_FAILED);
        }
        state.by_owner.insert(
            owner,
            Registration {
                user_id,
                remote_id,
                subscriber: Arc::new(Subscriber {
                    remote,
                    active: AtomicBool::new(true),
                }),
                _watch: callback,
            },
        );
        Ok(())
    }

    pub(crate) fn unregister(&self, owner: HydrateProgressOwner) -> Result<(), i32> {
        // Release the registry lock before removing the native death recipient.
        let removed = self.subscriptions().remove(&owner);
        removed.map(drop).ok_or(E_CALLBACK_NOT_REGISTERED)
    }

    fn on_remote_died(remote_id: usize) {
        let mut state = Self::get_instance().subscriptions();
        let owners: Vec<_> = state
            .by_owner
            .iter()
            .filter(|(_, registration)| registration.remote_id == remote_id)
            .map(|(owner, _)| *owner)
            .collect();
        let removed: Vec<_> = owners
            .iter()
            .filter_map(|owner| state.remove(owner))
            .collect();
        drop(state);
        info!("[ProgressCallback] client died, removed={}", @public(removed.len()));
    }

    pub(super) fn on_task_progress(
        &self,
        id: u64,
        user_id: i32,
        owner: HydrateProgressOwner,
        path: &str,
        progress_state: HydrateProgressState,
        sizes: (u64, u64),
    ) {
        let mut state = self.subscriptions();
        if matches!(
            progress_state,
            HydrateProgressState::Completed | HydrateProgressState::Cancelled
        ) {
            state.last_progress.remove(&id);
        } else {
            let now = Instant::now();
            if state.last_progress.get(&id).is_some_and(|(last, time)| {
                *last == progress_state && now.duration_since(*time) < MIN_INTERVAL
            }) {
                return;
            }
            state.last_progress.insert(id, (progress_state, now));
        }
        let Some(registration) = state.by_owner.get(&owner) else {
            return;
        };
        if registration.user_id != user_id {
            return;
        }
        let subscriber = Arc::clone(&registration.subscriber);
        let progress = HydrateProgress {
            file_path: path.to_owned(),
            state: progress_state,
            processed_size: sizes.0,
            total_size: sizes.1,
            accessor_id: owner.accessor_id,
        };
        // Submit under the lock to preserve ordering. Binder dispatch runs on the FFRT queue.
        ffrt::submit_progress_task(move || {
            if subscriber.active.load(Ordering::Acquire) {
                if let Err(err) = framework::send_progress(&subscriber.remote, &progress) {
                    warn!("[OnProgress] send failed, state={}, error={:?}", @public(progress.state as i32), @public(err));
                }
            }
        });
    }

    pub(crate) fn drain(&self) {
        ffrt::drain_progress_tasks();
    }

    pub(crate) fn clear(&self) {
        let registrations = {
            let mut state = self.subscriptions();
            for entry in state.by_owner.values() {
                entry.subscriber.active.store(false, Ordering::Release);
            }
            state.last_progress.clear();
            std::mem::take(&mut state.by_owner)
        };
        drop(registrations);
    }
}
