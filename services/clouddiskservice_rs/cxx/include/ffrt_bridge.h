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

#ifndef CLOUD_DISK_SERVICE_RS_FFRT_BRIDGE_H
#define CLOUD_DISK_SERVICE_RS_FFRT_BRIDGE_H

#include <cstdint>
#include <memory>

#include "c/timer.h"
#include "cpp/task.h"
#include "cxx.h"

namespace OHOS::FileManagement::CloudDiskService {
struct TaskClosure;

class FfrtTimer {
public:
    FfrtTimer(uint64_t intervalMs, rust::Fn<void()> callback);
    ~FfrtTimer();
    FfrtTimer(const FfrtTimer &) = delete;
    FfrtTimer &operator=(const FfrtTimer &) = delete;
    bool IsValid() const;

private:
    rust::Fn<void()> callback_;
    ffrt_timer_t handle_;
};

std::unique_ptr<FfrtTimer> StartTimer(uint64_t intervalMs, rust::Fn<void()> callback);

void SubmitTask(rust::Box<TaskClosure> task);
std::unique_ptr<ffrt::task_handle> SubmitTaskWithHandle(rust::Box<TaskClosure> task);
void SubmitTaskAfter(rust::Box<TaskClosure> task, const ffrt::task_handle &dependency);
std::unique_ptr<ffrt::task_handle> CloneTaskHandle(const ffrt::task_handle &handle);
void WaitTask(const ffrt::task_handle &handle);
std::unique_ptr<ffrt::task_handle> SubmitDelayedTask(rust::Box<TaskClosure> task, uint64_t delayUs);
int32_t CancelQueueTask(const ffrt::task_handle &handle);
} // namespace OHOS::FileManagement::CloudDiskService

#endif // CLOUD_DISK_SERVICE_RS_FFRT_BRIDGE_H
