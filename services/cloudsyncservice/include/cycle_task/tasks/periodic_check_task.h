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

#ifndef OHOS_CLOUD_SYNC_SERVICE_FULL_AUDIT_TASK_H
#define OHOS_CLOUD_SYNC_SERVICE_FULL_AUDIT_TASK_H
#include <cstdint>
#include <string>

#include "cloud_pref_impl.h"
#include "cloud_sync_constants.h"
#include "cycle_task.h"

namespace OHOS {
namespace FileManagement {
namespace CloudSync {

const std::string PeriodicCheckTaskName = "periodic_check_task";

class PeriodicCheckTask : public CycleTask {
public:
    PeriodicCheckTask(std::shared_ptr<CloudFile::DataSyncManager> dataSyncManager);
    int32_t RunTaskForBundle(int32_t userId, std::string bundleName) override;
    /**
     * @brief 主动触发核查入口
     *
     * 不做息屏/充电/温控门槛校验，条件由内部同步流程自行判断，避免主动触发因当前屏幕、
     * 电量或温控状态被直接拒绝。不经过 CycleTask::RunTask，因此不受周期间隔节流限制。
     *
     * @param userId 用户ID
     * @param bundleName 待核查的应用包名
     * @return int32_t 核查执行结果，同步触发失败时返回对应错误码
     */
    int32_t TriggerCheck(int32_t userId, const std::string &bundleName,
        const std::string &triggerBundleName = "");

private:
    int32_t CheckTriggerCondition();
    int32_t DoCheck(int32_t userId, const std::string &bundleName, CheckTriggerReason checkReason,
        const std::string &triggerBundleName = "");
};
} // namespace CloudSync
} // namespace FileManagement
} // namespace OHOS
#endif // OHOS_CLOUD_SYNC_SERVICE_FULL_AUDIT_TASK_H