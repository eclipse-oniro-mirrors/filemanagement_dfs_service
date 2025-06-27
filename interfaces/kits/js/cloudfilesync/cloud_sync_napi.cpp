/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include <set>
#include <sys/types.h>
#include <sys/xattr.h>

#include "async_work.h"
#include "cloud_sync_manager.h"
#include "cloud_sync_napi.h"
#include "dfs_error.h"
#include "dfsu_access_token_helper.h"
#include "securec.h"
#include "uri.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {
using namespace FileManagement::LibN;
using namespace std;
const int32_t PARAM0 = 0;
static const unsigned int READ_SIZE = 1024;
// The data size is consistent with IPC
static const unsigned int MAX_CHANGE_DATA_SIZE = 200 * 1024;
const string FILE_SCHEME = "file";
thread_local unique_ptr<ChangeListenerNapi> g_listObj = nullptr;
mutex CloudSyncNapi::sOnOffMutex_;
static mutex obsMutex_;
const int32_t AGING_DAYS = 30;
const int32_t GET_FILE_SYNC_MAX = 100;
CloudSyncState CloudSyncCallbackImpl::preState_ = CloudSyncState::COMPLETED;
ErrorType CloudSyncCallbackImpl::preError_ = ErrorType::NO_ERROR;

class ObserverImpl : public AAFwk::DataAbilityObserverStub {
public:
    explicit ObserverImpl(const shared_ptr<CloudNotifyObserver> cloudNotifyObserver)
        : cloudNotifyObserver_(cloudNotifyObserver){};
    void OnChange();
    void OnChangeExt(const AAFwk::ChangeInfo &info);
    static sptr<ObserverImpl> GetObserver(const Uri &uri, const shared_ptr<CloudNotifyObserver> &observer);
    static bool FindObserver(const Uri &uri, const shared_ptr<CloudNotifyObserver> &observer);
    static bool DeleteObserver(const Uri &uri, const shared_ptr<CloudNotifyObserver> &observer);

private:
    struct ObserverParam {
        sptr<ObserverImpl> obs_;
        list<Uri> uris_;
    };
    shared_ptr<CloudNotifyObserver> cloudNotifyObserver_;
    static ConcurrentMap<CloudNotifyObserver *, ObserverParam> observers_;
};

ConcurrentMap<CloudNotifyObserver *, ObserverImpl::ObserverParam> ObserverImpl::observers_;

void ObserverImpl::OnChange() {}

void ObserverImpl::OnChangeExt(const AAFwk::ChangeInfo &info)
{
    if (cloudNotifyObserver_ == nullptr) {
        LOGE("cloudNotifyObserver_ is null!");
        return;
    }
    cloudNotifyObserver_->OnchangeExt(info);
}

sptr<ObserverImpl> ObserverImpl::GetObserver(const Uri &uri, const shared_ptr<CloudNotifyObserver> &observer)
{
    lock_guard<mutex> lock(obsMutex_);
    sptr<ObserverImpl> result = nullptr;
    observers_.Compute(observer.get(), [&result, &uri, &observer](const auto &key, auto &value) {
        if (value.obs_ == nullptr) {
            value.obs_ = new (nothrow) ObserverImpl(observer);
            value.uris_.push_back(uri);
        } else {
            auto it = find(value.uris_.begin(), value.uris_.end(), uri);
            if (it == value.uris_.end()) {
                value.uris_.push_back(uri);
            }
        }

        result = value.obs_;
        return result != nullptr;
    });

    return result;
}

bool ObserverImpl::FindObserver(const Uri &uri, const shared_ptr<CloudNotifyObserver> &observer)
{
    lock_guard<mutex> lock(obsMutex_);
    auto result = observers_.Find(observer.get());
    if (result.first) {
        auto it = std::find(result.second.uris_.begin(), result.second.uris_.end(), uri);
        if (it == result.second.uris_.end()) {
            return false;
        }
    }
    return result.first;
}

bool ObserverImpl::DeleteObserver(const Uri &uri, const shared_ptr<CloudNotifyObserver> &observer)
{
    lock_guard<mutex> lock(obsMutex_);
    return observers_.ComputeIfPresent(observer.get(), [&uri](auto &key, auto &value) {
        value.uris_.remove_if([&uri](const auto &value) { return uri == value; });
        return !value.uris_.empty();
    });
}

CloudSyncCallbackImpl::CloudSyncCallbackImpl(napi_env env, napi_value fun) : env_(env)
{
    if (fun != nullptr) {
        napi_create_reference(env_, fun, 1, &cbOnRef_);
    }
}

