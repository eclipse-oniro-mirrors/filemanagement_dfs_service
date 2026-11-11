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

#ifndef CLOUD_DISK_SERVICE_RS_SYNC_FOLDER_MANAGER_H
#define CLOUD_DISK_SERVICE_RS_SYNC_FOLDER_MANAGER_H

#include <cstdint>
#include <string>
#include <vector>

#include "cloud_disk_comm.h"

namespace OHOS::FileManagement::CloudDiskService {
int32_t GetAllSyncFoldersForSa(std::vector<SyncFolderExt> &folders);
const std::string &GetSyncFolderBundleName(const SyncFolderExt &folder);
const std::string &GetSyncFolderPath(const SyncFolderExt &folder);
int32_t UnregisterForSa(const std::string &path);
} // namespace OHOS::FileManagement::CloudDiskService

#endif // CLOUD_DISK_SERVICE_RS_SYNC_FOLDER_MANAGER_H
