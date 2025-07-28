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

#include "sync_state_manager.h"

#include <type_traits>

#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {
using namespace std;

Action SyncStateManager::UpdateSyncState(SyncState newState)
{
    std::unique_lock<std::shared_mutex> lck(syncMutex_);
    state_ = newState;
    stopSyncFlag_ = false;
    return nextAction_;
}

bool SyncStateManager::CheckAndSetPending(bool forceFlag, SyncTriggerType triggerType)
{
    std::unique_lock<std::shared_mutex> lck(syncMutex_);
    if (!CheckCleaningFlag() &&
        !CheckDisableCloudFlag() &&
        !CheckMediaLibCleaning() &&
        state_ != SyncState::SYNCING) {
        state_ = SyncState::SYNCING;
        nextAction_ = Action::STOP;
        isForceSync_ = forceFlag;
        return false;
    }

    if (nextAction_ == Action::CHECK) {
        return true;
    }
    if (forceFlag) {
        nextAction_ = Action::FORCE_START;
    } else if (triggerType == SyncTriggerType::TASK_TRIGGER) {
        nextAction_ = Action::CHECK;
    } else {
        nextAction_ = Action::START;
    }
    return true;
}

bool SyncStateManager::CheckMediaLibCleaning()
{
    auto ret = system::GetParameter(CLOUDSYNC_STATUS_KEY, "");
    LOGI("CLOUDSYNC_STATUS_KEY %{public}s", ret.c_str());
    return ret == CLOUDSYNC_STATUS_CLEANING;
}

bool SyncStateManager::CheckCleaningFlag()
{
    auto ret = syncSignal.test(static_cast<uint32_t>(SignalPos::CLEANING));
    LOGI("CleaningFlag %{public}d", ret);
    return ret;
}

void SyncStateManager::SetCleaningFlag()
{
    std::unique_lock<std::shared_mutex> lck(syncMutex_);
    syncSignal.set(static_cast<uint32_t>(SignalPos::CLEANING));
    nextAction_ = Action::STOP;
}

Action SyncStateManager::ClearCleaningFlag()
{
    std::unique_lock<std::shared_mutex> lck(syncMutex_);
    syncSignal.reset(static_cast<uint32_t>(SignalPos::CLEANING));
    return nextAction_;
}

bool SyncStateManager::CheckDisableCloudFlag()
{
    auto ret = syncSignal.test(static_cast<uint32_t>(SignalPos::DISABLE_CLOUD));
    LOGI("DisableCloudFlag %{public}d", ret);
    return ret;
}

void SyncStateManager::SetDisableCloudFlag()
{
    std::unique_lock<std::shared_mutex> lck(syncMutex_);
    syncSignal.set(static_cast<uint32_t>(SignalPos::DISABLE_CLOUD));
    nextAction_ = Action::STOP;
}

Action SyncStateManager::ClearDisableCloudFlag()
{
    std::unique_lock<std::shared_mutex> lck(syncMutex_);
    syncSignal.reset(static_cast<uint32_t>(SignalPos::DISABLE_CLOUD));
    return nextAction_;
}

bool SyncStateManager::GetStopSyncFlag()
{
    return stopSyncFlag_;
}

void SyncStateManager::SetStopSyncFlag()
{
    std::unique_lock<std::shared_mutex> lck(syncMutex_);
    nextAction_ = Action::STOP;
    stopSyncFlag_ = true;
}

SyncState SyncStateManager::GetSyncState() const
{
    return state_;
}

bool SyncStateManager::GetForceFlag() const
{
    return isForceSync_;
}
} // namespace OHOS::FileManagement::CloudSync
