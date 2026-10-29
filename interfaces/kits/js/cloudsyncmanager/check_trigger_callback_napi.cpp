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

#include "check_trigger_callback_napi.h"

#include "check_trigger_info_napi.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {
using namespace FileManagement::LibN;

CheckTriggerCallbackNapi::CheckTriggerCallbackNapi(napi_env env, napi_value fun) : env_(env)
{
    if (fun != nullptr) {
        std::lock_guard<std::mutex> lock(refMtx_);
        auto status = napi_create_reference(env_, fun, 1, &callbackRef_);
        if (status != napi_ok) {
            callbackRef_ = nullptr;
            LOGE("Failed to create check trigger callback reference, status: %{public}d", status);
        }
    }
}

CheckTriggerCallbackNapi::~CheckTriggerCallbackNapi()
{
    DeleteReference();
}

void CheckTriggerCallbackNapi::OnCheckTriggered(CheckTriggerReason reason, const std::string &bundleName,
    const std::string &triggerBundleName)
{
    auto callbackImpl = shared_from_this();
    auto task = [callbackImpl, reason, bundleName, triggerBundleName]() {
        callbackImpl->OnComplete(reason, bundleName, triggerBundleName);
    };
    napi_status status = napi_send_event(env_, task, napi_eprio_immediate, "cloudSyncManager.CheckTrigger");
    if (status != napi_ok) {
        LOGE("Failed to send check trigger event, status: %{public}d", status);
    }
}

void CheckTriggerCallbackNapi::OnComplete(CheckTriggerReason reason, const std::string &bundleName,
    const std::string &triggerBundleName)
{
    napi_ref callbackRef = nullptr;
    {
        std::lock_guard<std::mutex> lock(refMtx_);
        callbackRef = callbackRef_;
    }
    if (callbackRef == nullptr) {
        LOGE("check trigger callback reference is null");
        return;
    }

    napi_handle_scope scope = nullptr;
    napi_status status = napi_open_handle_scope(env_, &scope);
    if (status != napi_ok || scope == nullptr) {
        LOGE("Failed to open handle scope, status: %{public}d", status);
        return;
    }
    napi_value jsCallback = nullptr;
    status = napi_get_reference_value(env_, callbackRef, &jsCallback);
    if (status != napi_ok || jsCallback == nullptr) {
        LOGE("Failed to get callback reference, status: %{public}d", status);
        napi_close_handle_scope(env_, scope);
        return;
    }

    napi_value infoVal = NClass::InstantiateClass(env_, CheckTriggerInfoNapi::className_, {});
    if (infoVal == nullptr) {
        LOGE("Failed to instantiate CheckTriggerInfoInner");
        napi_close_handle_scope(env_, scope);
        return;
    }
    auto infoEntity = NClass::GetEntityOf<CheckTriggerInfoEntity>(env_, infoVal);
    if (infoEntity == nullptr) {
        LOGE("Failed to get CheckTriggerInfoEntity");
        napi_close_handle_scope(env_, scope);
        return;
    }
    infoEntity->bundleName = bundleName;
    infoEntity->triggerBundleName = triggerBundleName;
    infoEntity->reason = static_cast<int32_t>(reason);

    napi_value result = nullptr;
    status = napi_call_function(env_, nullptr, jsCallback, 1, &infoVal, &result);
    if (status != napi_ok) {
        LOGE("Failed to call check trigger callback, status: %{public}d", status);
    }
    napi_close_handle_scope(env_, scope);
}

void CheckTriggerCallbackNapi::DeleteReference()
{
    std::lock_guard<std::mutex> lock(refMtx_);
    if (callbackRef_ != nullptr) {
        napi_delete_reference(env_, callbackRef_);
        callbackRef_ = nullptr;
    }
}
} // namespace OHOS::FileManagement::CloudSync
