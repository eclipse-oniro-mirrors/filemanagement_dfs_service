/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "decompress_gains_report_task.h"

#include <ctime>

#include "decompress_gains_reporter.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement {
namespace CloudSync {

DecompressGainsReportTask::DecompressGainsReportTask(std::shared_ptr<CloudFile::DataSyncManager> dataSyncManager)
    : CycleTask("decompress_gains_report_task", {"decompress"}, ONE_DAY, dataSyncManager)
{
    runTaskForSwitchOff_ = true;
}

DecompressGainsReportTask::~DecompressGainsReportTask() {}

int32_t DecompressGainsReportTask::RunTaskForBundle(int32_t userId, std::string bundleName)
{
    LOGI("timed report task triggered, userId: %{public}d, bundleName: %{public}s", userId, bundleName.c_str());
    bool ret = Decompress::DecompressGainsReporter::GetInstance().Report();
    if (ret) {
        LOGI("timed report task succeeded, bundleName: %{public}s", bundleName.c_str());
    } else {
        LOGI("timed report task skipped or failed, bundleName: %{public}s", bundleName.c_str());
    }
    return E_OK;
}

} // namespace CloudSync
} // namespace FileManagement
} // namespace OHOS
