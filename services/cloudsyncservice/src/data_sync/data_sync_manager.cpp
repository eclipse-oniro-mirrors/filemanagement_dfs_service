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

#include "data_sync/data_sync_manager.h"

#include <thread>
#include <vector>

#include "dfs_error.h"
#include "gallery_data_syncer.h"
#include "ipc/cloud_sync_callback_manager.h"
#include "sdk_helper.h"
#include "sync_rule/battery_status.h"
#include "sync_rule/cloud_status.h"
#include "sync_rule/network_status.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {

int32_t DataSyncManager::TriggerStartSync(const std::string &bundleName,
                                          const int32_t userId,
                                          bool forceFlag,
                                          SyncTriggerType triggerType)
{
    auto dataSyncer = GetDataSyncer(bundleName, userId);
    if (!dataSyncer) {
        LOGE("Get dataSyncer failed, bundleName: %{private}s", bundleName.c_str());
        return E_INVAL_ARG;
    }

    auto ret = IsSkipSync(bundleName, userId);
    if (ret != E_OK) {
        return ret;
    }
    /* get sdk helper */
    auto sdkHelper = std::make_shared<SdkHelper>();
    ret = sdkHelper->Init(userId, bundleName);
    if (ret != E_OK) {
        LOGE("get sdk helper err %{public}d", ret);
        return ret;
    }

    dataSyncer->SetSdkHelper(sdkHelper);
    std::thread([dataSyncer, forceFlag, triggerType]() { dataSyncer->StartSync(forceFlag, triggerType); }).detach();

    return E_OK;
}

int32_t DataSyncManager::TriggerStopSync(const std::string &bundleName,
                                         const int32_t userId,
                                         SyncTriggerType triggerType)
{
    auto dataSyncer = GetDataSyncer(bundleName, userId);
    if (!dataSyncer) {
        LOGE("Get dataSyncer failed, bundleName: %{private}s", bundleName.c_str());
        return E_INVAL_ARG;
    }

    /* get sdk helper */
    auto sdkHelper = std::make_shared<SdkHelper>();
    auto ret = sdkHelper->Init(userId, bundleName);
    if (ret != E_OK) {
        LOGE("get sdk helper err %{public}d", ret);
        return ret;
    }
    dataSyncer->SetSdkHelper(sdkHelper);
    std::thread([dataSyncer, triggerType]() { dataSyncer->StopSync(triggerType); }).detach();
    return E_OK;
}

int32_t DataSyncManager::StartDownloadFile(const std::string &bundleName, const int32_t userId, const std::string path)
{
    auto dataSyncer = GetDataSyncer(bundleName, userId);
    if (!dataSyncer) {
        LOGE("Get dataSyncer failed, bundleName: %{private}s", bundleName.c_str());
        return E_INVAL_ARG;
    }

    /* get sdk helper */
    auto sdkHelper = std::make_shared<SdkHelper>();
    auto ret = sdkHelper->Init(userId, bundleName);
    if (ret != E_OK) {
        LOGE("get sdk helper err %{public}d", ret);
        return ret;
    }

    dataSyncer->SetSdkHelper(sdkHelper);
    std::thread([dataSyncer, path, userId]() { dataSyncer->StartDownloadFile(path, userId); }).detach();
    return E_OK;
}

int32_t DataSyncManager::StopDownloadFile(const std::string &bundleName, const int32_t userId, const std::string path)
{
    auto dataSyncer = GetDataSyncer(bundleName, userId);
    if (!dataSyncer) {
        LOGE("Get dataSyncer failed, bundleName: %{private}s", bundleName.c_str());
        return E_INVAL_ARG;
    }
    std::thread([dataSyncer, userId, path]() { dataSyncer->StopDownloadFile(path, userId); }).detach();
    return E_OK;
}

int32_t DataSyncManager::RegisterDownloadFileCallback(const std::string &bundleName,
                                                      const int32_t userId,
                                                      const sptr<ICloudDownloadCallback>& downloadCallback)
{
    auto dataSyncer = GetDataSyncer(bundleName, userId);
    if (!dataSyncer) {
        LOGE("Get dataSyncer failed, bundleName: %{private}s", bundleName.c_str());
        return E_INVAL_ARG;
    }
    std::thread([dataSyncer, userId, downloadCallback]() {
        dataSyncer->RegisterDownloadFileCallback(userId, downloadCallback);
    }).detach();
    return E_OK;
}

