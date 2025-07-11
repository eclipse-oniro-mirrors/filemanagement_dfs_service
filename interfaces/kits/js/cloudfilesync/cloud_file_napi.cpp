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

#include "cloud_file_download_napi.h"

#include <sys/types.h>

#include "async_work.h"
#include "cloud_file_napi.h"
#include "cloud_sync_manager.h"
#include "dfs_error.h"
#include "utils_log.h"
#include "uv.h"

namespace OHOS::FileManagement::CloudSync {
using namespace FileManagement::LibN;
using namespace std;
const int32_t ARGS_ONE = 1;
int32_t CloudDownloadCallbackImplNapi::StartDownloadInner(const std::string &uri)
{
    int64_t downloadId = 0;
    std::lock_guard<std::mutex> lock(downloadInfoMtx_);
    int32_t ret = 0;
    if (ret != E_OK) {
        LOGE("Start batch download failed! ret = %{public}d", ret);
        return ret;
    }

    downloadInfos_.insert(std::make_pair(downloadId, std::make_shared<SingleProgressNapi>(downloadId)));
    return ret;
}

int32_t CloudDownloadCallbackImplNapi::StopDownloadInner(const std::string &uri)
{
    auto downloadIdList = GetDownloadIdsByUri(uri);
    int32_t ret = E_OK;
    int32_t resErr = E_OK;
    LOGI("Stop Download downloadId list size: %{public}zu", downloadIdList.size());
    for (auto taskId : downloadIdList) {
        LOGI("Stop Download downloadId: %{public}lld", static_cast<long long>(taskId));
        if (resErr != E_OK) {
            ret = resErr;
            continue;
        }
    }
    if (ret != E_OK) {
        LOGE("Stop Download failed! ret = %{public}d", ret);
    }
    return ret;
}

napi_value CloudFileNapi::Constructor(napi_env env, napi_callback_info info)
{
    LOGI("CloudFileNapi::Constructor begin");
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::ZERO)) {
        LOGE("Start Number of arguments unmatched");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    isDownloadCallbackRegistered_ = false;
    LOGI("CloudFileNapi::Constructor end");
    return funcArg.GetThisVar();
}

