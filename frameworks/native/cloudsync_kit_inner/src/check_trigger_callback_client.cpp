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

#include "check_trigger_callback_client.h"

#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {
void CheckTriggerCallbackClient::OnCheckTriggered(CheckTriggerReason reason, const std::string &bundleName,
    const std::string &triggerBundleName)
{
    if (!callback_) {
        LOGE("callback_ is null, maybe not registered");
        return;
    }
    callback_->OnCheckTriggered(reason, bundleName, triggerBundleName);
}
} // namespace OHOS::FileManagement::CloudSync
