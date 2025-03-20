/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cloud_file_cache_ani.h"
#include "error_handler.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {

static CloudFileCacheCore *CloudFileCacheUnwrap(ani_env *env, ani_object object)
{
    ani_long nativePtr;
    auto ret = env->Object_GetFieldByName_Long(object, "nativePtr", &nativePtr);
    if (ret != ANI_OK) {
        LOGE("Unwrap cloudFileCacheCore err: %{public}d", static_cast<int32_t>(ret));
        return nullptr;
    }
    std::uintptr_t ptrValue = static_cast<std::uintptr_t>(nativePtr);
    CloudFileCacheCore *cloudFileCache = reinterpret_cast<CloudFileCacheCore *>(ptrValue);
    return cloudFileCache;
}

static ani_status AniString2String(ani_env *env, ani_string str, std::string &res)
{
    ani_size strSize;
    ani_status ret = env->String_GetUTF8Size(str, &strSize);
    if (ret != ANI_OK) {
        LOGE("ani string get size failed. ret = %{public}d", static_cast<int32_t>(ret));
        return ret;
    }
    std::vector<char> buffer(strSize + 1);
    char *utf8Buffer = buffer.data();
    ani_size byteWrite = 0;
    ret = env->String_GetUTF8(str, utf8Buffer, strSize + 1, &byteWrite);
    if (ret != ANI_OK) {
        LOGE("ani string to string failed. ret = %{public}d", static_cast<int32_t>(ret));
        return ret;
    }
    utf8Buffer[byteWrite] = '\0';
    res = std::string(utf8Buffer);
    return ANI_OK;
}

void CloudFileCacheAni::CloudFileCacheConstructor(ani_env *env, ani_object object)
{
    ani_namespace ns {};
    ani_status ret = env->FindNamespace("L@ohos/file/cloudSync/cloudSync;", &ns);
    if (ret != ANI_OK) {
        LOGE("find namespace failed. ret = %{public}d", static_cast<int32_t>(ret));
        ErrorHandler::Throw(env, static_cast<int32_t>(ret));
        return;
    }
    static const char *className = "LCloudFileCache;";
    ani_class cls;
    ret = env->Namespace_FindClass(ns, className, &cls);
    if (ret != ANI_OK) {
        LOGE("find class failed. ret = %{public}d", static_cast<int32_t>(ret));
        ErrorHandler::Throw(env, static_cast<int32_t>(ret));
        return;
    }

    ani_method bindNativePtr;
    ret = env->Class_FindMethod(cls, "bindNativePtr", "J:V", &bindNativePtr);
    if (ret != ANI_OK) {
        LOGE("find class ctor. ret = %{public}d", static_cast<int32_t>(ret));
        ErrorHandler::Throw(env, static_cast<int32_t>(ret));
        return;
    }

    FsResult<CloudFileCacheCore *> data = CloudFileCacheCore::Constructor();
    if (!data.IsSuccess()) {
        LOGE("cloudFileCache constructor failed.");
        const auto &err = data.GetError();
        ErrorHandler::Throw(env, err);
        return;
    }

    const CloudFileCacheCore *cloudFileCache = data.GetData().value();
    ret = env->Object_CallMethod_Void(object, bindNativePtr, reinterpret_cast<ani_long>(cloudFileCache));
    if (ret != ANI_OK) {
        LOGE("bindNativePtr failed.");
        delete cloudFileCache;
        ErrorHandler::Throw(env, static_cast<int32_t>(ret));
    }
}

void CloudFileCacheAni::CloudFileCacheOn(ani_env *env, ani_object object, ani_string evt, ani_object fun)
{
    std::string event;
    ani_status ret = AniString2String(env, evt, event);
    if (ret != ANI_OK) {
        ErrorHandler::Throw(env, static_cast<int32_t>(ret));
        return;
    }

    ani_ref cbOnRef;
    ret = env->GlobalReference_Create(reinterpret_cast<ani_ref>(fun), &cbOnRef);
    if (ret != ANI_OK) {
        ErrorHandler::Throw(env, static_cast<int32_t>(ret));
        return;
    }
    std::shared_ptr<CloudDownloadCallbackAniImpl> callback = nullptr;
    if (event == "progress") {
        callback = std::make_shared<CloudDownloadCallbackAniImpl>(env, cbOnRef);
    } else if (event == "multiProgress") {
        callback = std::make_shared<CloudDownloadCallbackAniImpl>(env, cbOnRef, true);
    }

    auto cloudFileCache = CloudFileCacheUnwrap(env, object);
    if (cloudFileCache == nullptr) {
        LOGE("Cannot wrap cloudFileCache.");
        ErrorHandler::Throw(env, UNKNOWN_ERR);
        return;
    }
    auto data = cloudFileCache->DoOn(event, callback);
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloudFileCache do on failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
    }
}

