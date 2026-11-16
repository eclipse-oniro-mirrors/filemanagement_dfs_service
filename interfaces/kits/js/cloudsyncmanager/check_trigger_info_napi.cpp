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

#include "check_trigger_info_napi.h"

#include <cstdint>
#include <tuple>
#include <vector>

#include "dfs_error.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {
using namespace FileManagement::LibN;
using namespace std;

napi_value CheckTriggerInfoNapi::Constructor(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    funcArg.InitArgs(NARG_CNT::ZERO);

    auto infoEntity = make_unique<CheckTriggerInfoEntity>();
    if (infoEntity == nullptr) {
        LOGE("Failed to request heap memory.");
        return nullptr;
    }
    if (!NClass::SetEntityFor<CheckTriggerInfoEntity>(env, funcArg.GetThisVar(), move(infoEntity))) {
        LOGE("Failed to set CheckTriggerInfoEntity.");
        return nullptr;
    }
    return funcArg.GetThisVar();
}

napi_value CheckTriggerInfoNapi::GetBundleName(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    funcArg.InitArgs(NARG_CNT::ZERO);

    string bundleName;
    auto infoEntity = NClass::GetEntityOf<CheckTriggerInfoEntity>(env, funcArg.GetThisVar());
    if (infoEntity == nullptr) {
        LOGE("Failed to get CheckTriggerInfoEntity.");
    } else {
        bundleName = infoEntity->bundleName;
    }
    return NVal::CreateUTF8String(env, bundleName).val_;
}

napi_value CheckTriggerInfoNapi::GetTriggerBundleName(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    funcArg.InitArgs(NARG_CNT::ZERO);

    string triggerBundleName;
    auto infoEntity = NClass::GetEntityOf<CheckTriggerInfoEntity>(env, funcArg.GetThisVar());
    if (infoEntity == nullptr) {
        LOGE("Failed to get CheckTriggerInfoEntity.");
    } else {
        triggerBundleName = infoEntity->triggerBundleName;
    }
    return NVal::CreateUTF8String(env, triggerBundleName).val_;
}

napi_value CheckTriggerInfoNapi::GetReason(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    funcArg.InitArgs(NARG_CNT::ZERO);

    int32_t reason = 0;
    auto infoEntity = NClass::GetEntityOf<CheckTriggerInfoEntity>(env, funcArg.GetThisVar());
    if (infoEntity == nullptr) {
        LOGE("Failed to get CheckTriggerInfoEntity.");
    } else {
        reason = infoEntity->reason;
    }
    return NVal::CreateInt32(env, reason).val_;
}

std::string CheckTriggerInfoNapi::GetClassName()
{
    return className_;
}

bool CheckTriggerInfoNapi::Export()
{
    vector<napi_property_descriptor> props = {
        NVal::DeclareNapiGetter("bundleName", GetBundleName),
        NVal::DeclareNapiGetter("triggerBundleName", GetTriggerBundleName),
        NVal::DeclareNapiGetter("reason", GetReason),
    };

    string className = GetClassName();
    bool succ = false;
    napi_value classValue = nullptr;
    tie(succ, classValue) =
        NClass::DefineClass(exports_.env_, className, CheckTriggerInfoNapi::Constructor, move(props));
    if (!succ) {
        LOGE("Define class exceptions");
        NError(Convert2JsErrNum(E_SERVICE_INNER_ERROR)).ThrowErr(exports_.env_);
        return false;
    }
    succ = NClass::SaveClass(exports_.env_, className, classValue);
    if (!succ) {
        LOGE("Save class exceptions");
        NError(Convert2JsErrNum(E_SERVICE_INNER_ERROR)).ThrowErr(exports_.env_);
        return false;
    }

    return exports_.AddProp(className, classValue);
}
} // namespace OHOS::FileManagement::CloudSync