void CloudSyncCallbackImpl::OnComplete(UvChangeMsg *msg)
{
    auto cloudSyncCallback = msg->cloudSyncCallback_.lock();
    if (cloudSyncCallback == nullptr || cloudSyncCallback->cbOnRef_ == nullptr) {
        LOGE("cloudSyncCallback->cbOnRef_ is nullptr");
        return;
    }
    auto env = cloudSyncCallback->env_;
    auto ref = cloudSyncCallback->cbOnRef_;
    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env, &scope);
    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env, ref, &jsCallback);
    if (status != napi_ok) {
        LOGE("Create reference failed, status: %{public}d", status);
        napi_close_handle_scope(env, scope);
        return;
    }
    NVal obj = NVal::CreateObject(env);
    obj.AddProp("state", NVal::CreateInt32(env, (int32_t)msg->state_).val_);
    obj.AddProp("error", NVal::CreateInt32(env, (int32_t)msg->error_).val_);
    napi_value retVal = nullptr;
    napi_value global = nullptr;
    napi_get_global(env, &global);
    status = napi_call_function(env, global, jsCallback, ARGS_ONE, &(obj.val_), &retVal);
    if (status != napi_ok) {
        LOGE("napi call function failed, status: %{public}d", status);
    }
    napi_close_handle_scope(env, scope);
}
void CloudSyncCallbackImpl::OnDeathRecipient()
{
    auto isInDownOrUpload = (preState_ == CloudSyncState::UPLOADING) || (preState_ == CloudSyncState::DOWNLOADING);
    if (isInDownOrUpload && (preError_ == ErrorType::NO_ERROR)) {
        OnSyncStateChanged(CloudSyncState::STOPPED, ErrorType::NO_ERROR);
    }
}

void CloudSyncCallbackImpl::OnSyncStateChanged(CloudSyncState state, ErrorType error)
{
    LOGI("notify - state: %{public}d, error: %{public}d", state, error);
    UvChangeMsg *msg = new (std::nothrow) UvChangeMsg(shared_from_this(), state, error);
    if (msg == nullptr) {
        LOGE("Failed to create uv message object");
        return;
    }

    auto task = [msg]() {
        if (msg->cloudSyncCallback_.expired()) {
            LOGE("cloudSyncCallback_ is expired");
            delete msg;
            return;
        }
        msg->cloudSyncCallback_.lock()->OnComplete(msg);
        delete msg;
    };
    preState_ = state;
    preError_ = error;
    napi_status ret = napi_send_event(env_, task, napi_event_priority::napi_eprio_immediate);
    if (ret != napi_ok) {
        LOGE("Failed to execute libuv work queue, ret: %{public}d", ret);
        delete msg;
        return;
    }
}

void CloudSyncCallbackImpl::DeleteReference()
{
    if (cbOnRef_ != nullptr) {
        napi_delete_reference(env_, cbOnRef_);
        cbOnRef_ = nullptr;
    }
}

void CloudSyncCallbackImpl::OnSyncStateChanged(SyncType type, SyncPromptState state)
{
    return;
}

void CloudOptimizeCallbackImpl::OnOptimizeProcess(const OptimizeState state, const int32_t progress)
{
    napi_env env = env_;
    auto task = [this, env, state, progress] () {
        if (!cbOnRef_) {
            LOGE("cbOnRef_ is nullptr");
            return;
        }

        napi_handle_scope scope = nullptr;
        napi_status status = napi_open_handle_scope(env, &scope);
        if (status != napi_ok) {
            LOGE("Create reference failed, status: %{public}d", status);
            return;
        }

        napi_value jsCallback = cbOnRef_.Deref(env).val_;
        NVal obj = NVal::CreateObject(env);
        obj.AddProp("state", NVal::CreateInt32(env, (int32_t)state).val_);
        obj.AddProp("progress", NVal::CreateInt32(env, (int32_t)progress).val_);
        napi_value retVal = nullptr;
        status = napi_call_function(env_, nullptr, jsCallback, ARGS_ONE, &(obj.val_), &retVal);
        if (status != napi_ok) {
            LOGE("napi call function failed, status: %{public}d", status);
        }
        napi_close_handle_scope(env, scope);
    };
    auto ret = napi_send_event(env, task, napi_eprio_immediate);
    if  (ret != 0) {
        LOGE("failed to send event, ret: %{public}d", ret);
    }
}

string CloudSyncNapi::GetBundleName(const napi_env &env, const NFuncArg &funcArg)
{
    string bundleName;
    auto bundleEntity = NClass::GetEntityOf<BundleEntity>(env, funcArg.GetThisVar());
    if (bundleEntity) {
        bundleName = bundleEntity->bundleName_;
    }
    return bundleName;
}

napi_value CloudSyncNapi::Constructor(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::ZERO, NARG_CNT::ONE)) {
        NError(E_PARAMS).ThrowErr(env, "Number of arguments unmatched");
        return nullptr;
    }
    if (funcArg.GetArgc() == NARG_CNT::ZERO) {
        LOGD("init without bundleName");
        return funcArg.GetThisVar();
    }
    if (funcArg.GetArgc() == NARG_CNT::ONE) {
        auto [succ, bundleName, ignore] = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
        if (!succ || bundleName.get() == string("")) {
            LOGE("Failed to get bundle name");
            NError(E_PARAMS).ThrowErr(env);
            return nullptr;
        }
        auto bundleEntity = make_unique<BundleEntity>(bundleName.get());
        if (!NClass::SetEntityFor<BundleEntity>(env, funcArg.GetThisVar(), move(bundleEntity))) {
            LOGE("Failed to set file entity");
            NError(EIO).ThrowErr(env);
            return nullptr;
        }
        LOGI("init with bundleName");
        return funcArg.GetThisVar();
    }
    return nullptr;
}

bool CloudSyncNapi::InitArgsOnCallback(const napi_env &env, NFuncArg &funcArg)
{
    if (!funcArg.InitArgs(NARG_CNT::TWO)) {
        NError(E_PARAMS).ThrowErr(env, "Number of arguments unmatched");
        LOGE("OnCallback Number of arguments unmatched");
        return false;
    }

    auto [succ, type, ignore] = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!(succ && (type.get() == std::string("progress")))) {
        NError(E_PARAMS).ThrowErr(env);
        return false;
    }

    if (!NVal(env, funcArg[(int)NARG_POS::SECOND]).TypeIs(napi_function)) {
        LOGE("Argument type mismatch");
        NError(E_PARAMS).ThrowErr(env);
        return false;
    }

    if (callback_ != nullptr) {
        LOGI("callback already exist");
        return false;
    }
    return true;
}

