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

#include "sync_folder_manager.h"

#include "cloud_disk_sync_folder_manager.h"

namespace OHOS::FileManagement::CloudDiskService {
int32_t GetAllSyncFoldersForSa(std::vector<SyncFolderExt> &folders)
{
    return OHOS::FileManagement::CloudDiskSyncFolderManager::GetInstance().GetAllSyncFoldersForSa(folders);
}

const std::string &GetSyncFolderBundleName(const SyncFolderExt &folder)
{
    return folder.bundleName_;
}

const std::string &GetSyncFolderPath(const SyncFolderExt &folder)
{
    return folder.path_;
}

int32_t UnregisterForSa(const std::string &path)
{
    return OHOS::FileManagement::CloudDiskSyncFolderManager::GetInstance().UnregisterForSa(path);
}
} // namespace OHOS::FileManagement::CloudDiskService
