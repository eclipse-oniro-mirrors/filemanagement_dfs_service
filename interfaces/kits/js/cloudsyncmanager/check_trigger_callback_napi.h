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

#ifndef OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_NAPI_H
#define OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_NAPI_H

#include <memory>
#include <mutex>
#include <string>

#include "cloud_check_trigger_callback.h"
#include "filemgmt_libn.h"

namespace OHOS::FileManagement::CloudSync {
class CheckTriggerCallbackNapi : public CloudCheckTriggerCallback,
                                 public std::enable_shared_from_this<CheckTriggerCallbackNapi> {
public:
    CheckTriggerCallbackNapi(napi_env env, napi_value fun);
    ~CheckTriggerCallbackNapi() override;
    void OnCheckTriggered(CheckTriggerReason reason, const std::string &bundleName,
        const std::string &triggerBundleName) override;
    void DeleteReference() override;

private:
    void OnComplete(CheckTriggerReason reason, const std::string &bundleName, const std::string &triggerBundleName);
    napi_env env_;
    napi_ref callbackRef_{nullptr};
    std::mutex refMtx_;
};
} // namespace OHOS::FileManagement::CloudSync

#endif // OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_NAPI_H
