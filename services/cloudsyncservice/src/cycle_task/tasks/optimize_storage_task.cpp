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

#include <memory>
#include "cycle_task.h"
#include "dfs_error.h"
#include "drive_kit.h"
#include "data_sync_manager.h"
#include "gallery_data_syncer.h"
#include "cycle_task_runner.h"
#include "tasks/optimize_storage_task.h"
#include "sync_rule/cloud_status.h"
#include "utils_log.h"


namespace OHOS {
namespace FileManagement {
namespace CloudSync {
OptimizeStorageTask::OptimizeStorageTask(std::shared_ptr<DataSyncManager> dataSyncManager)
    : CycleTask("optimize_storage_task", "com.ohos.photos", CycleTaskRunner::ONE_DAY, dataSyncManager) {}

int32_t OptimizeStorageTask::RunTask(int32_t userId)
{
    auto driveKit = DriveKit::DriveKitNative::GetInstance(userId);
    auto dataSyncManager_ = GetDataSyncManager();
    if (driveKit == nullptr) {
        LOGE("get drive kit instance fail");
        return E_CLOUD_SDK;
    }

    std::map<std::string, std::string> param;
    auto err = driveKit->GetAppConfigParams(bundleName_, param);
    if (err.HasError() || param.empty()) {
        LOGE("GetAppConfigParams failed, server err:%{public}d and dk err:%{public}d", err.serverErrorCode,
             err.dkErrorCode);
        return E_CLOUD_SDK;
    }

    int32_t agingDays = std::stoi(param["validDays"]);
    int32_t agingPolicy = std::stoi(param["dataAgingPolicy"]);
    if (agingPolicy == 0) {
        return dataSyncManager_->OptimizeStorage(bundleName_, userId, agingDays);
    }

    return E_OK;
}


} // namespace CloudSync
} // namespace FileManagement
} // namespace OHOS