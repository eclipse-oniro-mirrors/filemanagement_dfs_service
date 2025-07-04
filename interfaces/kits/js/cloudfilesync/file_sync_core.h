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

#ifndef OHOS_FILEMGMT_FILE_SYNC_CORE_H
#define OHOS_FILEMGMT_FILE_SYNC_CORE_H

#include <optional>

#include "cloud_sync_callback_ani.h"
#include "filemgmt_libfs.h"

namespace OHOS::FileManagement::CloudSync {
using namespace ModuleFileIO;

class FileSyncCore {
public:
    static FsResult<FileSyncCore *> Constructor(const std::optional<std::string> &bundleName = std::nullopt);

    FsResult<void> DoStart();
    FsResult<void> DoStop();
    FsResult<void> DoOn(const std::string &event, const std::shared_ptr<CloudSyncCallbackMiddle> callback);
    FsResult<void> DoOff(const std::string &event,
        const std::optional<std::shared_ptr<CloudSyncCallbackMiddle>> &callback = std::nullopt);
    FsResult<int64_t> DoGetLastSyncTime();

    const std::string &GetBundleName() const;
    explicit FileSyncCore(const std::string &bundleName);
    FileSyncCore();
    ~FileSyncCore() = default;

private:
    std::shared_ptr<CloudSyncCallbackMiddle> callback_;
    std::unique_ptr<BundleEntity> bundleEntity;
};
} // namespace OHOS::FileManagement::CloudSync
#endif // OHOS_FILEMGMT_FILE_SYNC_CORE_H
