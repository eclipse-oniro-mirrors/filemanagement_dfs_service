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

#include "cloud_sync_ani.h"

#include "ani_utils.h"
#include "cloud_sync_callback_ani.h"
#include "dfs_error.h"
#include "dfsu_access_token_helper.h"
#include "error_handler.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {

using namespace arkts::ani_signature;

const string FILE_SCHEME = "file";
const int32_t E_IPCSS = 13600001;
const int32_t E_PERMISSION = 201;
thread_local unique_ptr<ChangeListenerAni> g_listObj = nullptr;
mutex CloudSyncAni::sOnOffMutex_;

static CloudSyncCore *CloudSyncUnwrap(ani_env *env, ani_object object)
{
    ani_long nativePtr;
    auto ret = env->Object_GetFieldByName_Long(object, "nativePtr", &nativePtr);
    if (ret != ANI_OK) {
        LOGE("Unwrap cloudsyncCore err: %{public}d", static_cast<int32_t>(ret));
        return nullptr;
    }
    std::uintptr_t ptrValue = static_cast<std::uintptr_t>(nativePtr);
    CloudSyncCore *cloudSync = reinterpret_cast<CloudSyncCore *>(ptrValue);
    return cloudSync;
}

void CloudSyncAni::CloudSyncConstructor(ani_env *env, ani_object object)
{
    ani_namespace ns {};
    Namespace nsSign = Builder::BuildNamespace("@ohos.file.cloudSync.cloudSync");
    ani_status ret = env->FindNamespace(nsSign.Descriptor().c_str(), &ns);
    if (ret != ANI_OK) {
        LOGE("find namespace failed. ret = %{public}d", static_cast<int32_t>(ret));
        ErrorHandler::Throw(env, ENOMEM);
        return;
    }
    Type clsName = Builder::BuildClass("GallerySync");
    ani_class cls;
    ret = env->Namespace_FindClass(ns, clsName.Descriptor().c_str(), &cls);
    if (ret != ANI_OK) {
        LOGE("find class failed. ret = %{public}d", static_cast<int32_t>(ret));
        ErrorHandler::Throw(env, ENOMEM);
        return;
    }

    ani_method bindNativePtr;
    std::string bindSign = Builder::BuildSignatureDescriptor({Builder::BuildLong()});
    ret = env->Class_FindMethod(cls, "bindNativePtr", bindSign.c_str(), &bindNativePtr);
    if (ret != ANI_OK) {
        LOGE("find class ctor. ret = %{public}d", static_cast<int32_t>(ret));
        ErrorHandler::Throw(env, ENOMEM);
        return;
    }

    FsResult<CloudSyncCore *> data = CloudSyncCore::Constructor();
    if (!data.IsSuccess()) {
        LOGE("cloudsync constructor failed.");
        const auto &err = data.GetError();
        ErrorHandler::Throw(env, err);
        return;
    }

    const CloudSyncCore *cloudSync = data.GetData().value();
    ret = env->Object_CallMethod_Void(object, bindNativePtr, reinterpret_cast<ani_long>(cloudSync));
    if (ret != ANI_OK) {
        LOGE("bindNativePtr failed.");
        delete cloudSync;
        ErrorHandler::Throw(env, ENOMEM);
    }
}

void CloudSyncAni::CloudSyncOn(ani_env *env, ani_object object, ani_string evt, ani_object fun)
{
    ani_ref cbOnRef;
    ani_status ret = env->GlobalReference_Create(reinterpret_cast<ani_ref>(fun), &cbOnRef);
    if (ret != ANI_OK) {
        LOGE("cloud sync on create global reference failed. ret = %{public}d", ret);
        ErrorHandler::Throw(env, E_IPCSS);
        return;
    }
    auto callback = std::make_shared<CloudSyncCallbackAniImpl>(env, cbOnRef);

    std::string event;
    ret = ANIUtils::AniString2String(env, evt, event);
    if (ret != ANI_OK) {
        ErrorHandler::Throw(env, E_IPCSS);
        return;
    }

    auto cloudSync = CloudSyncUnwrap(env, object);
    if (cloudSync == nullptr) {
        LOGE("Cannot wrap cloudsync.");
        ErrorHandler::Throw(env, E_IPCSS);
        return;
    }
    auto data = cloudSync->DoOn(event, callback);
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloud sync do on failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
    }
}

