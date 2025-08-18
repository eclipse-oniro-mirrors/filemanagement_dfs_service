/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "optimize_storage_task.h"
#include "cloud_file_kit.h"
#include "settings_data_manager.h"
#include "system_load.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement {
namespace CloudSync {
OptimizeStorageTask::OptimizeStorageTask(std::shared_ptr<CloudFile::DataSyncManager> dataSyncManager)
    : CycleTask("optimize_storage_task", {GALLERY_BUNDLE_NAME, HDC_BUNDLE_NAME}, ONE_DAY, dataSyncManager)
{
}

int32_t OptimizeStorageTask::RunTaskForBundle(int32_t userId, std::string bundleName)
{
    if (!SystemLoadStatus::IsSystemLoadAllowed()) {
        LOGE("OptimizeStorageTask::RunTaskForBundle system load is over hot");
        return E_STOP;
    }
    auto instance = CloudFile::CloudFileKit::GetInstance();
    if (instance == nullptr) {
        LOGE("get cloud file helper instance failed");
        return E_NULLPTR;
    }
    auto dataSyncManager_ = GetDataSyncManager();
    if (dataSyncManager_->CleanRemainFile(bundleName, userId) != E_OK) {
        LOGW(" clean reamin file fail");
    }

    int32_t agingDays = -1;
    int32_t agingPolicy = -1;
    if (bundleName == HDC_BUNDLE_NAME) {
        agingDays = SettingsDataManager::GetLocalSpaceFreeDays();
        agingPolicy = SettingsDataManager::GetLocalSpaceFreeStatus();
    } else {
        std::map<std::string, std::string> param;
        auto ret = instance->GetAppConfigParams(userId, bundleName, param);
        if (ret != E_OK || param.empty()) {
            LOGE("GetAppConfigParams failed");
            return ret;
        }
        agingDays = std::stoi(param["validDays"]);
        agingPolicy = std::stoi(param["dataAgingPolicy"]);
    }

    if (agingPolicy == 0) {
        return dataSyncManager_->OptimizeStorage(bundleName, userId, agingDays);
    }
    return E_OK;
}
} // namespace CloudSync
} // namespace FileManagement
} // namespace OHOS