napi_value CloudFileNapi::Start(napi_env env, napi_callback_info info)
{
    LOGI("Start begin");
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::ONE, NARG_CNT::TWO)) {
        LOGE("Start Number of arguments unmatched");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    auto [succUri, uri, ignore] = NVal(env, funcArg[NARG_POS::FIRST]).ToUTF8String();
    if (!succUri) {
        LOGE("Start get uri parameter failed!");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    auto cbExec = [uri = string(uri.get()), env = env]() -> NError {
        lock_guard<mutex> lock(mtx_);
        if (callback_ != nullptr && !isDownloadCallbackRegistered_) {
            int32_t ret = CloudSyncManager::GetInstance().RegisterDownloadFileCallback(callback_);
            if (ret != E_OK) {
                LOGE("RegisterDownloadFileCallback error, ret: %{public}d", ret);
                return NError(Convert2JsErrNum(ret));
            }
            isDownloadCallbackRegistered_ = true;
        }

        int32_t ret = CloudSyncManager::GetInstance().StartDownloadFile(uri);
        if (ret != E_OK) {
            LOGE("Start Download failed! ret = %{public}d", ret);
            return NError(Convert2JsErrNum(ret));
        }
        LOGI("Start Download Success!");
        return NError(ERRNO_NOERR);
    };

    auto cbCompl = [](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return NVal::CreateUndefined(env);
    };

    string procedureName = "cloudFileDownload";
    auto asyncWork = GetPromiseOrCallBackWork(env, funcArg, static_cast<size_t>(NARG_CNT::TWO));
    return asyncWork == nullptr ? nullptr : asyncWork->Schedule(procedureName, cbExec, cbCompl).val_;
}

CloudDownloadCallbackImpl::CloudDownloadCallbackImpl(napi_env env, napi_value fun, bool isBatch) : env_(env)
{
    if (fun != nullptr) {
        napi_create_reference(env_, fun, 1, &cbOnRef_);
    }
    isBatch_ = isBatch;
}

void CloudDownloadCallbackImpl::OnComplete(UvChangeMsg *msg)
{
    auto downloadcCallback = msg->CloudDownloadCallback_.lock();
    if (downloadcCallback == nullptr || downloadcCallback->cbOnRef_ == nullptr) {
        LOGE("downloadcCallback->cbOnRef_ is nullptr");
        return;
    }
    auto env = downloadcCallback->env_;
    auto ref = downloadcCallback->cbOnRef_;
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
    if (!msg->isBatch_) {
        obj.AddProp("state", NVal::CreateInt32(env, (int32_t)msg->downloadProgress_.state).val_);
        obj.AddProp("processed", NVal::CreateInt64(env, msg->downloadProgress_.downloadedSize).val_);
        obj.AddProp("size", NVal::CreateInt64(env, msg->downloadProgress_.totalSize).val_);
        obj.AddProp("uri", NVal::CreateUTF8String(env, msg->downloadProgress_.path).val_);
    } else {
        LOGI("Batch download callback items");
        obj.AddProp("state", NVal::CreateInt32(env, (int32_t)msg->downloadProgress_.batchState).val_);
        obj.AddProp("downloadedSize", NVal::CreateInt64(env, msg->downloadProgress_.batchDownloadSize).val_);
        obj.AddProp("totalSize", NVal::CreateInt64(env, msg->downloadProgress_.batchTotalSize).val_);
        obj.AddProp("successfulNum", NVal::CreateInt64(env, msg->downloadProgress_.batchSuccNum).val_);
        obj.AddProp("failedNum", NVal::CreateInt64(env, msg->downloadProgress_.batchFailNum).val_);
        obj.AddProp("totalNum", NVal::CreateInt64(env, msg->downloadProgress_.batchTotalNum).val_);
    }
    obj.AddProp("taskId", NVal::CreateInt64(env, msg->downloadProgress_.downloadId).val_);
    obj.AddProp("error", NVal::CreateInt32(env, (int32_t)msg->downloadProgress_.downloadErrorType).val_);

    LOGI("OnComplete callback start for taskId: %{public}lld",
         static_cast<long long>(msg->downloadProgress_.downloadId));
    napi_value retVal = nullptr;
    napi_value global = nullptr;
    napi_get_global(env, &global);
    status = napi_call_function(env, global, jsCallback, ARGS_ONE, &(obj.val_), &retVal);
    if (status != napi_ok) {
        LOGE("napi call function failed, status: %{public}d", status);
    }
    napi_close_handle_scope(env, scope);
}

void CloudDownloadCallbackImpl::OnDownloadProcess(const DownloadProgressObj &progress)
{
    UvChangeMsg *msg = new (std::nothrow) UvChangeMsg(shared_from_this(), progress, isBatch_);
    if (msg == nullptr) {
        LOGE("Failed to create uv message object");
        return;
    }
    auto task = [msg]() {
        if (msg->CloudDownloadCallback_.expired()) {
            LOGE("CloudDownloadCallback_ is expired");
            delete msg;
            return;
        }
        msg->CloudDownloadCallback_.lock()->OnComplete(msg);
        delete msg;
    };
    napi_status ret = napi_send_event(env_, task, napi_event_priority::napi_eprio_immediate);
    if (ret != napi_ok) {
        LOGE("Failed to execute libuv work queue, ret: %{public}d", ret);
        delete msg;
        return;
    }
}

void CloudDownloadCallbackImpl::DeleteReference()
{
    if (cbOnRef_ != nullptr) {
        napi_delete_reference(env_, cbOnRef_);
        cbOnRef_ = nullptr;
        return;
    }
}

napi_value CloudFileNapi::On(napi_env env, napi_callback_info info)
{
    LOGI("On begin");
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::TWO)) {
        LOGE("On Number of arguments unmatched");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    auto [succProgress, progress, ignore] = NVal(env, funcArg[NARG_POS::FIRST]).ToUTF8String();
    if (!succProgress || std::string(progress.get()) != "progress") {
        LOGE("On get progress failed!");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    if (!NVal(env, funcArg[NARG_POS::SECOND]).TypeIs(napi_function)) {
        LOGE("Argument type mismatch");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    if (callback_ != nullptr) {
        LOGI("callback already exist");
        return NVal::CreateUndefined(env).val_;
    }

    callback_ = make_shared<CloudDownloadCallbackImpl>(env, NVal(env, funcArg[(int)NARG_POS::SECOND]).val_);

    return NVal::CreateUndefined(env).val_;
}

napi_value CloudFileNapi::Off(napi_env env, napi_callback_info info)
{
    LOGI("Off begin");
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::ONE, NARG_CNT::TWO)) {
        LOGE("Off Number of arguments unmatched");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    auto [succProgress, progress, ignore] = NVal(env, funcArg[NARG_POS::FIRST]).ToUTF8String();
    if (!succProgress || std::string(progress.get()) != "progress") {
        LOGE("Off get progress failed!");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    if (funcArg.GetArgc() == (uint)NARG_CNT::TWO && !NVal(env, funcArg[NARG_POS::SECOND]).TypeIs(napi_function)) {
        LOGE("Argument type mismatch");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    /* callback_ may be nullptr */
    int32_t ret = CloudSyncManager::GetInstance().UnregisterDownloadFileCallback();
    if (ret != E_OK) {
        LOGE("UnregisterDownloadFileCallback error, ret: %{public}d", ret);
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

napi_value CloudFileNapi::Stop(napi_env env, napi_callback_info info)
{
    LOGI("Stop begin");
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::ONE, NARG_CNT::THREE)) {
        LOGE("Stop Number of arguments unmatched");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    auto [succUri, uri, ignore] = NVal(env, funcArg[NARG_POS::FIRST]).ToUTF8String();
    if (!succUri) {
        LOGE("Stop get uri parameter failed!");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool needClean = false;
    size_t maxArgSize = static_cast<size_t>(NARG_CNT::TWO);
    if (funcArg.GetArgc() >= NARG_CNT::TWO) {
        NVal ui(env, NVal(env, funcArg[(int)NARG_POS::SECOND]).val_);
        if (ui.TypeIs(napi_boolean)) {
            bool needCleanIgnore;
            std::tie(needCleanIgnore, needClean) = NVal(env, funcArg[NARG_POS::SECOND]).ToBool();
            maxArgSize = static_cast<size_t>(NARG_CNT::THREE);
        }
    }
    if (funcArg.GetArgc() == NARG_CNT::THREE) {
            maxArgSize = static_cast<size_t>(NARG_CNT::THREE);
    }
    auto cbExec = [uri = string(uri.get()), env = env, needClean]() -> NError {
        int32_t ret = CloudSyncManager::GetInstance().StopDownloadFile(uri, needClean);
        if (ret != E_OK) {
            LOGE("Stop Download failed! ret = %{public}d", ret);
            return NError(Convert2JsErrNum(ret));
        }
        LOGI("Stop Download Success!");
        return NError(ERRNO_NOERR);
    };

    auto cbCompl = [](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return NVal::CreateUndefined(env);
    };

    string procedureName = "cloudFileDownload";
    auto asyncWork = GetPromiseOrCallBackWork(env, funcArg, maxArgSize);
    return asyncWork == nullptr ? nullptr : asyncWork->Schedule(procedureName, cbExec, cbCompl).val_;
}

bool CloudFileNapi::ToExport(std::vector<napi_property_descriptor> props)
{
    std::string className = GetClassName();
    auto [succ, classValue] =
        NClass::DefineClass(exports_.env_, className, Constructor, std::move(props));
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

bool CloudFileNapi::Export()
{
    vector<napi_property_descriptor> props = {
        NVal::DeclareNapiFunction("start", CloudFileNapi::Start),
        NVal::DeclareNapiFunction("on", CloudFileNapi::On),
        NVal::DeclareNapiFunction("off", CloudFileNapi::Off),
        NVal::DeclareNapiFunction("stop", CloudFileNapi::Stop),
    };
    return ToExport(props);
}

void CloudFileNapi::SetClassName(std::string classname)
{
    className_ = classname;
}

string CloudFileNapi::GetClassName()
{
    return className_;
}
} // namespace OHOS::FileManagement::CloudSync