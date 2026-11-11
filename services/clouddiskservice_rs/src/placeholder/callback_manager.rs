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

//! Callback-manager skeleton corresponding to PlaceholderCallbackManager.

pub(crate) struct PlaceholderCallbackManager {
    _private: (),
}

impl PlaceholderCallbackManager {
    pub(crate) fn get_instance() -> &'static Self {
        static INSTANCE: PlaceholderCallbackManager = PlaceholderCallbackManager { _private: () };
        &INSTANCE
    }

    pub(crate) fn clear_by_sync_folder(&self, bundle_name: &str, sync_folder_index: u32) {
        info!(
            "[ClearBySyncFolder] pending implementation, bundle_name={}, sync_folder_index={}",
            @private(bundle_name),
            @public(sync_folder_index),
        );
        // TODO: Clear the callbacks for this bundle and sync folder when
        // Placeholder callback registration is implemented.
    }

    pub(crate) fn clear_all(&self) {
        info!("[PlaceholderCallbackManager::ClearAll] pending implementation");
        // TODO: Clear all Placeholder callback registrations and death recipients.
    }
}