void CloudSyncAni::CloudSyncOff0(ani_env *env, ani_object object, ani_string evt, ani_object fun)
{
    ani_ref cbOnRef;
    ani_status ret = env->GlobalReference_Create(reinterpret_cast<ani_ref>(fun), &cbOnRef);
    if (ret != ANI_OK) {
        LOGE("cloud sync off create global reference failed. ret = %{public}d", ret);
        ErrorHandler::Throw(env, E_IPCSS);
        return;
    }
    auto callback = std::make_shared<CloudSyncCallbackAniImpl>(env, cbOnRef);

    std::string event;
    ret = ANIUtils::AniString2String(env, evt, event);
    if (ret != ANI_OK) {
        ErrorHandler::Throw(env, E_IPCSS);
        return;
    }

    auto cloudSync = CloudSyncUnwrap(env, object);
    if (cloudSync == nullptr) {
        LOGE("Cannot wrap cloudsync.");
        ErrorHandler::Throw(env, E_IPCSS);
        return;
    }
    auto data = cloudSync->DoOff(event, callback);
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloud sync do off failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
    }
}

void CloudSyncAni::CloudSyncOff1(ani_env *env, ani_object object, ani_string evt)
{
    std::string event;
    ani_status ret = ANIUtils::AniString2String(env, evt, event);
    if (ret != ANI_OK) {
        LOGE("cloud sync off create global reference failed. ret = %{public}d", ret);
        ErrorHandler::Throw(env, E_IPCSS);
        return;
    }

    auto cloudSync = CloudSyncUnwrap(env, object);
    if (cloudSync == nullptr) {
        LOGE("Cannot wrap cloudsync.");
        ErrorHandler::Throw(env, E_IPCSS);
        return;
    }
    auto data = cloudSync->DoOff(event);
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloud sync do off failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
    }
}

void CloudSyncAni::CloudSyncStart(ani_env *env, ani_object object)
{
    auto cloudSync = CloudSyncUnwrap(env, object);
    if (cloudSync == nullptr) {
        LOGE("Cannot wrap cloudsync.");
        ErrorHandler::Throw(env, E_PERMISSION);
        return;
    }
    auto data = cloudSync->DoStart();
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloud sync do start failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
    }
}

void CloudSyncAni::CloudSyncStop(ani_env *env, ani_object object)
{
    auto cloudSync = CloudSyncUnwrap(env, object);
    if (cloudSync == nullptr) {
        LOGE("Cannot wrap cloudsync.");
        ErrorHandler::Throw(env, E_PERMISSION);
        return;
    }
    auto data = cloudSync->DoStop();
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloud sync do stop failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
    }
}

void CloudSyncAni::OptimizeStorage(ani_env *env, ani_class clazz)
{
    auto data = CloudSyncCore::DoOptimizeStorage();
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloud sync do OptimizeStorage failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
    }
}

void CloudSyncAni::StartOptimizeStorage(ani_env *env, ani_class clazz, ani_object optim, ani_object fun)
{
    OptimizeSpaceOptions optimizeOptions {};
    ani_double totalSize;
    ani_status ret = env->Object_GetPropertyByName_Double(optim, "totalSize", &totalSize);
    if (ret != ANI_OK) {
        LOGE("get totalSize failed. ret = %{public}d", ret);
        ErrorHandler::Throw(env, E_IPCSS);
        return;
    }
    ani_double agingDays;
    ret = env->Object_GetPropertyByName_Double(optim, "agingDays", &agingDays);
    if (ret != ANI_OK) {
        LOGE("get agingDays failed. ret = %{public}d", ret);
        ErrorHandler::Throw(env, E_IPCSS);
        return;
    }

    LOGI("totalSize: %{public}lld, agingDays:%{public}d",
        static_cast<long long>(totalSize), static_cast<int32_t>(agingDays));
    optimizeOptions.totalSize = static_cast<int64_t>(totalSize);
    optimizeOptions.agingDays = static_cast<int32_t>(agingDays);

    ani_ref cbOnRef;
    ret = env->GlobalReference_Create(reinterpret_cast<ani_ref>(fun), &cbOnRef);
    if (ret != ANI_OK) {
        LOGE("start optim storage create reference failed. ret = %{public}d", ret);
        ErrorHandler::Throw(env, E_IPCSS);
        return;
    }
    auto callback = std::make_shared<CloudOptimizeCallbackAniImpl>(env, cbOnRef);

    auto data = CloudSyncCore::DoStartOptimizeStorage(optimizeOptions, callback);
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloud sync do StopOptimizeStorage failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
    }
}