napi_value CloudSyncNapi::OnCallback(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!InitArgsOnCallback(env, funcArg)) {
        return nullptr;
    }

    string bundleName = GetBundleName(env, funcArg);
    callback_ = make_shared<CloudSyncCallbackImpl>(env, NVal(env, funcArg[(int)NARG_POS::SECOND]).val_);
    int32_t ret = CloudSyncManager::GetInstance().RegisterCallback(callback_, bundleName);
    if (ret != E_OK) {
        LOGE("OnCallback Register error, result: %{public}d", ret);
        NError(Convert2JsErrNum(ret)).ThrowErr(env);
        return nullptr;
    }

    return NVal::CreateUndefined(env).val_;
}

bool CloudSyncNapi::InitArgsOffCallback(const napi_env &env, NFuncArg &funcArg)
{
    if (!funcArg.InitArgs(NARG_CNT::ONE, NARG_CNT::TWO)) {
        NError(E_PARAMS).ThrowErr(env, "Number of arguments unmatched");
        LOGE("OffCallback Number of arguments unmatched");
        return false;
    }

    auto [succ, type, ignore] = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!(succ && (type.get() == std::string("progress")))) {
        NError(E_PARAMS).ThrowErr(env);
        return false;
    }

    if (funcArg.GetArgc() == (uint)NARG_CNT::TWO && !NVal(env, funcArg[(int)NARG_POS::SECOND]).TypeIs(napi_function)) {
        LOGE("Argument type mismatch");
        NError(E_PARAMS).ThrowErr(env);
        return false;
    }
    return true;
}

napi_value CloudSyncNapi::OffCallback(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!InitArgsOffCallback(env, funcArg)) {
        return nullptr;
    }

    string bundleName = GetBundleName(env, funcArg);
    int32_t ret = CloudSyncManager::GetInstance().UnRegisterCallback(bundleName);
    if (ret != E_OK) {
        LOGE("OffCallback UnRegister error, result: %{public}d", ret);
        NError(Convert2JsErrNum(ret)).ThrowErr(env);
        return nullptr;
    }
    if (callback_ != nullptr) {
        /* napi delete reference */
        callback_->DeleteReference();
        callback_ = nullptr;
    }
    return NVal::CreateUndefined(env).val_;
}

napi_value CloudSyncNapi::Start(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::ZERO, NARG_CNT::ONE)) {
        LOGE("Failed to init args");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    string bundleName = GetBundleName(env, funcArg);
    auto cbExec = [bundleName]() -> NError {
        int32_t ret = CloudSyncManager::GetInstance().StartSync(bundleName);
        if (ret != E_OK) {
            LOGE("Start Sync error, result: %{public}d", ret);
            return NError(Convert2JsErrNum(ret));
        }
        return NError(ERRNO_NOERR);
    };

    auto cbComplete = [](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return NVal::CreateUndefined(env);
    };

    std::string procedureName = "Start";
    auto asyncWork = GetPromiseOrCallBackWork(env, funcArg, static_cast<size_t>(NARG_CNT::TWO));
    return asyncWork == nullptr ? nullptr : asyncWork->Schedule(procedureName, cbExec, cbComplete).val_;
}

napi_value CloudSyncNapi::Stop(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::ZERO, NARG_CNT::ONE)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    string bundleName = GetBundleName(env, funcArg);
    auto cbExec = [bundleName]() -> NError {
        int32_t ret = CloudSyncManager::GetInstance().StopSync(bundleName);
        if (ret != E_OK) {
            LOGE("Stop Sync error, result: %{public}d", ret);
            return NError(Convert2JsErrNum(ret));
        }
        return NError(ERRNO_NOERR);
    };

    auto cbComplete = [](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return NVal::CreateUndefined(env);
    };

    std::string procedureName = "Stop";
    auto asyncWork = GetPromiseOrCallBackWork(env, funcArg, static_cast<size_t>(NARG_CNT::TWO));
    return asyncWork == nullptr ? nullptr : asyncWork->Schedule(procedureName, cbExec, cbComplete).val_;
}

