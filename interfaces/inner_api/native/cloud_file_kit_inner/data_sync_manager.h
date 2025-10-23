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
#ifndef OHOS_CLOUD_FILE_DATA_SYNC_MANAGER_H
#define OHOS_CLOUD_FILE_DATA_SYNC_MANAGER_H

#include <bitset>
#include <memory>

#include "data_sync_const.h"
#include "i_cloud_download_callback.h"
#include "i_cloud_optimize_callback.h"
#include "i_cloud_sync_callback.h"
#include "i_downgrade_dl_callback.h"

namespace OHOS::FileManagement::CloudFile {
class DataSyncManager {
public:
    using BundleNameUserInfo = CloudSync::BundleNameUserInfo;
    using SyncTriggerType = CloudSync::SyncTriggerType;
    DataSyncManager() = default;
    virtual ~DataSyncManager() = default;

    virtual int32_t TriggerStartSync(const std::string &bundleName,
                                     const int32_t userId,
                                     bool forceFlag,
                                     SyncTriggerType triggerType,
                                     std::string prepareTraceId = "");
    virtual int32_t TriggerStopSync(const std::string &bundleName, const int32_t userId,
                                    bool forceFlag, SyncTriggerType triggerType);
    virtual int32_t StopSyncSynced(const std::string &bundleName, const int32_t userId,
                                        bool forceFlag, SyncTriggerType triggerType);
    virtual int32_t TriggerRecoverySync(SyncTriggerType triggerType);
    virtual int32_t StopUploadTask(const std::string &bundleName, const int32_t userId);
    virtual int32_t ResetCursor(const std::string &bundleName, const int32_t &userId, bool flag = false);
    virtual void RegisterCloudSyncCallback(const std::string &bundleName,
                                           const std::string &callerBundleName,
                                           const int32_t userId,
                                           const sptr<CloudSync::ICloudSyncCallback> &callback);
    virtual void UnRegisterCloudSyncCallback(const std::string &bundleName, const std::string &callerBundleName);
    virtual int32_t IsSkipSync(const std::string &bundleName, const int32_t userId, bool forceFlag);
    virtual int32_t StartFileCache(const BundleNameUserInfo &bundleNameUserInfo,
                                   const std::vector<std::string> &uriVec,
                                   int64_t &downloadId,
                                   int32_t fieldkey,
                                   const sptr<CloudSync::ICloudDownloadCallback> &downloadCallback,
                                   int32_t timeout = -1);
    virtual int32_t StartDownloadFile(const BundleNameUserInfo &bundleNameUserInfo,
                                      const std::string &uri,
                                      int64_t &downloadId,
                                      const sptr<CloudSync::ICloudDownloadCallback> &downloadCallback);
    virtual int32_t StopFileCache(const BundleNameUserInfo &bundleNameUserInfo,
                                  int64_t downloadId,
                                  bool needClean,
                                  int32_t timeout = -1);
    virtual int32_t StopDownloadFile(const BundleNameUserInfo &bundleNameUserInfo, int64_t downloadId, bool needClean);
    virtual int32_t CleanCloudFile(const int32_t userId, const std::string &bundleName, const int action);
    virtual int32_t OptimizeStorage(const std::string &bundleName, const int32_t userId, const int32_t agingDays);
    virtual int32_t StartOptimizeStorage(const BundleNameUserInfo &bundleNameUserInfo,
        const CloudSync::OptimizeSpaceOptions &optimizeOptions,
        const sptr<CloudSync::ICloudOptimizeCallback> &optimizeCallback);
    virtual int32_t StopOptimizeStorage(const BundleNameUserInfo &bundleNameUserInfo);
    virtual int32_t OptimizeCache(const int32_t userId, const std::string &bundleName);
    virtual int32_t BatchCleanFile(const std::vector<CloudSync::CleanFileInfo> &fileInfo,
        std::vector<std::string> &failCloudId);
    virtual int32_t DownloadThumb();
    virtual int32_t TriggerDownloadThumb();
    virtual int32_t CacheVideo();
    virtual int32_t CleanVideoCache();
    virtual int32_t CleanCache(const std::string &bundleName, const int32_t userId, const std::string &uri);
    virtual int32_t BatchDentryFileInsert(const std::vector<CloudSync::DentryFileInfo> &fileInfo,
        std::vector<std::string> &failCloudId);
    virtual int32_t DisableCloud(const int32_t userId);
    virtual int32_t GetUserId(int32_t &userId);
    virtual int32_t SaveSubscription(const std::string &bundleName, const int32_t userId);
    virtual int32_t ReportEntry(const std::string &bundleName, const int32_t userId);
    virtual int32_t ChangeAppSwitch(const std::string &bundleName, const int32_t userId, bool status);
    virtual int32_t StartDowngrade(const std::string &bundleName,
                                   const sptr<CloudSync::IDowngradeDlCallback> &downloadCallback);
    virtual int32_t StopDowngrade(const std::string &bundleName);
    virtual int32_t GetCloudFileInfo(const std::string &bundleName, CloudSync::CloudFileInfo &cloudFileInfo);
    // file version
    virtual int32_t GetHistoryVersionList(const BundleNameUserInfo &bundleNameUserInfo, const std::string &uri,
                                          const int32_t versionNumLimit,
                                          std::vector<CloudSync::HistoryVersion> &historyVersionList);
    virtual int32_t DownloadHistoryVersion(const BundleNameUserInfo &bundleNameUserInfo, const std::string &uri,
                                           int64_t &downloadId, const uint64_t versionId,
                                           const sptr<CloudSync::ICloudDownloadCallback> &downloadCallback,
                                           std::string &versionUri);
    virtual int32_t ReplaceFileWithHistoryVersion(const BundleNameUserInfo &bundleNameUserInfo, const std::string &uri,
                                                  const std::string &versionUri);
    virtual int32_t IsFileConflict(const BundleNameUserInfo &bundleNameUserInfo, const std::string &uri,
                                   bool &isConflict);
    virtual int32_t ClearFileConflict(const BundleNameUserInfo &bundleNameUserInfo, const std::string &uri);
};
} // namespace OHOS::FileManagement::CloudFile

#endif // OHOS_CLOUD_FILE_DATA_SYNC_MANAGER_H