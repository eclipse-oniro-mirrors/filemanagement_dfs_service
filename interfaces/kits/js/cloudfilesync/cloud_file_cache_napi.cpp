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

#include "cloud_file_cache_napi.h"

#include <cstddef>
#include <set>
#include <sys/types.h>

#include "async_work.h"
#include "cloud_sync_manager.h"
#include "dfs_error.h"
#include "multi_download_progress_napi.h"
#include "register_callback_manager_napi.h"
#include "utils_log.h"
#include "uv.h"

namespace OHOS::FileManagement::CloudSync {
using namespace FileManagement::LibN;
using namespace std;

static const std::set<int32_t> errForSingleFileSync = { ENOENT, EACCES, EAGAIN, EINTR, ENOSYS, E_ILLEGAL_URI };

int32_t CloudFileCacheCallbackImplNapi::StartDownloadInner(const std::string &uri, int32_t fieldKey)
{
    int64_t downloadId = 0;
    std::lock_guard<std::mutex> lock(downloadInfoMtx_);
    int32_t ret = CloudSyncManager::GetInstance().StartFileCache({uri}, downloadId, fieldKey, shared_from_this());
    if (ret != E_OK) {
        LOGE("Start single download failed! ret = %{public}d", ret);
        return ret;
    }
    downloadInfos_.insert(std::make_pair(downloadId, std::make_shared<SingleProgressNapi>(downloadId)));
    return ret;
}

int32_t CloudFileCacheCallbackImplNapi::StopDownloadInner(const std::string &uri, bool needClean)
{
    auto downloadIdList = GetDownloadIdsByUri(uri);
    int32_t ret = E_OK;
    int32_t resErr = E_OK;
    LOGI("Stop Download downloadId list size: %{public}zu", downloadIdList.size());
    for (auto taskId : downloadIdList) {
        resErr = CloudSyncManager::GetInstance().StopFileCache(taskId, needClean);
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

int32_t CloudFileCacheCallbackImplNapi::StartDownloadInner(const std::vector<std::string> &uriVec,
                                                           int64_t &downloadId,
                                                           int32_t fieldKey)
{
    std::unique_lock<std::mutex> lock(downloadInfoMtx_);
    int32_t ret = CloudSyncManager::GetInstance().StartFileCache(uriVec, downloadId, fieldKey, shared_from_this());
    if (ret != E_OK) {
        LOGE("Start batch download failed! ret = %{public}d", ret);
        return ret;
    }
    downloadInfos_.insert(std::make_pair(downloadId, std::make_shared<BatchProgressNapi>(downloadId)));
    return ret;
}

int32_t CloudFileCacheCallbackImplNapi::StopDownloadInner(int64_t downloadId, bool needClean)
{
    int32_t ret = CloudSyncManager::GetInstance().StopFileCache(downloadId, needClean);
    if (ret != E_OK) {
        LOGE("Batch stop file cache failed! ret = %{public}d", ret);
    }
    return ret;
}

bool RegisterManager::HasEvent(const string &eventType)
{
    unique_lock<mutex> registerMutex_;
    bool hasEvent = false;
    for (auto &iter : registerInfo_) {
        if (iter->eventType == eventType) {
            hasEvent = true;
            break;
        }
    }
    return hasEvent;
}

bool RegisterManager::AddRegisterInfo(shared_ptr<RegisterInfoArg> info)
{
    if (HasEvent(info->eventType)) {
        return false;
    }
    {
        unique_lock<mutex> registerMutex_;
        registerInfo_.insert(info);
    }
    return true;
}

bool RegisterManager::RemoveRegisterInfo(const string &eventType)
{
    unique_lock<mutex> registerMutex_;
    bool isFound = false;
    for (auto iter = registerInfo_.begin(); iter != registerInfo_.end();) {
        if ((*iter)->eventType == eventType) {
            isFound = true;
            iter = registerInfo_.erase(iter);
        } else {
            iter++;
        }
    }
    return isFound;
}

napi_value CloudFileCacheNapi::Constructor(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::ZERO)) {
        LOGE("Start Number of arguments unmatched");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    auto fileCacheEntity = make_unique<FileCacheEntity>();
    if (!NClass::SetEntityFor<FileCacheEntity>(env, funcArg.GetThisVar(), move(fileCacheEntity))) {
        LOGE("Failed to set file cache entity.");
        NError(E_UNKNOWN_ERR).ThrowErr(env);
        return nullptr;
    }
    return funcArg.GetThisVar();
}

bool CloudFileCacheNapi::ToExport(std::vector<napi_property_descriptor> props)
{
    std::string className = GetClassName();
    auto [succ, classValue] = NClass::DefineClass(exports_.env_, className, Constructor, std::move(props));
    if (!succ) {
        NError(E_UNKNOWN_ERR).ThrowErr(exports_.env_);
        LOGE("Failed to define GallerySync class");
        return false;
    }

    succ = NClass::SaveClass(exports_.env_, className, classValue);
    if (!succ) {
        NError(E_UNKNOWN_ERR).ThrowErr(exports_.env_);
        LOGE("Failed to save GallerySync class");
        return false;
    }

    return exports_.AddProp(className, classValue);
}

napi_value CloudFileCacheNapi::CleanCloudFileCache(napi_env env, napi_callback_info info)
{
    LOGI("CleanCache start");
    NFuncArg funcArg(env, info);

    if (!funcArg.InitArgs(NARG_CNT::ONE)) {
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    auto [succ, uri, ignore] = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        LOGE("Get uri error");
        NError(EINVAL).ThrowErr(env);
        return nullptr;
    }

    int32_t ret = CloudSyncManager::GetInstance().CleanCache(uri.get());
    if (ret != E_OK) {
        NError(Convert2JsErrNum(ret)).ThrowErr(env);
        return nullptr;
    }
    return NVal::CreateUndefined(env).val_;
}

static int32_t GetCleanCacheErrPublic(int32_t result)
{
    LOGE("cleanCache failed, errno : %{public}d", result);
    if (errForSingleFileSync.find(result) != errForSingleFileSync.end()) {
        return result;
    }
    return E_SERVICE_INNER_ERROR;
}

napi_value CloudFileCacheNapi::CleanFileCache(napi_env env, napi_callback_info info)
{
    LOGI("CleanCache start");
    NFuncArg funcArg(env, info);
    
    if (!funcArg.InitArgs(NARG_CNT::ONE)) {
        NError(EINVAL).ThrowErr(env);
        LOGE("Number of arguments unmatched");
        return nullptr;
    }

    auto [succ, uri, ignore] = NVal(env, funcArg[(int)NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        LOGE("CleanFileCache get uri parameter failed!");
        NError(EINVAL).ThrowErr(env);
        return nullptr;
    }

    int32_t ret = CloudSyncManager::GetInstance().CleanFileCache(uri.get());
    if (ret != E_OK) {
        int32_t err = GetCleanCacheErrPublic(ret);
        NError(Convert2JsErrNum(err)).ThrowErr(env);
        LOGE("CleanFileCache failed, ret:%{public}d", err);
        return nullptr;
    }
    return NVal::CreateUndefined(env).val_;
}

bool CloudFileCacheNapi::Export()
{
    std::vector<napi_property_descriptor> props = {
        NVal::DeclareNapiFunction("on", CloudFileCacheNapi::On),
        NVal::DeclareNapiFunction("off", CloudFileCacheNapi::Off),
        NVal::DeclareNapiFunction("start", CloudFileCacheNapi::StartFileCache),
        NVal::DeclareNapiFunction("startBatch", CloudFileCacheNapi::StartBatchFileCache),
        NVal::DeclareNapiFunction("stop", CloudFileCacheNapi::StopFileCache),
        NVal::DeclareNapiFunction("stopBatch", CloudFileCacheNapi::StopBatchFileCache),
        NVal::DeclareNapiFunction("cleanCache", CloudFileCacheNapi::CleanCloudFileCache),
        NVal::DeclareNapiFunction("cleanFileCache", CloudFileCacheNapi::CleanFileCache),
    };

    return ToExport(props);
}

napi_value CloudFileCacheNapi::StartFileCache(napi_env env, napi_callback_info info)
{
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

    auto cbExec = [uri = string(uri.get())]() -> NError {
        int32_t ret = CloudSyncManager::GetInstance().StartFileCache(uri);
        if (ret != E_OK) {
            LOGE("Start Download failed! ret = %{public}d", ret);
            if (ret != E_INVAL_ARG) {
                ret = E_BROKEN_IPC;
            }
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

    string procedureName = "cloudFileCache";
    auto asyncWork = GetPromiseOrCallBackWork(env, funcArg, static_cast<size_t>(NARG_CNT::TWO));
    return asyncWork == nullptr ? nullptr : asyncWork->Schedule(procedureName, cbExec, cbCompl).val_;
}

napi_value CloudFileCacheNapi::StopFileCache(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::ONE, NARG_CNT::THREE)) {
        LOGE("Stop Number of arguments unmatched");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    auto [succ, uri, ignore] = NVal(env, funcArg[NARG_POS::FIRST]).ToUTF8String();
    if (!succ) {
        LOGE("Stop get uri parameter failed!");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    bool needClean = false;
    size_t maxArgSize = static_cast<size_t>(NARG_CNT::TWO);
    if (funcArg.GetArgc() >= NARG_CNT::TWO) {
        NVal option(env, funcArg[NARG_POS::SECOND]);
        if (!option.TypeIs(napi_function)) {
            tie(succ, needClean) = option.ToBool();
            if (!succ) {
                LOGE("Failed to get clean flag!");
                NError(E_PARAMS).ThrowErr(env);
                return nullptr;
            }
            maxArgSize = static_cast<size_t>(NARG_CNT::THREE);
        }
    }
    auto cbExec = [uri = string(uri.get()), env = env, needClean]() -> NError {
        int32_t ret = CloudSyncManager::GetInstance().StopDownloadFile(uri, needClean);
        if (ret != E_OK) {
            LOGE("Stop Download failed! ret = %{public}d", ret);
            if (ret != E_INVAL_ARG) {
                ret = E_BROKEN_IPC;
            }
            return NError(Convert2JsErrNum(ret));
        }
        return NError(ERRNO_NOERR);
    };

    auto cbCompl = [](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return NVal::CreateUndefined(env);
    };

    string procedureName = "cloudFileCache";
    auto asyncWork = GetPromiseOrCallBackWork(env, funcArg, maxArgSize);
    return asyncWork == nullptr ? nullptr : asyncWork->Schedule(procedureName, cbExec, cbCompl).val_;
}

struct FileCacheArg {
    vector<string> uriList;
    int64_t downloadId;
};

napi_value CloudFileCacheNapi::StartBatchFileCache(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::ONE, NARG_CNT::TWO)) {
        LOGE("Start Number of arguments unmatched");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    auto fileUris = std::make_shared<FileCacheArg>();
    auto [resGetUris, uriArray, ignore] = NVal(env, funcArg[NARG_POS::FIRST]).ToStringArray();
    if (!resGetUris) {
        LOGE("Start get uri array parameter failed!");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    fileUris->uriList = uriArray;
    auto cbExec = [fileUris]() -> NError {
        int32_t ret = CloudSyncManager::GetInstance().StartFileCache(fileUris->uriList, fileUris->downloadId);
        if (ret != E_OK) {
            LOGE("Batch start file cache failed! ret = %{public}d", ret);
            ret = (ret == E_CLOUD_SDK) ? E_UNKNOWN_ERR : ret;
            return NError(Convert2JsErrNum(ret));
        }
        return NError(ERRNO_NOERR);
    };

    auto cbCompl = [fileUris](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return NVal::CreateInt64(env, fileUris->downloadId);
    };

    string procedureName = "cloudFileCache";
    auto asyncWork = GetPromiseOrCallBackWork(env, funcArg, static_cast<size_t>(NARG_CNT::TWO));
    return asyncWork == nullptr ? nullptr : asyncWork->Schedule(procedureName, cbExec, cbCompl).val_;
}

napi_value CloudFileCacheNapi::StopBatchFileCache(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::ONE, NARG_CNT::THREE)) {
        LOGE("Start Number of arguments unmatched");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    auto [succ, downloadId] = NVal(env, funcArg[NARG_POS::FIRST]).ToInt64();
    if (!succ || downloadId <= 0) {
        LOGE("Start get download ID parameter failed!");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    bool needClean = false;
    size_t maxArgSize = static_cast<size_t>(NARG_CNT::TWO);
    if (funcArg.GetArgc() >= NARG_CNT::TWO) {
        NVal option(env, funcArg[NARG_POS::SECOND]);
        if (!option.TypeIs(napi_function)) {
            tie(succ, needClean) = option.ToBool();
            if (!succ) {
                LOGE("Failed to get clean flag!");
                NError(E_PARAMS).ThrowErr(env);
                return nullptr;
            }
            maxArgSize = static_cast<size_t>(NARG_CNT::THREE);
        }
    }

    auto cbExec = [downloadId = downloadId, needClean]() -> NError {
        int32_t ret = CloudSyncManager::GetInstance().StopFileCache(downloadId, needClean);
        if (ret != E_OK) {
            LOGE("Batch stop file cache failed! ret = %{public}d", ret);
            ret = (ret == E_CLOUD_SDK) ? E_UNKNOWN_ERR : ret;
            return NError(Convert2JsErrNum(ret));
        }
        return NError(ERRNO_NOERR);
    };

    auto cbCompl = [](napi_env env, NError err) -> NVal {
        if (err) {
            return {env, err.GetNapiErr(env)};
        }
        return NVal::CreateUndefined(env);
    };

    string procedureName = "cloudFileCache";
    auto asyncWork = GetPromiseOrCallBackWork(env, funcArg, maxArgSize);
    return asyncWork == nullptr ? nullptr : asyncWork->Schedule(procedureName, cbExec, cbCompl).val_;
}

napi_value CloudFileCacheNapi::On(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::TWO)) {
        LOGE("Batch-On Number of arguments unmatched");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    auto [succProgress, progress, ignore] = NVal(env, funcArg[NARG_POS::FIRST]).ToUTF8String();
    string eventType(progress.get());
    if (!succProgress || (eventType != PROGRESS && eventType != MULTI_PROGRESS)) {
        LOGE("Batch-On get progress failed!");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    if (!NVal(env, funcArg[NARG_POS::SECOND]).TypeIs(napi_function)) {
        LOGE("Batch-On argument type mismatch");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    auto fileCacheEntity = NClass::GetEntityOf<FileCacheEntity>(env, funcArg.GetThisVar());
    if (!fileCacheEntity) {
        LOGE("Failed to get file cache entity.");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    auto arg = make_shared<RegisterInfoArg>();
    arg->eventType = eventType;
    if (eventType == PROGRESS) {
        arg->callback = make_shared<CloudDownloadCallbackImpl>(env, NVal(env, funcArg[(int)NARG_POS::SECOND]).val_);
    } else {
        arg->callback =
            make_shared<CloudDownloadCallbackImpl>(env, NVal(env, funcArg[(int)NARG_POS::SECOND]).val_, true);
    }

    if (!fileCacheEntity->registerMgr.AddRegisterInfo(arg)) {
        LOGE("Batch-On register callback fail, callback already exist");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    int32_t ret = CloudSyncManager::GetInstance().RegisterFileCacheCallback(arg->callback);
    if (ret != E_OK) {
        LOGE("Failed to register callback, error: %{public}d", ret);
        (void)fileCacheEntity->registerMgr.RemoveRegisterInfo(eventType);
        NError(Convert2JsErrNum(ret)).ThrowErr(env);
        return nullptr;
    }

    return NVal::CreateUndefined(env).val_;
}

napi_value CloudFileCacheNapi::Off(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::ONE, NARG_CNT::TWO)) {
        LOGE("Off Number of arguments unmatched");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }
    auto [succProgress, progress, ignore] = NVal(env, funcArg[NARG_POS::FIRST]).ToUTF8String();
    string eventType(progress.get());
    if (!succProgress || (eventType != PROGRESS && eventType != MULTI_PROGRESS)) {
        LOGE("Batch-Off get progress failed!");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    if (funcArg.GetArgc() == (uint)NARG_CNT::TWO && !NVal(env, funcArg[NARG_POS::SECOND]).TypeIs(napi_function)) {
        LOGE("Batch-Off argument type mismatch");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    auto fileCacheEntity = NClass::GetEntityOf<FileCacheEntity>(env, funcArg.GetThisVar());
    if (!fileCacheEntity) {
        LOGE("Failed to get file cache entity.");
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    if (!fileCacheEntity->registerMgr.HasEvent(eventType)) {
        LOGE("Batch-Off no callback is registered for this event type: %{public}s.", eventType.c_str());
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    int32_t ret = CloudSyncManager::GetInstance().UnregisterFileCacheCallback();
    if (ret != E_OK) {
        LOGE("Failed to unregister callback, error: %{public}d", ret);
        NError(Convert2JsErrNum(ret)).ThrowErr(env);
        return nullptr;
    }

    if (!fileCacheEntity->registerMgr.RemoveRegisterInfo(eventType)) {
        LOGE("Batch-Off no callback is registered for this event type: %{public}s.", eventType.c_str());
        NError(E_PARAMS).ThrowErr(env);
        return nullptr;
    }

    return NVal::CreateUndefined(env).val_;
}

string CloudFileCacheNapi::GetClassName()
{
    return className_;
}
} // namespace OHOS::FileManagement::CloudSync