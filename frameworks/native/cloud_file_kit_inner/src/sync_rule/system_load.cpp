/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "system_load.h"

#include "dfs_error.h"
#include "parameters.h"
#include "task_state_manager.h"
#include "utils_log.h"
#include "res_sched_client.h"

namespace OHOS::FileManagement::CloudSync {

void SystemLoadListener::SetDataSycner(std::shared_ptr<CloudFile::DataSyncManager> dataSyncManager)
{
    dataSyncManager_ = dataSyncManager;
}

void SystemLoadStatus::RegisterSystemloadCallback(std::shared_ptr<CloudFile::DataSyncManager> dataSyncManager)
{
    sptr<SystemLoadListener> loadListener = new (std::nothrow) SystemLoadListener();
    if (loadListener == nullptr) {
        return;
    }
    loadListener->SetDataSycner(dataSyncManager);
    ResourceSchedule::ResSchedClient::GetInstance().RegisterSystemloadNotifier(loadListener);
}

void SystemLoadListener::OnSystemloadLevel(int32_t level)
{
    SystemLoadStatus::Setload(level);
    if (level >= SYSTEMLOADLEVEL_HOT) {
        LOGI("OnSystemloadLevel over warm");
    } else if (dataSyncManager_) {
        std::string systemLoadSync = system::GetParameter(TEMPERATURE_SYSPARAM_SYNC, "");
        std::string systemLoadThumb = system::GetParameter(TEMPERATURE_SYSPARAM_THUMB, "");
        LOGI("OnSystemloadLevel is normal, level:%{public}d", level);
        if (systemLoadSync == "true") {
            LOGI("SetParameter TEMPERATURE_SYSPARAM_SYNC false");
            system::SetParameter(TEMPERATURE_SYSPARAM_SYNC, "false");
            TaskStateManager::GetInstance().StartTask();
            dataSyncManager_->TriggerRecoverySync(SyncTriggerType::SYSTEM_LOAD_TRIGGER);
        }
        if (systemLoadThumb == "true") {
            LOGI("SetParameter TEMPERATURE_SYSPARAM_THUMB false");
            system::SetParameter(TEMPERATURE_SYSPARAM_THUMB, "false");
            TaskStateManager::GetInstance().StartTask();
            dataSyncManager_->DownloadThumb();
        }
    }
}

void SystemLoadStatus::GetSystemloadLevel()
{
    loadstatus_ = ResourceSchedule::ResSchedClient::GetInstance().GetSystemloadLevel();
    LOGI("GetSystemloadLevel finish, loadstatus:%{public}d", loadstatus_);
}

void SystemLoadStatus::Setload(int32_t load)
{
    loadstatus_ = load;
}

void SystemLoadStatus::InitSystemload(std::shared_ptr<CloudFile::DataSyncManager> dataSyncManager)
{
    GetSystemloadLevel();
    RegisterSystemloadCallback(dataSyncManager);
}

<<<<<<< HEAD
bool SystemLoadStatus::IsLoadStatusUnderHot(STOPPED_TYPE process)
{
    if (loadstatus_ > SYSTEMLOADLEVEL_HOT) {
        if (process == STOPPED_IN_THUMB) {
            LOGI("SetParameter TEMPERATURE_SYSPARAM_THUMB true");
            system::SetParameter(TEMPERATURE_SYSPARAM_THUMB, "true");
        } else if (process == STOPPED_IN_SYNC) {
            LOGI("SetParameter TEMPERATURE_SYSPARAM_SYNC true");
            system::SetParameter(TEMPERATURE_SYSPARAM_SYNC, "true");
        }
=======
bool SystemLoadStatus::IsLoadStatusUnderHeat()
{
    if (loadstatus_ > SYSTEMLOADLEVEL_OVERHEATED) {
        return false;
    }
    return true;
}

bool SystemLoadStatus::IsLoadStatusUnderWarm()
{
    if (loadstatus_ > SYSTEMLOADLEVEL_WARM) {
>>>>>>> 051f84ad (system load)
        return false;
    }
    return true;
}
}