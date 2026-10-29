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

#include "check_trigger_callback_ani.h"

#include <tuple>
#include <vector>

#include "ani_signature_builder.h"
#include "ani_utils.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {
using namespace arkts::ani_signature;

constexpr int32_t ANI_SCOPE_SIZE = 16;

CheckTriggerCallbackAni::CheckTriggerCallbackAni(ani_env *env, ani_ref fun) : vm_(nullptr), cbOnRef_(nullptr)
{
    if (env == nullptr || env->GetVM(&vm_)) {
        LOGE("CheckTriggerCallbackAni get vm failed.");
        return;
    }
    if (fun != nullptr) {
        ani_ref globalRef = nullptr;
        ani_status ret = env->GlobalReference_Create(fun, &globalRef);
        if (ret != ANI_OK) {
            LOGE("CheckTriggerCallbackAni create global reference failed. ret = %{public}d", ret);
            return;
        }
        cbOnRef_ = globalRef;
    }
}

ani_status CheckTriggerCallbackAni::GetCheckTriggerInfo(CheckTriggerReason reason, const std::string &bundleName,
    const std::string &triggerBundleName, const ani_class &cls, ani_object &info)
{
    ani_env *env = nullptr;
    if (vm_ == nullptr || vm_->GetEnv(ANI_VERSION_1, &env)) {
        LOGE("CheckTriggerCallbackAni get env failed.");
        return ANI_INVALID_ARGS;
    }
    ani_method ctor;
    std::string ct = Builder::BuildConstructorName();
    std::string argSign = Builder::BuildSignatureDescriptor({
        Builder::BuildClass("std.core.String"),
        Builder::BuildClass("std.core.String"),
        Builder::BuildEnum("@ohos.file.cloudSyncManager.cloudSyncManager.CheckTriggerReason")
    });
    ani_status ret = env->Class_FindMethod(cls, ct.c_str(), argSign.c_str(), &ctor);
    if (ret != ANI_OK) {
        LOGE("find ctor method failed. ret = %{public}d", ret);
        return ret;
    }

    ani_enum reasonEnum;
    Type reasonSign = Builder::BuildEnum("@ohos.file.cloudSyncManager.cloudSyncManager.CheckTriggerReason");
    ret = env->FindEnum(reasonSign.Descriptor().c_str(), &reasonEnum);
    if (ret != ANI_OK) {
        LOGE("FindEnum reasonSign failed. ret = %{public}d", ret);
        return ret;
    }

    ani_enum_item reasonItem;
    ret = env->Enum_GetEnumItemByIndex(reasonEnum, static_cast<ani_int>(reason), &reasonItem);
    if (ret != ANI_OK) {
        LOGE("Enum_GetEnumItemByIndex reasonEnum failed. ret = %{public}d", ret);
        return ret;
    }

    auto bundleNameResult = ANIUtils::ToAniString(env, bundleName);
    if (!std::get<0>(bundleNameResult)) {
        LOGE("create bundle name string failed.");
        return ANI_INVALID_ARGS;
    }
    ani_string bundleNameStr = std::get<1>(bundleNameResult);

    auto triggerBundleNameResult = ANIUtils::ToAniString(env, triggerBundleName);
    if (!std::get<0>(triggerBundleNameResult)) {
        LOGE("create trigger bundle name string failed.");
        return ANI_INVALID_ARGS;
    }
    ani_string triggerBundleNameStr = std::get<1>(triggerBundleNameResult);

    ret = env->Object_New(cls, ctor, &info, bundleNameStr, triggerBundleNameStr, reasonItem);
    if (ret != ANI_OK) {
        LOGE("create new check trigger info object failed. ret = %{public}d", ret);
        return ret;
    }

    return ANI_OK;
}

void CheckTriggerCallbackAni::OnCheckTriggered(CheckTriggerReason reason, const std::string &bundleName,
    const std::string &triggerBundleName)
{
    std::shared_ptr<CheckTriggerCallbackAni> callbackImpl = shared_from_this();
    auto task = [callbackImpl, reason, bundleName, triggerBundleName]() {
        LOGI("OnCheckTriggered reason is %{public}d", reason);
        ani_env *tmpEnv = nullptr;
        if (callbackImpl == nullptr || callbackImpl->cbOnRef_ == nullptr || callbackImpl->vm_ == nullptr ||
            callbackImpl->vm_->GetEnv(ANI_VERSION_1, &tmpEnv)) {
            LOGE("CheckTriggerCallbackAni get env failed.");
            return;
        }
        ani_size nrRefs = ANI_SCOPE_SIZE;
        ani_status ret = tmpEnv->CreateLocalScope(nrRefs);
        if (ret != ANI_OK) {
            LOGE("create local scope failed. ret = %{public}d", ret);
            return;
        }
        Type clsName = Builder::BuildClass("@ohos.file.cloudSyncManager.cloudSyncManager.CheckTriggerInfoInner");
        ani_class cls;
        ret = tmpEnv->FindClass(clsName.Descriptor().c_str(), &cls);
        if (ret != ANI_OK) {
            LOGE("find class failed. ret = %{public}d", ret);
            tmpEnv->DestroyLocalScope();
            return;
        }
        ani_object info;
        ret = callbackImpl->GetCheckTriggerInfo(reason, bundleName, triggerBundleName, cls, info);
        if (ret != ANI_OK) {
            LOGE("GetCheckTriggerInfo function failed. ret = %{public}d", ret);
            tmpEnv->DestroyLocalScope();
            return;
        }
        ani_ref ref;
        ani_fn_object etsCb = reinterpret_cast<ani_fn_object>(callbackImpl->cbOnRef_);
        std::vector<ani_ref> vec = { info };
        ret = tmpEnv->FunctionalObject_Call(etsCb, 1, vec.data(), &ref);
        if (ret != ANI_OK) {
            LOGE("ani call function failed. ret = %{public}d", ret);
            tmpEnv->DestroyLocalScope();
            return;
        }
        ret = tmpEnv->DestroyLocalScope();
        if (ret != ANI_OK) {
            LOGE("failed to DestroyLocalScope. ret = %{public}d", ret);
        }
    };
    if (!ANIUtils::SendEventToMainThread(task)) {
        LOGE("failed to send event");
    }
}

void CheckTriggerCallbackAni::DeleteReference()
{
    ani_env *env = nullptr;
    if (vm_ == nullptr || vm_->GetEnv(ANI_VERSION_1, &env)) {
        LOGE("CheckTriggerCallbackAni get env failed.");
        return;
    }
    if (cbOnRef_ != nullptr) {
        env->GlobalReference_Delete(cbOnRef_);
        cbOnRef_ = nullptr;
    }
}

CheckTriggerCallbackAni::~CheckTriggerCallbackAni()
{
    DeleteReference();
}
} // namespace OHOS::FileManagement::CloudSync
