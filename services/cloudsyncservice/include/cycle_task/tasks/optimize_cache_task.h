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

#ifndef OHOS_CLOUD_SYNC_SERVICE_OPTIMIZE_CACHE_TASK_H
#define OHOS_CLOUD_SYNC_SERVICE_OPTIMIZE_CACHE_TASK_H
#include "cloud_pref_impl.h"
#include "cycle_task.h"

namespace OHOS {
namespace FileManagement {
namespace CloudSync {

class OptimizeCacheTask : public CycleTask {
public:
    OptimizeCacheTask(std::shared_ptr<CloudFile::DataSyncManager> dataSyncManager);
    int32_t RunTaskForBundle(int32_t userId, std::string bundleName) override;
};
} // namespace CloudSync
} // namespace FileManagement
} // namespace OHOS
#endif // OHOS_CLOUD_SYNC_SERVICE_OPTIMIZE_CACHE_TASK_H