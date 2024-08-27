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
    if (level > SYSTEMLOADLEVEL_HOT) {
        LOGI("OnSystemloadLevel over warm");
    } else if (level <= SYSTEMLOADLEVEL_WARM && dataSyncManager_) {
        std::string systemLoadSync = system::GetParameter(TEMPERATURE_SYSPARAM_SYNC, "");
        std::string systemLoadThumb = system::GetParameter(TEMPERATURE_SYSPARAM_THUMB, "");
        LOGI("OnSystemloadLevel is noraml, level:%{public}d", level);
        if (systemLoadSync == "true") {
            system::SetParameter(TEMPERATURE_SYSPARAM_SYNC, "false");
            dataSyncManager_->TriggerRecoverySync(SyncTriggerType::SYSTEM_LOAD_TRIGGER);
        }
        if (systemLoadThumb == "true") {
            system::SetParameter(TEMPERATURE_SYSPARAM_THUMB, "false");
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

bool SystemLoadStatus::IsLoadStatusUnderHot(bool setFlag)
{
    if (loadstatus_ > SYSTEMLOADLEVEL_HOT) {
        if (setFlag) {
            LOGI("SetSystemloadLevel TEMPERATURE_SYSPARAM_THUMB true");
            system::SetParameter(TEMPERATURE_SYSPARAM_THUMB, "true");
        }
        return false;
    }
    return true;
}
}