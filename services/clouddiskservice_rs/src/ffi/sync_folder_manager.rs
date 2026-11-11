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

use crate::common::error_codes::{E_INVALID_ARG, E_OK};
use crate::sync_folder_manager::SyncFolderValue;

#[cxx::bridge(namespace = "OHOS::FileManagement::CloudDiskService")]
mod bridge {
    // SAFETY: The manager does not retain the borrowed vector/string. Accessors
    // return references to fields of the live vector element, with the same lifetime.
    unsafe extern "C++" {
        include!("sync_folder_manager.h");

        #[namespace = "OHOS::FileManagement"]
        type SyncFolderExt;

        #[rust_name = "get_all_sync_folders_for_sa"]
        fn GetAllSyncFoldersForSa(folders: Pin<&mut CxxVector<SyncFolderExt>>) -> i32;

        #[rust_name = "get_sync_folder_bundle_name"]
        fn GetSyncFolderBundleName(folder: &SyncFolderExt) -> &CxxString;

        #[rust_name = "get_sync_folder_path"]
        fn GetSyncFolderPath(folder: &SyncFolderExt) -> &CxxString;

        #[rust_name = "unregister_for_sa"]
        fn UnregisterForSa(path: &CxxString) -> i32;
    }

    impl CxxVector<SyncFolderExt> {}
}

pub(crate) fn get_all_sync_folders_for_sa() -> Result<Vec<SyncFolderValue>, i32> {
    let mut folders = cxx::CxxVector::<bridge::SyncFolderExt>::new();
    let ret = bridge::get_all_sync_folders_for_sa(folders.pin_mut());
    if ret != E_OK {
        return Err(ret);
    }
    let mut result = Vec::with_capacity(folders.len());
    for folder in folders.iter() {
        let bundle_name = bridge::get_sync_folder_bundle_name(folder).to_str();
        let path = bridge::get_sync_folder_path(folder).to_str();
        match (bundle_name, path) {
            (Ok(bundle_name), Ok(path)) => result.push(SyncFolderValue {
                bundle_name: bundle_name.to_owned(),
                path: path.to_owned(),
            }),
            _ => {
                error!("[GetAllSyncFoldersForSa] invalid UTF-8 in sync folder, ret={}", @public(E_INVALID_ARG));
            }
        }
    }
    Ok(result)
}

pub(crate) fn unregister_for_sa(path: &str) -> i32 {
    cxx::let_cxx_string!(path = path);
    bridge::unregister_for_sa(&path)
}
