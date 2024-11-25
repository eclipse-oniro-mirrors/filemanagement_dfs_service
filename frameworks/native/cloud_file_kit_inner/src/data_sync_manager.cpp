/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License"){}
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

#include "data_sync_manager.h"
#include "dfs_error.h"

namespace OHOS::FileManagement::CloudFile {
using namespace std;

int32_t DataSyncManager::TriggerStartSync(const std::string &bundleName,
                                          const int32_t userId,
                                          bool forceFlag,
                                          SyncTriggerType triggerType,
                                          std::string prepareTraceId)
{
    return E_OK;
}

int32_t DataSyncManager::TriggerStopSync(const std::string &bundleName,
                                         const int32_t userId,
                                         bool forceFlag,
                                         SyncTriggerType triggerType)
{
    return E_OK;
}

int32_t DataSyncManager::StopUploadTask(const std::string &bundleName, const int32_t userId)
{
    return E_OK;
}

int32_t DataSyncManager::TriggerRecoverySync(SyncTriggerType triggerType)
{
    return E_OK;
}

int32_t DataSyncManager::ResetCursor(const std::string &bundleName, const int32_t &userId)
{
    return E_OK;
}

void DataSyncManager::RegisterCloudSyncCallback(const std::string &bundleName,
                                                const std::string &callerBundleName,
                                                const int32_t userId,
                                                const sptr<CloudSync::ICloudSyncCallback> &callback)
{
    return;
}

void DataSyncManager::UnRegisterCloudSyncCallback(const std::string &bundleName,
                                                  const std::string &callerBundleName)
{
    return;
}

int32_t DataSyncManager::IsSkipSync(const std::string &bundleName, const int32_t userId, bool forceFlag)
{
    return E_OK;
}

int32_t DataSyncManager::StartDownloadFile(const BundleNameUserInfo &bundleNameUserInfo,
                                           const std::vector<std::string> pathVec,
                                           int64_t &downloadId, std::bitset<FIELD_KEY_MAX_SIZE> fieldkey)
{
    return E_OK;
}

int32_t DataSyncManager::StopDownloadFile(const BundleNameUserInfo &bundleNameUserInfo,
                                          const std::string path,
                                          bool needClean)
{
    return E_OK;
}

int32_t DataSyncManager::StopFileCache(const BundleNameUserInfo &bundleNameUserInfo,
                                       const int64_t &downloadId,
                                       bool needClean)
{
    return E_OK;
}

int32_t DataSyncManager::RegisterDownloadFileCallback(const BundleNameUserInfo &bundleNameUserInfo,
                                                      const sptr<CloudSync::ICloudDownloadCallback> &downloadCallback)
{
    return E_OK;
}

int32_t DataSyncManager::UnregisterDownloadFileCallback(const BundleNameUserInfo &bundleNameUserInfo)
{
    return E_OK;
}

int32_t DataSyncManager::CleanCloudFile(const int32_t userId, const std::string &bundleName, const int action)
{
    return E_OK;
}

int32_t DataSyncManager::CleanRemainFile(const std::string &bundleName, const int32_t userId)
{
    return E_OK;
}

int32_t DataSyncManager::OptimizeStorage(const std::string &bundleName, const int32_t userId, const int32_t agingDays)
{
    return E_OK;
}

int32_t DataSyncManager::DownloadThumb()
{
    return E_OK;
}

int32_t DataSyncManager::CacheVideo()
{
    return E_OK;
}

int32_t DataSyncManager::CleanVideoCache()
{
    return E_OK;
}

int32_t DataSyncManager::CleanCache(const std::string &bundleName, const int32_t userId, const std::string &uri)
{
    return E_OK;
}

int32_t DataSyncManager::DisableCloud(const int32_t userId)
{
    return E_OK;
}

int32_t DataSyncManager::GetUserId(int32_t &userId)
{
    return E_OK;
}

int32_t DataSyncManager::SaveSubscription(const std::string &bundleName, const int32_t userId)
{
    return E_OK;
}

int32_t DataSyncManager::ReportEntry(const std::string &bundleName, const int32_t userId)
{
    return E_OK;
}

} // namespace OHOS::FileManagement::CloudFile