void CloudSyncAni::StopOptimizeStorage(ani_env *env, ani_class clazz)
{
    auto data = CloudSyncCore::DoStopOptimizeStorage();
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloud sync do StopOptimizeStorage failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
    }
}

ani_int CloudSyncAni::GetFileSyncState(ani_env *env, ani_class clazz, ani_string path)
{
    string filePath;
    ani_status ret = ANIUtils::AniString2String(env, path, filePath);
    if (ret != ANI_OK) {
        LOGE("ani string get size failed. ret = %{public}d", static_cast<int32_t>(ret));
        ErrorHandler::Throw(env, E_IPCSS);
        return static_cast<int32_t>(ret);
    }
    auto data = CloudSyncCore::DoGetFileSyncState(filePath);
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloud sync do GetFileSyncState failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
        return err.GetErrNo();
    }
    return static_cast<ani_int>(data.GetData().value());
}

ani_int CloudSyncAni::GetCoreFileSyncState(ani_env *env, ani_class clazz, ani_string path)
{
    string filePath;
    ani_status ret = ANIUtils::AniString2String(env, path, filePath);
    if (ret != ANI_OK) {
        LOGE("ani string get size failed. ret = %{public}d", static_cast<int32_t>(ret));
        ErrorHandler::Throw(env, JsErrCode::E_INNER_FAILED);
        return static_cast<int32_t>(ret);
    }
    auto data = CloudSyncCore::DoGetCoreFileSyncState(filePath);
    if (!data.IsSuccess()) {
        const auto &err = data.GetError();
        LOGE("cloud sync do GetCoreFileSyncState failed, ret = %{public}d", err.GetErrNo());
        ErrorHandler::Throw(env, err);
        return err.GetErrNo();
    }
    return static_cast<ani_int>(data.GetData().value());
}

static bool CheckIsValidUri(Uri uri)
{
    string scheme = uri.GetScheme();
    if (scheme != FILE_SCHEME) {
        return false;
    }
    string sandboxPath = uri.GetPath();
    char realPath[PATH_MAX + 1] { '\0' };
    if (sandboxPath.length() > PATH_MAX) {
        LOGE("sandboxPath length is too long.");
        return false;
    }
    if (realpath(sandboxPath.c_str(), realPath) == nullptr) {
        LOGE("realpath failed with %{public}d", errno);
        return false;
    }
    if (strncmp(realPath, sandboxPath.c_str(), sandboxPath.size()) != 0) {
        LOGE("sandboxPath is not equal to realPath");
        return false;
    }
    if (sandboxPath.find("/data/storage/el2/cloud") != 0) {
        LOGE("not surported uri");
        return false;
    }
    return true;
}

int32_t CloudSyncAni::RegisterToObs(const RegisterParams &registerParams)
{
    auto observer = make_shared<CloudNotifyObserver>(*g_listObj, registerParams.uri, registerParams.cbOnRef);
    Uri uri(registerParams.uri);
    auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
    if (obsMgrClient == nullptr) {
        LOGE("get DataObsMgrClient failed");
        return E_SA_LOAD_FAILED;
    }
    sptr<ObserverImpl> obs = ObserverImpl::GetObserver(uri, observer);
    if (obs == nullptr) {
        LOGE("new ObserverImpl failed");
        return E_INVAL_ARG;
    }
    ErrCode ret = obsMgrClient->RegisterObserverExt(uri, obs, registerParams.recursion);
    if (ret != E_OK) {
        LOGE("ObsMgr register fail");
        ObserverImpl::DeleteObserver(uri, observer);
        return E_INVAL_ARG;
    }
    lock_guard<mutex> lock(CloudSyncAni::sOnOffMutex_);
    g_listObj->observers_.push_back(observer);
    return E_OK;
}

bool CloudSyncAni::CheckRef(ani_env *env, ani_ref ref, ChangeListenerAni &listObj, const string &uri)
{
    ani_boolean isSame = false;
    shared_ptr<CloudNotifyObserver> obs;
    string obsUri;
    {
        std::lock_guard<mutex> lock(CloudSyncAni::sOnOffMutex_);
        for (auto it = listObj.observers_.begin(); it < listObj.observers_.end(); it++) {
            ani_status ret = env->Reference_StrictEquals(ref, (*it)->ref_, &isSame);
            if (ret != ANI_OK) {
                LOGE("compare ref failed. ret = %{public}d", static_cast<int32_t>(ret));
                return false;
            }

            if (isSame) {
                obsUri = (*it)->uri_;
                if (uri.compare(obsUri) != 0) {
                    return true;
                }
                return false;
            }
        }
    }
    return true;
}

