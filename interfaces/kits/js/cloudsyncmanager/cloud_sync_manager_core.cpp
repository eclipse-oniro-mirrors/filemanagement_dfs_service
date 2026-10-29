/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cloud_sync_manager_core.h"

#include <mutex>
#include <sys/types.h>

#include "check_trigger_callback_info.h"
#include "cloud_file_utils.h"
#include "cloud_sync_manager.h"
#include "dfs_error.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {
using namespace std;
namespace {
    static constexpr int32_t BUNDLES_MAX_SIZE = 20;
    std::mutex g_checkTriggerCallbackMutex;
    CheckTriggerCallbackInfo g_checkTriggerCallbackInfo;
    }

FsResult<void> CloudSyncManagerCore::DoChangeAppCloudSwitch(
    const string &accountId, const string &bundleName, bool status)
{
    LOGI("change app cloud switch.");
    int32_t result = CloudSyncManager::GetInstance().ChangeAppSwitch(accountId, bundleName, status);
    if (result == E_PERMISSION_DENIED || result == E_PERMISSION_SYSTEM) {
        LOGE("ChangeAppSwitch failed. ret = %{public}d", result);
        return FsResult<void>::Error(Convert2ErrNum(result));
    }

    return FsResult<void>::Success();
}

FsResult<void> CloudSyncManagerCore::DoNotifyEventChange(
    int32_t userId, const string &eventId, const string &extraData)
{
    LOGI("NotifyEventChange entrance");
    int32_t result = CloudSyncManager::GetInstance().NotifyEventChange(userId, eventId, extraData);
    if (result == E_PERMISSION_DENIED || result == E_PERMISSION_SYSTEM) {
        LOGE("NotifyEventChange failed. ret = %{public}d", result);
        return FsResult<void>::Error(Convert2ErrNum(result));
    }

    return FsResult<void>::Success();
}

FsResult<void> CloudSyncManagerCore::DoNotifyDataChange(const string &accountId, const string &bundleName)
{
    LOGI("NotifyDataChange entrance");
    int32_t result = CloudSyncManager::GetInstance().NotifyDataChange(accountId, bundleName);
    if (result == E_PERMISSION_DENIED || result == E_PERMISSION_SYSTEM) {
        LOGE("NotifyDataChange failed. ret = %{public}d", result);
        return FsResult<void>::Error(Convert2ErrNum(result));
    }

    return FsResult<void>::Success();
}

FsResult<void> CloudSyncManagerCore::DoDisableCloud(const string &accountId)
{
    LOGI("DisableCloud");
    int32_t result = CloudSyncManager::GetInstance().DisableCloud(accountId);
    if (result == E_PERMISSION_DENIED || result == E_PERMISSION_SYSTEM) {
        LOGE("DisableCloud failed. ret = %{public}d", result);
        return FsResult<void>::Error(Convert2ErrNum(result));
    }

    return FsResult<void>::Success();
}

FsResult<void> CloudSyncManagerCore::DoEnableCloud(const string &accountId, const SwitchDataObj &switchData)
{
    LOGI("EnableCloud");
    int32_t result = CloudSyncManager::GetInstance().EnableCloud(accountId, switchData);
    if (result == E_PERMISSION_DENIED || result == E_PERMISSION_SYSTEM) {
        LOGE("EnableCloud failed. ret = %{public}d", result);
        return FsResult<void>::Error(Convert2ErrNum(result));
    }

    return FsResult<void>::Success();
}

FsResult<void> CloudSyncManagerCore::DoClean(const std::string &accountId, const CleanOptions &cleanOptions)
{
    LOGI("Clean");
    int32_t result = CloudSyncManager::GetInstance().Clean(accountId, cleanOptions);
    if (result == E_PERMISSION_DENIED || result == E_PERMISSION_SYSTEM) {
        LOGE("Clean failed. ret = %{public}d", result);
        return FsResult<void>::Error(Convert2ErrNum(result));
    }

    return FsResult<void>::Success();
}

FsResult<std::vector<LocalFilePresentStatus>> CloudSyncManagerCore::DoGetLocalFilePresentStatus(
    const std::vector<std::string> &bundleNames)
{
    LOGI("GetLocalFilePresentStatus entrance");
    if (bundleNames.empty() || bundleNames.size() > BUNDLES_MAX_SIZE) {
        LOGE("bundleNames is empty or exceed max size.");
        return FsResult<std::vector<LocalFilePresentStatus>>::Error(JsErrCode::E_INVALID_ARGUMENT);
    }

    std::vector<LocalFilePresentStatus> localFilePresentStatusList;
    int32_t result =
        CloudSyncManager::GetInstance().GetBundlesLocalFilePresentStatus(bundleNames, localFilePresentStatusList);
    if (result != E_OK) {
        LOGE("GetLocalFilePresentStatus failed. ret = %{public}d", result);
        return FsResult<std::vector<LocalFilePresentStatus>>::Error(Convert2ErrNum(result));
    }

    return FsResult<std::vector<LocalFilePresentStatus>>::Success(localFilePresentStatusList);
}

