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

#include "optimize_cache_task.h"
#include "cloud_file_kit.h"
#include "parameters.h"
#include "system_load.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement {
namespace CloudSync {
static const std::string FILEMANAGER_KEY = "persist.kernel.bundle_name.filemanager";
OptimizeCacheTask::OptimizeCacheTask(std::shared_ptr<CloudFile::DataSyncManager> dataSyncManager)
    : CycleTask("optimize_cache_task",
                {GALLERY_BUNDLE_NAME, system::GetParameter(FILEMANAGER_KEY, ""), HDC_BUNDLE_NAME},
                ONE_DAY,
                dataSyncManager)
{}

int32_t OptimizeCacheTask::RunTaskForBundle(int32_t userId, std::string bundleName)
{
    if (!SystemLoadStatus::IsLoadStatusUnderHot()) {
        LOGE("OptimizeCacheTask::RunTaskForBundle system load is over hot");
        return E_STOP;
    }
    return dataSyncManager_->OptimizeCache(userId, bundleName);
}
} // namespace CloudSync
} // namespace FileManagement
} // namespace OHOS