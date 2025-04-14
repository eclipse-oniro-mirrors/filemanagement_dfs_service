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
#include "system_load.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement {
namespace CloudSync {
OptimizeStorageTask::OptimizeStorageTask(std::shared_ptr<CloudFile::DataSyncManager> dataSyncManager)
    : CycleTask("optimize_storage_task", {"com.ohos.photos"}, ONE_DAY, dataSyncManager)
{
}

int32_t OptimizeStorageTask::RunTaskForBundle(int32_t userId, std::string bundleName)
{
    if (!SystemLoadStatus::IsLoadStatusUnderHot()) {
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
    std::map<std::string, std::string> param;
    auto ret = instance->GetAppConfigParams(userId, bundleName, param);
    if (ret != E_OK || param.empty()) {
        LOGE("GetAppConfigParams failed");
        return ret;
    }

    int32_t agingDays = std::atoi(param["validDays"].c_str());
    int32_t agingPolicy = std::atoi(param["dataAgingPolicy"].c_str());
    if (agingPolicy == 0) {
        return dataSyncManager_->OptimizeStorage(bundleName, userId, agingDays);
    }
    return E_OK;
}
} // namespace CloudSync
} // namespace FileManagement
} // namespace OHOS