void CloudFileCacheAni::CloudFileCacheOff0(ani_env *env, ani_object object, ani_string evt, ani_object fun)
{
    ani_ref cbOnRef;
    ani_status ret = env->GlobalReference_Create(reinterpret_cast<ani_ref>(fun), &cbOnRef);
    if (ret != ANI_OK) {
        ErrorHandler::Throw(env, static_cast<int32_t>(ret));
        return;
    }
    auto callback = std::make_shared<CloudDownloadCallbackAniImpl>(env, cbOnRef);

    std::string event;
    ret = AniString2String(env, evt, event);
    if (ret != ANI_OK) {
        ErrorHandler::Throw(env, static_cast<int32_t>(ret));
        return;
    }

    auto cloudFileCache = CloudFileCacheUnwrap(env, object);
    if (cloudFileCache == nullptr) {
        LOGE("Cannot wrap cloudFileCache.");
        ErrorHandler::Throw(env, UNKNOWN_ERR);
        return;
    }
    auto data = cloudFileCache->DoOff(event, callback);
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloudFileCache do off failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
    }
}

void CloudFileCacheAni::CloudFileCacheOff1(ani_env *env, ani_object object, ani_string evt)
{
    std::string event;
    ani_status ret = AniString2String(env, evt, event);
    if (ret != ANI_OK) {
        ErrorHandler::Throw(env, static_cast<int32_t>(ret));
        return;
    }

    auto cloudFileCache = CloudFileCacheUnwrap(env, object);
    if (cloudFileCache == nullptr) {
        LOGE("Cannot wrap cloudFileCache.");
        ErrorHandler::Throw(env, UNKNOWN_ERR);
        return;
    }
    auto data = cloudFileCache->DoOff(event);
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloudFileCache do off failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
    }
}

void CloudFileCacheAni::CloudFileCacheStart(ani_env *env, ani_object object, ani_string uri)
{
    std::string uriInput;
    ani_status ret = AniString2String(env, uri, uriInput);
    if (ret != ANI_OK) {
        ErrorHandler::Throw(env, static_cast<int32_t>(ret));
        return;
    }

    auto cloudFileCache = CloudFileCacheUnwrap(env, object);
    if (cloudFileCache == nullptr) {
        LOGE("Cannot wrap cloudFileCache.");
        ErrorHandler::Throw(env, UNKNOWN_ERR);
        return;
    }
    auto data = cloudFileCache->DoStart(uriInput);
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloudFileCache do start failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
    }
}

void CloudFileCacheAni::CloudFileCacheStop(ani_env *env, ani_object object, ani_string uri, ani_boolean needClean)
{
    std::string uriInput;
    ani_status ret = AniString2String(env, uri, uriInput);
    if (ret != ANI_OK) {
        ErrorHandler::Throw(env, static_cast<int32_t>(ret));
        return;
    }

    auto cloudFileCache = CloudFileCacheUnwrap(env, object);
    if (cloudFileCache == nullptr) {
        LOGE("Cannot wrap cloudFileCache.");
        ErrorHandler::Throw(env, UNKNOWN_ERR);
        return;
    }

    bool needCleanInput = needClean;

    auto data = cloudFileCache->DoStop(uriInput, needCleanInput);
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloudFileCache do stop failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
    }
}

void CloudFileCacheAni::CloudFileCacheCleanCache(ani_env *env, ani_object object, ani_string uri)
{
    std::string uriInput;
    ani_status ret = AniString2String(env, uri, uriInput);
    if (ret != ANI_OK) {
        ErrorHandler::Throw(env, static_cast<int32_t>(ret));
        return;
    }

    auto cloudFileCache = CloudFileCacheUnwrap(env, object);
    if (cloudFileCache == nullptr) {
        LOGE("Cannot wrap cloudFileCache.");
        ErrorHandler::Throw(env, UNKNOWN_ERR);
        return;
    }
    auto data = cloudFileCache->CleanCache(uriInput);
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloudFileCache do cleanCache failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
    }
}
} // namespace OHOS::FileManagement::CloudSync