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

#include "ffrt_bridge.h"

#include <memory>
#include <utility>

#include "c/queue.h"
#include "cpp/queue.h"
#include "ffrt.rs.h"

namespace OHOS::FileManagement::CloudDiskService {
FfrtTimer::FfrtTimer(uint64_t intervalMs, rust::Fn<void()> callback) : callback_(callback)
{
    handle_ = ffrt_timer_start(ffrt_qos_default, intervalMs, this, [](void *data) {
        static_cast<FfrtTimer *>(data)->callback_();
    }, true);
}

FfrtTimer::~FfrtTimer()
{
    if (IsValid()) {
        // Wait for an in-flight tick before destroying callback_.
        ffrt_timer_stop(ffrt_qos_default, handle_);
    }
}

bool FfrtTimer::IsValid() const
{
    return handle_ >= 0;
}

std::unique_ptr<FfrtTimer> StartTimer(uint64_t intervalMs, rust::Fn<void()> callback)
{
    auto timer = std::make_unique<FfrtTimer>(intervalMs, callback);
    if (!timer->IsValid()) {
        return nullptr;
    }
    return timer;
}

void SubmitTask(rust::Box<TaskClosure> task)
{
    // ffrt::submit takes std::function, which requires a copyable callable.
    // Shared ownership keeps the Rust box alive until FFRT destroys the task.
    auto closure = std::make_shared<rust::Box<TaskClosure>>(std::move(task));
    ffrt::submit([closure] { (*closure)->run(); });
}

std::unique_ptr<ffrt::task_handle> SubmitTaskWithHandle(rust::Box<TaskClosure> task)
{
    auto closure = std::make_shared<rust::Box<TaskClosure>>(std::move(task));
    auto handle = ffrt::submit_h([closure] { (*closure)->run(); });
    if (handle == nullptr) {
        return nullptr;
    }
    return std::make_unique<ffrt::task_handle>(std::move(handle));
}

void SubmitTaskAfter(rust::Box<TaskClosure> task, const ffrt::task_handle &dependency)
{
    auto closure = std::make_shared<rust::Box<TaskClosure>>(std::move(task));
    ffrt::submit([closure] { (*closure)->run(); }, {dependency});
}

std::unique_ptr<ffrt::task_handle> CloneTaskHandle(const ffrt::task_handle &handle)
{
    return std::make_unique<ffrt::task_handle>(handle);
}

void WaitTask(const ffrt::task_handle &handle)
{
    ffrt::wait({handle});
}

std::unique_ptr<ffrt::task_handle> SubmitDelayedTask(rust::Box<TaskClosure> task, uint64_t delayUs)
{
    // One serial unload queue for this SA, matching C++ unloadQueue_.
    static ffrt::queue unloadQueue("clouddiskUnloadQueue");
    auto closure = std::make_shared<rust::Box<TaskClosure>>(std::move(task));
    auto handle = unloadQueue.submit_h([closure] { (*closure)->run(); }, ffrt::task_attr().delay(delayUs));
    if (handle == nullptr) {
        return nullptr;
    }
    return std::make_unique<ffrt::task_handle>(std::move(handle));
}

int32_t CancelQueueTask(const ffrt::task_handle &handle)
{
    // ffrt::queue::cancel forwards to this same API using only the task handle.
    return ffrt_queue_cancel(handle);
}
} // namespace OHOS::FileManagement::CloudDiskService
