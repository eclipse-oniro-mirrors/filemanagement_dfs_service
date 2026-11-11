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

//! SAFWK lifecycle, corresponding to the C++ service's OnStart and OnStop.

use system_ability_fwk::ability::{Ability, Handler};
use system_ability_fwk::cxx_share::SystemAbilityOnDemandReason;

use crate::service::CloudDiskService;

pub(crate) const CLOUD_DISK_SERVICE_SA_ID: i32 = 5207;
// system_ability_definition.h: COMMON_EVENT_SERVICE_ID.
const COMMON_EVENT_SERVICE_ID: i32 = 3299;

pub(crate) struct AbilityHandler(Handler);

impl AbilityHandler {
    pub(crate) fn new(handler: Handler) -> Self {
        Self(handler)
    }

    pub(crate) fn listen_common_events(&self) -> bool {
        self.0.add_system_ability_listen(COMMON_EVENT_SERVICE_ID)
    }
}

// SAFETY: SAFWK owns the registered SA beyond OnStop. The service keeps this
// handler behind a mutex and joins initialization before releasing it. Only
// listener operations cross threads; their C++ implementation uses listenerLock_.
unsafe impl Send for AbilityHandler {}

impl Ability for CloudDiskService {
    fn on_start(&self, handler: Handler) {
        self.start(handler, String::new(), String::new());
    }

    fn on_start_with_reason(&self, reason: SystemAbilityOnDemandReason, handler: Handler) {
        self.start(handler, reason.name, reason.value);
    }

    fn on_stop(&self) {
        self.stop();
    }

    fn on_system_ability_load_event(&self, said: i32, device_id: String) {
        info!("[OnAddSystemAbility] sa_id={}, device_id={}", @public(said), @private(device_id));
        if said == COMMON_EVENT_SERVICE_ID {
            self.on_common_event_service_ready();
        }
    }
}

pub(crate) fn register_cloud_disk_service() {
    match CloudDiskService::new().build_system_ability(CLOUD_DISK_SERVICE_SA_ID, false) {
        Some(ability) => {
            ability.register();
            info!(
                "[RegisterSA] registered, saId={}",
                @public(CLOUD_DISK_SERVICE_SA_ID),
            );
        }
        None => error!(
            "[RegisterSA] failed, saId={}",
            @public(CLOUD_DISK_SERVICE_SA_ID),
        ),
    }
}
