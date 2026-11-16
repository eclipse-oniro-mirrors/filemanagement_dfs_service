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
#include "periodic_check_task.h"

#include <ctime>

#include "battery_status.h"
#include "cloud_sync_common.h"
#include "parameters.h"
#include "screen_status.h"
#include "system_load.h"
#include "utils_log.h"
#include "operation_log_handler.h"

namespace OHOS {
namespace FileManagement {
namespace CloudSync {
static const std::string FILEMANAGER_KEY = "persist.kernel.bundle_name.filemanager";
PeriodicCheckTask::PeriodicCheckTask(std::shared_ptr<CloudFile::DataSyncManager> dataSyncManager)
    : CycleTask(PeriodicCheckTaskName,
                {GALLERY_BUNDLE_NAME, HDC_BUNDLE_NAME, system::GetParameter(FILEMANAGER_KEY, "")},
                THREE_DAY,
                dataSyncManager)
{
}

int32_t PeriodicCheckTask::CheckTriggerCondition()
{
    if (ScreenStatus::IsScreenOn() || !BatteryStatus::IsCharging() || !SystemLoadStatus::IsSystemLoadAllowed()) {
        LOGI("PeriodicCheckTask::CheckTriggerCondition isScreenOn or not charging");
        return E_STOP;
    }
    return E_OK;
}

int32_t PeriodicCheckTask::DoCheck(int32_t userId, const std::string &bundleName,
    CheckTriggerReason checkReason, const std::string &triggerBundleName)
{
    SyncTriggerType triggerType = SyncTriggerType::TASK_TRIGGER;
    if (checkReason == CheckTriggerReason::MANUAL) {
        triggerType = SyncTriggerType::MANUAL_TRIGGER;
    }
    int32_t ret = dataSyncManager_->TriggerStartSync(
        bundleName, userId, SyncMode::NORMAL, triggerType, "", triggerBundleName);
    if (ret != E_OK) {
        LOGE("trigger %{public}s periodic sync failed, ret: %{public}d", bundleName.c_str(), ret);
        return ret;
    }

    ret = dataSyncManager_->UpdateCachedFileSize(bundleName, userId);
    if (ret != E_OK) {
        LOGE("UpdateCachedFileSize failed for %{public}s, ret: %{public}d", bundleName.c_str(), ret);
    }

    if (bundleName != GALLERY_BUNDLE_NAME && bundleName != HDC_BUNDLE_NAME) {
        ret = CloudDisk::OperationLogHandler::GetInstance().CleanOldRecords();
        if (ret != E_OK) {
            LOGE("OperationLogHandler CleanOldRecords failed for %{public}s, ret: %{public}d", bundleName.c_str(), ret);
        }
    }
    return E_OK;
}

int32_t PeriodicCheckTask::RunTaskForBundle(int32_t userId, std::string bundleName)
{
    int32_t ret = CheckTriggerCondition();
    if (ret != E_OK) {
        return ret;
    }
    return DoCheck(userId, bundleName, CheckTriggerReason::PERIODIC);
}

int32_t PeriodicCheckTask::TriggerCheck(int32_t userId, const std::string &bundleName,
    const std::string &triggerBundleName)
{
    LOGI("Begin PeriodicCheckTask::TriggerCheck");
    int32_t ret = DoCheck(userId, bundleName, CheckTriggerReason::MANUAL, triggerBundleName);
    if (ret == E_OK) {
        InitCloudPrefForBundle(userId, bundleName);
        SetLastRunTime(std::time(nullptr));
    }
    LOGI("End PeriodicCheckTask::TriggerCheck, ret: %{public}d", ret);
    return ret;
}
} // namespace CloudSync
} // namespace FileManagement
} // namespace OHOS