bool CloudSyncNapi::CheckRef(napi_env env, napi_ref ref, ChangeListenerNapi &listObj, const string &uri)
{
    napi_value offCallback = nullptr;
    napi_status status = napi_get_reference_value(env, ref, &offCallback);
    if (status != napi_ok) {
        LOGE("Create reference fail, status: %{public}d", status);
        return false;
    }
    bool isSame = false;
    shared_ptr<CloudNotifyObserver> obs;
    string obsUri;
    {
        lock_guard<mutex> lock(sOnOffMutex_);
        for (auto it = listObj.observers_.begin(); it < listObj.observers_.end(); it++) {
            napi_value onCallback = nullptr;
            status = napi_get_reference_value(env, (*it)->ref_, &onCallback);
            if (status != napi_ok) {
                LOGE("Create reference fail, status: %{public}d", status);
                return false;
            }
            napi_strict_equals(env, offCallback, onCallback, &isSame);
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

static bool CheckIsValidUri(Uri uri)
{
    string scheme = uri.GetScheme();
    if (scheme != FILE_SCHEME) {
        return false;
    }
    string sandboxPath = uri.GetPath();
    char realPath[PATH_MAX + 1]{'\0'};
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

static int32_t GetRegisterParams(napi_env env, napi_callback_info info, RegisterParams &registerParams)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::THREE)) {
        LOGE("Arguments number mismatch");
        return E_PARAMS;
    }

    auto [succUri, uri, ignore] = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succUri) {
        LOGE("get arg uri fail");
        return E_PARAMS;
    }
    registerParams.uri = string(uri.get());
    if (!CheckIsValidUri(Uri(registerParams.uri))) {
        LOGE("RegisterChange uri parameter format error!");
        return E_PARAMS;
    }

    auto [succRecursion, recursion] = NVal(env, funcArg[(int)NARG_POS::SECOND]).ToBool();
    if (!succRecursion) {
        LOGE("get arg recursion fail");
        return E_PARAMS;
    }
    registerParams.recursion = recursion;

    if (!NVal(env, funcArg[(int)NARG_POS::THIRD]).TypeIs(napi_function)) {
        LOGE("Argument type mismatch");
        return E_PARAMS;
    }
    napi_status status =
        napi_create_reference(env, NVal(env, funcArg[(int)NARG_POS::THIRD]).val_, 1, &registerParams.cbOnRef);
    if (status != napi_ok) {
        LOGE("Create reference fail, status: %{public}d", status);
        return E_PARAMS;
    }

    return ERR_OK;
}

int32_t CloudSyncNapi::RegisterToObs(napi_env env, const RegisterParams &registerParams)
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
    lock_guard<mutex> lock(CloudSyncNapi::sOnOffMutex_);
    g_listObj->observers_.push_back(observer);
    return E_OK;
}

napi_value CloudSyncNapi::RegisterChange(napi_env env, napi_callback_info info)
{
    if (g_listObj == nullptr) {
        g_listObj = make_unique<ChangeListenerNapi>(env);
    }

    RegisterParams registerParams;
    int32_t ret = GetRegisterParams(env, info, registerParams);
    if (ret != ERR_OK) {
        LOGE("Get Params fail");
        NError(ret).ThrowErr(env);
        return nullptr;
    }

    if (CheckRef(env, registerParams.cbOnRef, *g_listObj, registerParams.uri)) {
        ret = RegisterToObs(env, registerParams);
        if (ret != E_OK) {
            LOGE("Get Params fail");
            NError(ret).ThrowErr(env);
            return nullptr;
        }
    } else {
        LOGE("Check Ref fail");
        NError(E_PARAMS).ThrowErr(env);
        napi_delete_reference(env, registerParams.cbOnRef);
        return nullptr;
    }
    return NVal::CreateUndefined(env).val_;
}

napi_value CloudSyncNapi::UnregisterFromObs(napi_env env, const string &uri)
{
    auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
    if (obsMgrClient == nullptr) {
        LOGE("get DataObsMgrClient failed");
        NError(E_SA_LOAD_FAILED).ThrowErr(env);
        return nullptr;
    }
    std::vector<std::shared_ptr<CloudNotifyObserver>> offObservers;
    {
        lock_guard<mutex> lock(sOnOffMutex_);
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
            NError(E_PARAMS).ThrowErr(env);
            return nullptr;
        }
        sptr<ObserverImpl> obs = ObserverImpl::GetObserver(Uri(uri), observer);
        if (obs == nullptr) {
            LOGE("new observerimpl failed");
            NError(E_PARAMS).ThrowErr(env);
            return nullptr;
        }
        ErrCode ret = obsMgrClient->UnregisterObserverExt(Uri(uri), obs);
        if (ret != ERR_OK) {
            LOGE("call obs unregister fail");
            NError(E_PARAMS).ThrowErr(env);
            return nullptr;
        }
        ObserverImpl::DeleteObserver(Uri(uri), observer);
    }
    return NVal::CreateUndefined(env).val_;
}

napi_value CloudSyncNapi::UnregisterChange(napi_env env, napi_callback_info info)
{
    if (g_listObj == nullptr || g_listObj->observers_.empty()) {
        LOGI("no obs to unregister");
        return nullptr;
    }

    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::ONE)) {
        LOGE("params number mismatch");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    auto [succUri, uri, ignore] = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succUri || !CheckIsValidUri(Uri(uri.get()))) {
        LOGE("get uri fail");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    return UnregisterFromObs(env, uri.get());
}

void CloudSyncNapi::SetClassName(const std::string classname)
{
    className_ = classname;
}

std::string CloudSyncNapi::GetClassName()
{
    return className_;
}

bool CloudSyncNapi::ToExport(std::vector<napi_property_descriptor> props)
{
    std::string className = GetClassName();
    auto [succ, classValue] =
        NClass::DefineClass(exports_.env_, className, Constructor, std::move(props));
    if (!succ) {
        NError(E_GETRESULT).ThrowErr(exports_.env_);
        LOGE("Failed to define CloudSyncNapi class");
        return false;
    }

    succ = NClass::SaveClass(exports_.env_, className, classValue);
    if (!succ) {
        NError(E_GETRESULT).ThrowErr(exports_.env_);
        LOGE("Failed to save CloudSyncNapi class");
        return false;
    }

    return exports_.AddProp(className, classValue);
}

