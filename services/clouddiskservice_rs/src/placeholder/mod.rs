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

//! Placeholder file operations and lifecycle managers.

mod callback_manager;
pub(crate) mod file;
mod hydration;
mod hydration_temp;
pub(crate) use hydration_temp::clean_orphans as clean_orphan_hydration_files;
mod progress_manager;
pub(crate) mod system_accessor;
mod task_manager;

pub(crate) use callback_manager::PlaceholderCallbackManager;
pub(crate) use progress_manager::{HydrateProgressOwner, PlaceholderProgressManager};
pub(crate) use task_manager::{PlaceholderTaskCancelReason, PlaceholderTaskManager};
