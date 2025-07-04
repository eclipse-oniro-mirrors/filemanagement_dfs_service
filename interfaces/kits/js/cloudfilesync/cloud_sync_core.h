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

#ifndef OHOS_FILEMGMT_CLOUD_SYNC_CORE_H
#define OHOS_FILEMGMT_CLOUD_SYNC_CORE_H

#include <optional>

#include "cloud_sync_callback_ani.h"
#include "cloud_sync_common.h"
#include "filemgmt_libfs.h"

namespace OHOS::FileManagement::CloudSync {
using namespace ModuleFileIO;

class CloudSyncCore {
public:
    static FsResult<CloudSyncCore *> Constructor();

    FsResult<void> DoStart();
    FsResult<void> DoStop();
    FsResult<void> DoOn(const std::string &event, const std::shared_ptr<CloudSyncCallbackMiddle> callback);
    FsResult<void> DoOff(const std::string &event,
        const std::optional<std::shared_ptr<CloudSyncCallbackMiddle>> &callback = std::nullopt);
    static FsResult<int32_t> DoGetFileSyncState(std::string path);
    static FsResult<void> DoOptimizeStorage();
    static FsResult<void> DoStartOptimizeStorage(const OptimizeSpaceOptions &optimizeOptions,
        const std::shared_ptr<CloudOptimizeCallbackMiddle> callback);
    static FsResult<void> DoStopOptimizeStorage();

    const std::string &GetBundleName() const;
    CloudSyncCore();
    ~CloudSyncCore() = default;

private:
    std::shared_ptr<CloudSyncCallbackMiddle> callback_;
    std::unique_ptr<BundleEntity> bundleEntity;
};
} // namespace OHOS::FileManagement::CloudSync
#endif // OHOS_FILEMGMT_CLOUD_SYNC_CORE_H
