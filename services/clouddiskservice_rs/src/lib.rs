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

//! Rust server framework for CloudDiskService (SA 5207).
//!
//! Existing C++ clients keep their IDL-generated proxy. This crate implements
//! the same wire protocol and dispatches to the business entry points in
//! `service`. Sync-folder registration and unregistration are implemented; other
//! points still provide synthetic IPC verification replies.

#[macro_use]
mod logging;
mod account_status_listener;
mod common;
mod ffi;
mod framework;
mod monitor;
mod placeholder;
mod service;
mod sync_folder_manager;
mod sync_folder_service;

// SAFWK dlopens the shared library and expects it to register its SA.
#[used]
#[link_section = ".init_array"]
static REGISTER_CLOUD_DISK_SERVICE: extern "C" fn() = {
    extern "C" fn register() {
        framework::register_cloud_disk_service();
    }
    register
};