bool CloudSyncNapi::Export()
{
    std::vector<napi_property_descriptor> props = {
        NVal::DeclareNapiFunction("on", OnCallback),
        NVal::DeclareNapiFunction("off", OffCallback),
        NVal::DeclareNapiFunction("start", Start),
        NVal::DeclareNapiFunction("stop", Stop),
        NVal::DeclareNapiFunction("getFileSyncState", GetFileSyncState),
        NVal::DeclareNapiFunction("optimizeStorage", OptimizeStorage),
        NVal::DeclareNapiFunction("startOptimizeSpace", StartOptimizeStorage),
        NVal::DeclareNapiFunction("stopOptimizeSpace", StopOptimizeStorage),
    };
    std::string className = GetClassName();
    auto [succ, classValue] =
        NClass::DefineClass(exports_.env_, className, CloudSyncNapi::Constructor, std::move(props));
    if (!succ) {
        NError(E_GETRESULT).ThrowErr(exports_.env_);
        LOGE("Failed to define GallerySync class");
        return false;
    }

    succ = NClass::SaveClass(exports_.env_, className, classValue);
    if (!succ) {
        NError(E_GETRESULT).ThrowErr(exports_.env_);
        LOGE("Failed to save GallerySync class");
        return false;
    }

    return exports_.AddProp(className, classValue);
}

int32_t CloudSyncNapi::HandOptimizeStorageParams(napi_env env, napi_callback_info info, NFuncArg &funcArg,
    OptimizeSpaceOptions &optimizeOptions)
{
    bool succ = false;
    auto argv = NVal(env, funcArg[NARG_POS::FIRST]);
    if (!argv.HasProp("totalSize") || !argv.HasProp("agingDays")) {
        return ERR_BAD_VALUE;
    }

    int64_t totalSize = 0;
    int32_t agingDays = 0;
    tie(succ, totalSize) = argv.GetProp("totalSize").ToInt64();
    if (!succ) {
        return ERR_BAD_VALUE;
    }
    tie(succ, agingDays) = argv.GetProp("agingDays").ToInt32();
    if (!succ) {
        return ERR_BAD_VALUE;
    }

    if (!NVal(env, funcArg[(int)NARG_POS::SECOND]).TypeIs(napi_function)) {
        LOGE("Argument type mismatch");
        return ERR_BAD_VALUE;
    }

    LOGI("totalSize:%{public}lld, agingDays:%{public}d", static_cast<long long>(totalSize), agingDays);
    optimizeOptions.totalSize = totalSize;
    optimizeOptions.agingDays = agingDays;
    return ERR_OK;
}

napi_value CloudSyncNapi::StartOptimizeStorage(napi_env env, napi_callback_info info)
{
    LOGI("StartOptimizeStorage enter");
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::TWO)) {
        NError(E_PARAMS).ThrowErr(env, "Number of arguments unmatched");
        LOGE("Number of arguments unmatched");
        return nullptr;
    }
    OptimizeSpaceOptions optimizeOptions {};
    int32_t res = HandOptimizeStorageParams(env, info, funcArg, optimizeOptions);
    if (res != ERR_OK) {
        LOGE("hand paeams failed");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    auto callback = make_shared<CloudOptimizeCallbackImpl>(env, NVal(env, funcArg[(int)NARG_POS::SECOND]));
    auto cbExec = [optimizeOptions, callback]() -> NError {
        int32_t ret = CloudSyncManager::GetInstance().OptimizeStorage(optimizeOptions, callback);
        if (ret != E_OK) {
            LOGE("StartOptimizeStorage error, result: %{public}d", ret);
            return NError(Convert2JsErrNum(ret));
        }
        return NError(ERRNO_NOERR);
    };

    auto cbComplete = [](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return NVal::CreateUndefined(env);
    };

    std::string procedureName = "StartOptimizeStorage";
    auto asyncWork = GetPromiseOrCallBackWork(env, funcArg, static_cast<size_t>(NARG_CNT::THREE));
    return asyncWork == nullptr ? nullptr : asyncWork->Schedule(procedureName, cbExec, cbComplete).val_;
}

napi_value CloudSyncNapi::StopOptimizeStorage(napi_env env, napi_callback_info info)
{
    LOGI("StopOptimizeStorage enter");
    int32_t ret = CloudSyncManager::GetInstance().StopOptimizeStorage();
    if (ret != E_OK) {
        LOGE("StopOptimizeStorage error, result: %{public}d", ret);
        NError(Convert2JsErrNum(ret)).ThrowErr(env);
        return nullptr;
    }

    return NVal::CreateUndefined(env).val_;
}

napi_value CloudSyncNapi::OptimizeStorage(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::ZERO)) {
        LOGE("Failed to init args");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    OptimizeSpaceOptions optimizeOptions {};
    optimizeOptions.totalSize = 0;
    optimizeOptions.agingDays = AGING_DAYS;

    auto cbExec = [optimizeOptions]() -> NError {
        int32_t ret = CloudSyncManager::GetInstance().OptimizeStorage(optimizeOptions);
        if (ret != E_OK) {
            LOGE("OptimizeStorage error, result: %{public}d", ret);
            return NError(Convert2JsErrNum(ret));
        }
        return NError(ERRNO_NOERR);
    };

    auto cbComplete = [](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return NVal::CreateUndefined(env);
    };

    std::string procedureName = "OptimizeStorage";
    auto asyncWork = GetPromiseOrCallBackWork(env, funcArg, static_cast<size_t>(NARG_CNT::TWO));
    return asyncWork == nullptr ? nullptr : asyncWork->Schedule(procedureName, cbExec, cbComplete).val_;
}

