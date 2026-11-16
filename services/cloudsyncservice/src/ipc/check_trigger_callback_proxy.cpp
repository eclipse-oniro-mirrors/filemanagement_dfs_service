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

#include "ipc/check_trigger_callback_proxy.h"

#include "dfs_error.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {
void CheckTriggerCallbackProxy::OnCheckTriggered(CheckTriggerReason reason, const std::string &bundleName,
    const std::string &triggerBundleName)
{
    LOGI("Start");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOGE("Failed to write interface token");
        return;
    }
    if (!data.WriteInt32(static_cast<int32_t>(reason))) {
        LOGE("Failed to write check trigger reason");
        return;
    }
    if (!data.WriteString(bundleName)) {
        LOGE("Failed to write check trigger bundle name");
        return;
    }
    if (!data.WriteString(triggerBundleName)) {
        LOGE("Failed to write check trigger trigger bundle name");
        return;
    }

    auto remote = Remote();
    if (remote == nullptr) {
        LOGE("remote is nullptr");
        return;
    }
    int32_t ret = remote->SendRequest(ICheckTriggerCallback::SERVICE_CMD_ON_CHECK_TRIGGERED, data, reply, option);
    if (ret != E_OK) {
        LOGE("Failed to send out the request, ret:%{public}d", ret);
    }
}
} // namespace OHOS::FileManagement::CloudSync
