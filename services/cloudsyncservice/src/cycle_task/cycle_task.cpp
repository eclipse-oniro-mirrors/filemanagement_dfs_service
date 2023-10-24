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

#include "cycle_task.h"
#include "dfs_error.h"
#include "gallery_data_syncer.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement {
namespace CloudSync {
std::string CycleTask::getTaskName() const
{
    return this->taskName_;
}

std::string CycleTask::getBundleName() const
{
    return this->bundleName_;
}

int32_t CycleTask::getIntervalTime() const
{
    return this->intervalTime_;
}

} // namespace CloudSync
} // namespace FileManagement
} // namespace OHOS