static bool IsGetSingleFileStatus(const napi_env &env, const NFuncArg &funcArg)
{
    if (!NVal(env, funcArg[(int)NARG_POS::FIRST]).TypeIs(napi_string)) {
        return false;
    }
    return true;
}

static tuple<int32_t, int32_t> GetxattrErr()
{
    LOGE("getxattr failed, errno : %{public}d", errno);
    std::set<int32_t> errForSingleFileSync = { ENOENT, EACCES, EAGAIN, EINTR, ENOSYS };
    if (errForSingleFileSync.find(errno) != errForSingleFileSync.end()) {
        return { errno, -1 };
    }
    return { E_UNKNOWN_ERR, -1 };
}

tuple<int32_t, int32_t> CloudSyncNapi::GetSingleFileSyncState(const string &uri)
{
    Uri fileUri(uri);
    char resolvedPath[PATH_MAX] = {'\0'};

    if (realpath(fileUri.GetPath().c_str(), resolvedPath) == nullptr) {
        LOGE("get realPath failed");
        return { LibN::USER_FILE_MANAGER_SYS_CAP_TAG + E_URIM, -1 };
    }

    std::string sandBoxPath(resolvedPath);
    std::string xattrKey = "user.cloud.filestatus";
    auto xattrValueSize = getxattr(sandBoxPath.c_str(), xattrKey.c_str(), nullptr, 0);
    if (xattrValueSize < 0) {
        return GetxattrErr();
    }
    std::unique_ptr<char[]> xattrValue = std::make_unique<char[]>((long)xattrValueSize + 1);
    if (xattrValue == nullptr) {
        LOGE("Failed to allocate memory for xattrValue, errno : %{public}d", errno);
        return { E_UNKNOWN_ERR, -1 };
    }
    xattrValueSize = getxattr(sandBoxPath.c_str(), xattrKey.c_str(), xattrValue.get(), xattrValueSize);
    if (xattrValueSize <= 0) {
        return GetxattrErr();
    }

    std::string xattrValueStr(xattrValue.get(), xattrValueSize);
    bool isValid = std::all_of(xattrValueStr.begin(), xattrValueStr.end(), ::isdigit);
    if (!isValid) {
        LOGE("invalid xattrValue");
        return { E_PARAMS, -1};
    }

    int32_t fileStatus = std::stoi(xattrValue.get());
    int32_t val;
    if (fileStatus == FileSync::FILESYNC_TO_BE_UPLOADED || fileStatus == FileSync::FILESYNC_UPLOADING ||
        fileStatus == FileSync::FILESYNC_UPLOAD_FAILURE || fileStatus == FileSync::FILESYNC_UPLOAD_SUCCESS) {
        val = statusMap[fileStatus];
    } else {
        val = FileSyncState::FILESYNCSTATE_COMPLETED;
    }
    return { E_OK, val };
}

tuple<int32_t, int32_t> CloudSyncNapi::GetFileSyncStateForBatch(const string &uri)
{
    Uri fileUri(uri);
    char resolvedPath[PATH_MAX] = {'\0'};

    if (realpath(fileUri.GetPath().c_str(), resolvedPath) == nullptr) {
        LOGE("get realPath failed");
        return { LibN::USER_FILE_MANAGER_SYS_CAP_TAG + E_URIM, -1 };
    }

    std::string sandBoxPath(resolvedPath);
    std::string xattrKey = "user.cloud.filestatus";
    auto xattrValueSize = getxattr(sandBoxPath.c_str(), xattrKey.c_str(), nullptr, 0);
    if (xattrValueSize < 0) {
        LOGE("getxattr failed, errno : %{public}d", errno);
        return { LibN::STORAGE_SERVICE_SYS_CAP_TAG + LibN::E_IPCSS, -1 };
    }
    std::unique_ptr<char[]> xattrValue = std::make_unique<char[]>((long)xattrValueSize + 1);
    if (xattrValue == nullptr) {
        LOGE("Failed to allocate memory for xattrValue, errno : %{public}d", errno);
        return { LibN::STORAGE_SERVICE_SYS_CAP_TAG + LibN::E_IPCSS, -1 };
    }
    xattrValueSize = getxattr(sandBoxPath.c_str(), xattrKey.c_str(), xattrValue.get(), xattrValueSize);
    if (xattrValueSize <= 0) {
        LOGE("getxattr failed, errno : %{public}d", errno);
        return { LibN::STORAGE_SERVICE_SYS_CAP_TAG + LibN::E_IPCSS, -1 };
    }

    std::string xattrValueStr(xattrValue.get(), xattrValueSize);
    bool isValid = std::all_of(xattrValueStr.begin(), xattrValueStr.end(), ::isdigit);
    if (!isValid) {
        LOGE("invalid xattrValue");
        return { E_PARAMS, -1};
    }

    int32_t fileStatus = std::stoi(xattrValue.get());
    int32_t val;
    if (fileStatus == FileSync::FILESYNC_TO_BE_UPLOADED || fileStatus == FileSync::FILESYNC_UPLOADING ||
        fileStatus == FileSync::FILESYNC_UPLOAD_FAILURE || fileStatus == FileSync::FILESYNC_UPLOAD_SUCCESS) {
        val = statusMap[fileStatus];
    } else {
        val = FileSyncState::FILESYNCSTATE_COMPLETED;
    }
    return { E_OK, val };
}

