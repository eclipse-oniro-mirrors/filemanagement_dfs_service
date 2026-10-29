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

#ifndef OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_CLIENT_MANAGER_H
#define OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_CLIENT_MANAGER_H

#include <mutex>
#include <vector>

#include "check_trigger_callback_info.h"
#include "nocopyable.h"

namespace OHOS::FileManagement::CloudSync {
class CheckTriggerCallbackClientManager : public NoCopyable {
public:
    static CheckTriggerCallbackClientManager &GetInstance();
    int32_t AddCallback(const CheckTriggerCallbackInfo &callbackInfo);
    int32_t RemoveCallback(const CheckTriggerCallbackInfo &callbackInfo);
    int32_t GetAllCallback(std::vector<CheckTriggerCallbackInfo> &callbackInfos);
    bool IsEmpty();

private:
    CheckTriggerCallbackClientManager() = default;
    std::mutex callbackListMutex_;
    std::vector<CheckTriggerCallbackInfo> callbackList_;
};
} // namespace OHOS::FileManagement::CloudSync

#endif // OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_CLIENT_MANAGER_H