int32_t CloudSyncAni::GetRegisterParams(
    ani_env *env, ani_string uri, ani_boolean recursion, ani_object fun, RegisterParams &registerParams)
{
    std::string uriInput;
    ani_status ret = ANIUtils::AniString2String(env, uri, uriInput);
    if (ret != ANI_OK) {
        LOGE("get register param get uri failed. ret = %{public}d", static_cast<int32_t>(ret));
        return static_cast<int32_t>(ret);
    }
    registerParams.uri = uriInput;
    if (!CheckIsValidUri(Uri(registerParams.uri))) {
        LOGE("RegisterChange uri parameter format error!");
        return E_PARAMS;
    }
    registerParams.recursion = recursion;
    ret = env->GlobalReference_Create(reinterpret_cast<ani_ref>(fun), &registerParams.cbOnRef);
    if (ret != ANI_OK) {
        LOGE("get register param create ref failed. ret = %{public}d", static_cast<int32_t>(ret));
        return static_cast<int32_t>(ret);
    }

    return E_OK;
}

void CloudSyncAni::RegisterChange(ani_env *env, ani_class clazz, ani_string uri, ani_boolean recursion, ani_object fun)
{
    if (g_listObj == nullptr) {
        g_listObj = make_unique<ChangeListenerAni>(env);
    }

    RegisterParams registerParams;
    int32_t ret = GetRegisterParams(env, uri, recursion, fun, registerParams);
    if (ret != ERR_OK) {
        LOGE("Get Params fail");
        ErrorHandler::Throw(env, E_PARAMS);
        return;
    }

    if (CheckRef(env, registerParams.cbOnRef, *g_listObj, registerParams.uri)) {
        ret = RegisterToObs(registerParams);
        if (ret != E_OK) {
            LOGE("Get Params fail");
            ErrorHandler::Throw(env, E_PARAMS);
            return;
        }
    } else {
        LOGE("Check Ref fail");
        ErrorHandler::Throw(env, E_PARAMS);
        env->GlobalReference_Delete(registerParams.cbOnRef);
        return;
    }
}

void CloudSyncAni::UnregisterFromObs(ani_env *env, std::string uri)
{
    if (!CheckIsValidUri(Uri(uri))) {
        LOGE("unavaiable uri.");
        ErrorHandler::Throw(env, E_PARAMS);
        return;
    }

    auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
    if (obsMgrClient == nullptr) {
        LOGE("get DataObsMgrClient failed");
        ErrorHandler::Throw(env, E_IPCSS);
        return;
    }
    std::vector<std::shared_ptr<CloudNotifyObserver>> offObservers;
    {
        std::lock_guard<mutex> lock(CloudSyncAni::sOnOffMutex_);
        for (auto iter = g_listObj->observers_.begin(); iter != g_listObj->observers_.end();) {
            if (uri == (*iter)->uri_) {
                offObservers.push_back(*iter);
                vector<shared_ptr<CloudNotifyObserver>>::iterator tmp = iter;
                iter = g_listObj->observers_.erase(tmp);
            } else {
                iter++;
            }
        }
    }
    for (auto observer : offObservers) {
        if (!ObserverImpl::FindObserver(Uri(uri), observer)) {
            LOGE("observer not exist");
            ErrorHandler::Throw(env, E_PARAMS);
            return;
        }
        sptr<ObserverImpl> obs = ObserverImpl::GetObserver(Uri(uri), observer);
        if (obs == nullptr) {
            LOGE("new observerimpl failed");
            ErrorHandler::Throw(env, E_PARAMS);
            return;
        }
        ErrCode ret = obsMgrClient->UnregisterObserverExt(Uri(uri), obs);
        if (ret != ERR_OK) {
            LOGE("call obs unregister fail");
            ErrorHandler::Throw(env, E_PARAMS);
            return;
        }
        ObserverImpl::DeleteObserver(Uri(uri), observer);
    }
}

void CloudSyncAni::UnRegisterChange(ani_env *env, ani_class clazz, ani_string uri)
{
    if (g_listObj == nullptr || g_listObj->observers_.empty()) {
        LOGI("no obs to unregister");
        return;
    }

    std::string uriInput;
    ani_status ret = ANIUtils::AniString2String(env, uri, uriInput);
    if (ret != ANI_OK) {
        ErrorHandler::Throw(env, E_PARAMS);
        return;
    }

    UnregisterFromObs(env, uriInput);
}
} // namespace OHOS::FileManagement::CloudSync