static NVal AsyncComplete(const napi_env &env, std::shared_ptr<BatchContext> ctx)
{
    napi_value results = nullptr;

    napi_status status = napi_create_array(env, &results);
    if (status != napi_ok) {
        LOGE("Failed to create array");
        return { env, NError(LibN::STORAGE_SERVICE_SYS_CAP_TAG + LibN::E_IPCSS).GetNapiErr(env) };
    }

    int32_t index = 0;
    for (auto result : ctx->resultList) {
        status = napi_set_element(env, results, index, NVal::CreateInt32(env, result).val_);
        if (status != napi_ok) {
            LOGE("Failed to set element on data");
            return { env, NError(LibN::STORAGE_SERVICE_SYS_CAP_TAG + LibN::E_IPCSS).GetNapiErr(env) };
        }
        index++;
    }
    return { env, results };
}

napi_value CloudSyncNapi::GetBatchFileSyncState(const napi_env &env, const NFuncArg &funcArg)
{
    NVal arrayVal(env, funcArg[static_cast<int>(NARG_POS::FIRST)]);
    if (!arrayVal.TypeIs(napi_object)) {
        LOGE("Invalid argments");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    auto [succ, uris, ignore] = arrayVal.ToStringArray();
    auto ctx = std::make_shared<BatchContext>();
    ctx->uriList.swap(uris);
    if (ctx->uriList.size() > GET_FILE_SYNC_MAX) {
        LOGE("The parameter is too long");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    auto cbExec = [env, ctx]() ->NError {
        int32_t result = 0;
        int32_t fileStatus = 0;
        for (auto &uri : ctx->uriList) {
            tie(result, fileStatus) = GetFileSyncStateForBatch(uri);
            if (result != E_OK) {
                return NError(result);
            }
            ctx->resultList.push_back(fileStatus);
        }
        return NError(NO_ERROR);
    };

    auto cbComplete = [ctx](napi_env env, NError err) -> NVal {
        if (err) {
            return { env, err.GetNapiErr(env) };
        }
        return AsyncComplete(env, ctx);
    };

    std::string procedureName = "GetFileSyncState";
    auto asyncWork = GetPromiseOrCallBackWork(env, funcArg, static_cast<size_t>(NARG_CNT::TWO));
    return asyncWork == nullptr ? nullptr : asyncWork->Schedule(procedureName, cbExec, cbComplete).val_;
}

napi_value CloudSyncNapi::GetFileSyncState(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    int32_t result = 0;
    int32_t fileStatus = 0;

    if (!funcArg.InitArgs(NARG_CNT::ONE, NARG_CNT::TWO)) {
        LOGE("Number of arguments unmatched");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    if (IsGetSingleFileStatus(env, funcArg)) {
        std::unique_ptr<char []> uri;
        bool succ = false;
        tie(succ, uri, std::ignore) = NVal(env, funcArg[static_cast<int>(NARG_POS::FIRST)]).ToUTF8String();
        if (!succ) {
            LOGE("Invalid uri");
            NError(E_PARAMS).ThrowErr(env);
            return nullptr;
        }
        tie(result, fileStatus) = GetSingleFileSyncState(string(uri.get()));
        if (result != E_OK) {
            NError(result).ThrowErr(env);
            return nullptr;
        }
        return NVal::CreateInt32(env, fileStatus).val_;
    }
    return GetBatchFileSyncState(env, funcArg);
}

void ChangeListenerNapi::OnChange(CloudChangeListener &listener, const napi_ref cbRef)
{
    if (static_cast<NotifyType>(listener.changeInfo.changeType_) == NotifyType::NOTIFY_NONE ||
        listener.changeInfo.uris_.empty() || listener.changeInfo.size_ > MAX_CHANGE_DATA_SIZE) {
        LOGD("notify data is invalid: %{public}d %{public}zu %{public}d",
             static_cast<int>(listener.changeInfo.changeType_), listener.changeInfo.uris_.size(),
             listener.changeInfo.size_);
        return;
    }

    UvChangeMsg *msg = new (std::nothrow) UvChangeMsg(env_, cbRef, listener.changeInfo, listener.strUri);
    if (msg == nullptr) {
        LOGE("Failed to create uv message object");
        return;
    }
    msg->data_ = (uint8_t *)malloc(msg->changeInfo_.size_);
    if (msg->data_ == nullptr) {
        LOGE("new msg->data failed");
        delete msg;
        return;
    }
    int copyRet = memcpy_s(msg->data_, msg->changeInfo_.size_, msg->changeInfo_.data_, msg->changeInfo_.size_);
    if (copyRet != 0) {
        LOGE("Parcel data copy failed, err = %{public}d", copyRet);
        free(msg->data_);
        delete msg;
        return;
    }

    auto ret = SendEvent(msg);
    if (ret != napi_ok) {
        LOGE("Failed to execute libuv work queue, ret: %{public}d", ret);
        free(msg->data_);
        delete msg;
        return;
    }
}

int32_t ChangeListenerNapi::SendEvent(UvChangeMsg *msg)
{
    auto task = [msg]() {
        // js thread
        napi_env env = msg->env_;
        napi_handle_scope scope = nullptr;
        if (napi_open_handle_scope(env, &scope) != napi_ok) {
            LOGE("Failed to open handle scope");
            return;
        }
        do {
            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(env, msg->ref_, &jsCallback);
            if (status != napi_ok) {
                LOGE("Create reference fail, status: %{public}d", status);
                break;
            }

            napi_value retVal = nullptr;
            napi_value result[ARGS_ONE];
            result[PARAM0] = ChangeListenerNapi::SolveOnChange(env, msg);
            if (result[PARAM0] == nullptr) {
                break;
            }

            status = napi_call_function(env, nullptr, jsCallback, ARGS_ONE, result, &retVal);
            if (status != napi_ok) {
                LOGE("CallJs napi_call_function fail, status: %{public}d", status);
            }
        } while (0);
        napi_close_handle_scope(env, scope);
        free(msg->data_);
        delete msg;
    };
    return napi_send_event(env_, task, napi_event_priority::napi_eprio_immediate);
}

static napi_status SetValueArray(const napi_env &env, const char *fieldStr, const std::list<Uri> listValue,
    napi_value &result)
{
    napi_value value = nullptr;
    napi_status status = napi_create_array_with_length(env, listValue.size(), &value);
    if (status != napi_ok) {
        LOGE("Create array error! field: %{public}s", fieldStr);
        return status;
    }
    int elementIndex = 0;
    for (auto uri : listValue) {
        napi_value uriRet = nullptr;
        napi_create_string_utf8(env, uri.ToString().c_str(), NAPI_AUTO_LENGTH, &uriRet);
        status = napi_set_element(env, value, elementIndex++, uriRet);
        if (status != napi_ok) {
            LOGE("Set lite item failed, error: %d", status);
        }
    }
    status = napi_set_named_property(env, result, fieldStr, value);
    if (status != napi_ok) {
        LOGE("Set array named property error! field: %{public}s", fieldStr);
    }

    return status;
}

static napi_status SetValueInt32(const napi_env &env, const char *fieldStr, const int intValue, napi_value &result)
{
    napi_value value;
    napi_status status = napi_create_int32(env, intValue, &value);
    if (status != napi_ok) {
        LOGE("Set value create int32 error! field: %{public}s", fieldStr);
        return status;
    }
    status = napi_set_named_property(env, result, fieldStr, value);
    if (status != napi_ok) {
        LOGE("Set int32 named property error! field: %{public}s", fieldStr);
    }
    return status;
}

static napi_status SetIsDir(const napi_env &env, const shared_ptr<MessageParcel> parcel, napi_value &result)
{
    uint32_t len = 0;
    napi_status status = napi_invalid_arg;
    if (!parcel->ReadUint32(len)) {
        LOGE("Failed to read sub uri list length");
        return status;
    }
    napi_value isDirArray = nullptr;
    napi_create_array_with_length(env, len, &isDirArray);
    int elementIndex = 0;
    if (len > READ_SIZE) {
        return napi_invalid_arg;
    }
    for (uint32_t i = 0; i < len; i++) {
        bool isDir = parcel->ReadBool();
        napi_value isDirRet = nullptr;
        napi_get_boolean(env, isDir, &isDirRet);
        napi_set_element(env, isDirArray, elementIndex++, isDirRet);
    }
    status = napi_set_named_property(env, result, "isDirectory", isDirArray);
    if (status != napi_ok) {
        LOGE("Set subUri named property error!");
    }
    return status;
}

napi_value ChangeListenerNapi::SolveOnChange(napi_env env, UvChangeMsg *msg)
{
    static napi_value result;
    if (msg->changeInfo_.uris_.empty()) {
        napi_get_undefined(env, &result);
        return result;
    }
    napi_create_object(env, &result);
    SetValueArray(env, "uris", msg->changeInfo_.uris_, result);
    SetValueInt32(env, "type", (int)msg->changeInfo_.changeType_, result);

    if (msg->changeInfo_.size_ > MAX_CHANGE_DATA_SIZE || msg->data_ == nullptr) {
        LOGE("change info is invalid");
        return nullptr;
    }
    uint8_t *parcelData = (uint8_t *)malloc(msg->changeInfo_.size_);
    if (parcelData == nullptr) {
        LOGE("new parcelData failed");
        return nullptr;
    }
    int copyRet = memcpy_s(parcelData, msg->changeInfo_.size_, msg->data_, msg->changeInfo_.size_);
    if (copyRet != 0) {
        LOGE("Parcel data copy failed, err = %{public}d", copyRet);
        free(parcelData);
        return nullptr;
    }

    shared_ptr<MessageParcel> parcel = make_shared<MessageParcel>();
    if (!parcel->ParseFrom(reinterpret_cast<uintptr_t>(parcelData), msg->changeInfo_.size_)) {
        LOGE("parcel parse failed");
        free(parcelData);
        return nullptr;
    }
    napi_status status = SetIsDir(env, parcel, result);
    if (status != napi_ok) {
        LOGE("Set subArray named property error! field: subUris");
        return nullptr;
    }

    return result;
}

void CloudNotifyObserver::OnChange() {}

void CloudNotifyObserver::OnchangeExt(const AAFwk::ChangeInfo &changeInfo)
{
    CloudChangeListener listener;
    listener.changeInfo = changeInfo;
    listener.strUri = uri_;
    listObj_.OnChange(listener, ref_);
}

} // namespace OHOS::FileManagement::CloudSync
