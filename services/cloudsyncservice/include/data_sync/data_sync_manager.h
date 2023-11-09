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

#ifndef OHOS_FILEMGMT_DATA_SYNC_MANAGER_H
#define OHOS_FILEMGMT_DATA_SYNC_MANAGER_H

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "i_cloud_download_callback.h"
#include "data_sync/data_syncer.h"

namespace OHOS::FileManagement::CloudSync {
constexpr int32_t INVALID_USER_ID = -1;
class DataSyncManager {
public:
    DataSyncManager() = default;
    ~DataSyncManager() = default;

    int32_t TriggerStartSync(const std::string &bundleName,
                             const int32_t userId,
                             bool forceFlag,
                             SyncTriggerType triggerType);
    int32_t TriggerStopSync(const std::string &bundleName, const int32_t userId, SyncTriggerType triggerType);
    int32_t TriggerRecoverySync(SyncTriggerType triggerType);
    void RegisterCloudSyncCallback(const std::string &bundleName, const int32_t userId);
    std::shared_ptr<DataSyncer> GetDataSyncer(const std::string &bundleName, const int32_t userId);
    int32_t IsSkipSync(const std::string &bundleName, const int32_t userId) const;
    int32_t StartDownloadFile(const std::string &bundleName, const int32_t userId, const std::string path);
    int32_t StopDownloadFile(const std::string &bundleName, const int32_t userId, const std::string path);
    int32_t RegisterDownloadFileCallback(const std::string &bundleName,
                                         const int32_t userId,
                                         const sptr<ICloudDownloadCallback> &downloadCallback);
    int32_t UnregisterDownloadFileCallback(const std::string &bundleName,
                                           const int32_t userId);
    int32_t CleanCloudFile(const int32_t userId, const std::string &bundleName, const int action);

private:
    std::vector<std::shared_ptr<DataSyncer>> dataSyncers_;
    std::mutex dataSyncMutex_;
    int32_t currentUserId_{INVALID_USER_ID};

    int32_t IsUserVerified(const int32_t userId);
};
} // namespace OHOS::FileManagement::CloudSync

#endif // OHOS_FILEMGMT_DATA_SYNC_MANAGER_H