int32_t DataSyncManager::UnregisterDownloadFileCallback(const std::string &bundleName,
                                                        const int32_t userId)
{
    auto dataSyncer = GetDataSyncer(bundleName, userId);
    if (!dataSyncer) {
        LOGE("Get dataSyncer failed, bundleName: %{private}s", bundleName.c_str());
        return E_INVAL_ARG;
    }
    std::thread([dataSyncer, userId]() { dataSyncer->UnregisterDownloadFileCallback(userId); }).detach();
    return E_OK;
}

int32_t DataSyncManager::TriggerRecoverySync(SyncTriggerType triggerType)
{
    std::vector<std::string> needSyncApps;
    {
        std::lock_guard<std::mutex> lck(dataSyncMutex_);
        if (currentUserId_ == INVALID_USER_ID) {
            LOGE("useId is invalid");
            return E_INVAL_ARG;
        }

        for (auto dataSyncer : dataSyncers_) {
            if (dataSyncer->GetUserId() == currentUserId_) {
                if ((triggerType == SyncTriggerType::NETWORK_AVAIL_TRIGGER) ||
                    (dataSyncer->GetSyncState() == SyncState::SYNC_FAILED)) {
                    auto bundleName = dataSyncer->GetBundleName();
                    needSyncApps.push_back(bundleName);
                }
            }
        }
    }

    if (needSyncApps.size() == 0) {
        LOGI("not need to trigger sync");
        return E_OK;
    }

    int32_t ret = E_OK;
    for (const auto &app : needSyncApps) {
        ret = TriggerStartSync(app, currentUserId_, false, triggerType);
        if (ret) {
            LOGE("trigger sync failed, ret = %{public}d, bundleName = %{public}s", ret, app.c_str());
        }
    }
    return ret;
}

std::shared_ptr<DataSyncer> DataSyncManager::GetDataSyncer(const std::string &bundleName, const int32_t userId)
{
    std::lock_guard<std::mutex> lck(dataSyncMutex_);
    currentUserId_ = userId;
    for (auto dataSyncer : dataSyncers_) {
        if ((dataSyncer->GetBundleName() == bundleName) && (dataSyncer->GetUserId() == userId)) {
            return dataSyncer;
        }
    }

    std::shared_ptr<DataSyncer> dataSyncer_ = std::make_shared<GalleryDataSyncer>(bundleName, userId);
    int32_t ret = dataSyncer_->Init(bundleName, userId);
    if (ret != E_OK) {
        return nullptr;
    }
    dataSyncers_.push_back(dataSyncer_);
    return dataSyncer_;
}

int32_t DataSyncManager::IsSkipSync(const std::string &bundleName, const int32_t userId) const
{
    if (NetworkStatus::GetNetConnStatus() == NetworkStatus::NetConnStatus::NO_NETWORK) {
        LOGE("network is not available");
        return E_SYNC_FAILED_NETWORK_NOT_AVAILABLE;
    }
    if (!CloudStatus::IsCloudStatusOkay(bundleName, userId)) {
        LOGE("cloud status is not OK");
        return E_CLOUD_SDK;
    }
    if (!BatteryStatus::IsBatteryCapcityOkay()) {
        return E_SYNC_FAILED_BATTERY_TOO_LOW;
    }
    return E_OK;
}

int32_t DataSyncManager::CleanCloudFile(const int32_t userId, const std::string &bundleName, const int action)
{
    LOGD("Enter function CleanCloudFile");
    auto dataSyncer = GetDataSyncer(bundleName, userId);
    if (!dataSyncer) {
        LOGE(" Clean Get dataSyncer failed, bundleName: %{private}s", bundleName.c_str());
        return E_INVAL_ARG;
    }
    LOGD("dataSyncer.bundleName_ is %s", dataSyncer->GetBundleName().c_str());
    LOGD("dataSyncer.userId_ is %d", dataSyncer->GetUserId());
    return dataSyncer->Clean(action);
}
} // namespace OHOS::FileManagement::CloudSync
