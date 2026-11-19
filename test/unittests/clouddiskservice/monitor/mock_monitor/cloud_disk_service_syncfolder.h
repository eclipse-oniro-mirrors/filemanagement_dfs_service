/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef CLOUD_DISK_SERVICE_SYNCFOLDER_H
#define CLOUD_DISK_SERVICE_SYNCFOLDER_H

#include <string>

#include "disk_types.h"

namespace OHOS {
namespace FileManagement {
namespace CloudDiskService {
/*
 * for tdd only, do not depend other modules
 */
class CloudDiskServiceSyncFolder {
public:
    static int32_t SetSyncFolderChanges(const struct EventInfo &eventInfos)
    {
        postedCount_++;
        lastPostedOperateType_ = eventInfos.operateType;
        lastPostedPath_ = eventInfos.path;
        lastPostedSyncFolderIndex_ = eventInfos.syncFolderIndex;
        return 0;
    }

    static void ResetPostedEvents()
    {
        postedCount_ = 0;
        lastPostedOperateType_ = OperationType::OPERATION_MAX;
        lastPostedPath_ = "";
        lastPostedSyncFolderIndex_ = 0;
    }

    static int32_t GetPostedCount()
    {
        return postedCount_;
    }

    static OperationType GetLastPostedOperateType()
    {
        return lastPostedOperateType_;
    }

    static std::string GetLastPostedPath()
    {
        return lastPostedPath_;
    }

    static uint32_t GetLastPostedSyncFolderIndex()
    {
        return lastPostedSyncFolderIndex_;
    }

private:
    inline static int32_t postedCount_ = 0;
    inline static OperationType lastPostedOperateType_ = OperationType::OPERATION_MAX;
    inline static std::string lastPostedPath_ = "";
    inline static uint32_t lastPostedSyncFolderIndex_ = 0;
};
} // namespace CloudDiskService
} // namespace FileManagement
} // namespace OHOS
#endif
