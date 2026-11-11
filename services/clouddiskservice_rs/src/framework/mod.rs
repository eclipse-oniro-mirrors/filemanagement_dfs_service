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

//! SAFWK lifecycle and IPC protocol adaptation for CloudDiskService.

mod ability;
mod callback_proxy;
mod interface;
mod stub;

pub(crate) use ability::{register_cloud_disk_service, AbilityHandler, CLOUD_DISK_SERVICE_SA_ID};
pub(crate) use callback_proxy::CloudDiskServiceCallbackProxy;
pub(crate) use interface::ICloudDiskService;