FsResult<std::vector<DowngradeProgress>> CloudSyncManagerCore::DoGetDowngradeTaskState(
    const std::vector<std::string> &bundleNames)
{
    LOGI("GetDowngradeDownloadTaskState entrance");
    if (bundleNames.empty() || bundleNames.size() > BUNDLES_MAX_SIZE) {
        LOGE("bundleNames is empty or exceed max size.");
        return FsResult<std::vector<DowngradeProgress>>::Error(JsErrCode::E_INVALID_ARGUMENT);
    }

    std::vector<DowngradeProgress> downgradeProgressList;
    int32_t result =
        CloudSyncManager::GetInstance().GetDowngradeDownloadTaskState(bundleNames, downgradeProgressList);
    if (result != E_OK) {
        LOGE("GetDowngradeDownloadTaskState failed. ret = %{public}d", result);
        return FsResult<std::vector<DowngradeProgress>>::Error(Convert2ErrNum(result));
    }

    return FsResult<std::vector<DowngradeProgress>>::Success(downgradeProgressList);
}

FsResult<void> CloudSyncManagerCore::DoTriggerCloudCheck(int32_t userId, const std::string &bundleName)
{
    LOGI("TriggerCloudCheck entrance");
    int32_t result = CloudSyncManager::GetInstance().TriggerCloudCheck(userId, bundleName);
    if (result != E_OK) {
        LOGE("TriggerCloudCheck failed. ret = %{public}d", result);
        return FsResult<void>::Error(Convert2ErrNum(result));
    }

    return FsResult<void>::Success();
}

FsResult<void> CloudSyncManagerCore::DoRegisterCheckTriggerCallback(
    const std::shared_ptr<CloudCheckTriggerCallback> &callback)
{
    LOGI("RegisterCheckTriggerCallback entrance");
    if (callback == nullptr) {
        LOGE("callback is invalid");
        return FsResult<void>::Error(JsErrCode::E_INVALID_ARGUMENT);
    }

    std::lock_guard<std::mutex> lock(g_checkTriggerCallbackMutex);
    if (g_checkTriggerCallbackInfo.callback != nullptr) {
        LOGI("check trigger callback already exist");
        return FsResult<void>::Success();
    }

    CheckTriggerCallbackInfo callbackInfo;
    callbackInfo.callbackId = CloudDisk::CloudFileUtils::GenerateUuid();
    callbackInfo.callback = callback;
    int32_t result = CloudSyncManager::GetInstance().RegisterCheckTriggerCallback(callbackInfo);
    if (result != E_OK) {
        LOGE("RegisterCheckTriggerCallback failed. ret = %{public}d", result);
        g_checkTriggerCallbackInfo = CheckTriggerCallbackInfo();
        return FsResult<void>::Error(Convert2ErrNum(result));
    }
    g_checkTriggerCallbackInfo = callbackInfo;
    return FsResult<void>::Success();
}

FsResult<void> CloudSyncManagerCore::DoUnRegisterCheckTriggerCallback()
{
    LOGI("UnRegisterCheckTriggerCallback entrance");
    std::lock_guard<std::mutex> lock(g_checkTriggerCallbackMutex);
    if (g_checkTriggerCallbackInfo.callbackId.empty()) {
        LOGI("check trigger callback not exist");
        return FsResult<void>::Success();
    }

    int32_t result = CloudSyncManager::GetInstance().UnRegisterCheckTriggerCallback(g_checkTriggerCallbackInfo);
    if (result != E_OK) {
        LOGE("UnRegisterCheckTriggerCallback failed. ret = %{public}d", result);
        return FsResult<void>::Error(Convert2ErrNum(result));
    }
    if (g_checkTriggerCallbackInfo.callback != nullptr) {
        g_checkTriggerCallbackInfo.callback->DeleteReference();
    }
    g_checkTriggerCallbackInfo = CheckTriggerCallbackInfo();
    return FsResult<void>::Success();
}
} // namespace OHOS::FileManagement::CloudSync