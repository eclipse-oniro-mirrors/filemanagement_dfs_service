/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "sendfile.h"

#include <tuple>

#include "napi/n_class.h"
#include "napi/n_func_arg.h"
#include "napi/n_val.h"
#include "uni_error.h"

#include "napi/n_async/n_async_work_callback.h"
#include "napi/n_async/n_async_work_promise.h"

#include "system_ability_definition.h"
#include "i_distributedfile_service.h"
#include "service_proxy.h"

#include <iservice_registry.h>
#include <system_ability_manager.h>

namespace OHOS {
namespace DistributedFS {
namespace ModuleSendFile {
using namespace OHOS::Storage::DistributedFile;

napi_value SendFile(napi_env env, napi_callback_info info)
{
    NFuncArg funcArg(env, info);
    if (!funcArg.InitArgs(NARG_CNT::FOUR, NARG_CNT::FIVE)) {
        UniError(EINVAL).ThrowErr(env, "Number of arguments unmatched");
        return nullptr;
    }

    bool succ = false;
    auto resultCode = std::make_shared<int32_t>();

    std::unique_ptr<char []> deviceId;
    std::vector<std::string> sourPath;
    std::vector<std::string> destPath;

    uint32_t fileCount = 0;

    std::tie(succ, deviceId, std::ignore) = NVal(env, funcArg[NARG_POS::FIRST]).ToUTF8String();
    napi_get_value_uint32(env, funcArg[NARG_POS::FOURTH], &fileCount);

    HILOGI("SendFile::deviceId :%{public}s", deviceId.get());

    napi_value result;
    napi_status status;
    for (uint32_t i=0; i < fileCount; i++) {
        status = napi_get_element(env, funcArg[NARG_POS::SECOND], i, &result);
        if (napi_ok != status) {
            return nullptr;
        }
        std::unique_ptr<char []> path;
        std::tie(succ, path, std::ignore) = NVal(env, result).ToUTF8String();
        sourPath.push_back(path.get());
        HILOGI("SendFile::JS_SendFile src[%{public}d]:%{public}s", i, sourPath.at(i).c_str());

        status = napi_get_element(env, funcArg[NARG_POS::THIRD], i, &result);
        if (napi_ok != status) {
            return nullptr;
        }
        std::tie(succ, path, std::ignore) = NVal(env, result).ToUTF8String();
        destPath.push_back(path.get());
        HILOGI("SendFile::JS_SendFile src[%{public}d]:%{public}s", i, destPath.at(i).c_str());
    }

    HILOGI("napi_value source: %{public}d, dest: %{public}d", sourPath.size(), destPath.size());
    std::string deviceIdString(deviceId.get());
    auto cbExec = [deviceIdString, sourPath, destPath, fileCount, resultCode](napi_env env) -> UniError {
        *resultCode = ExecSendFile(deviceIdString.c_str(), sourPath, destPath, fileCount);
        HILOGI("cbExec resultCode : %{public}d", *resultCode);
        return UniError(ERRNO_NOERR);
    };

    auto cbComplete = [resultCode](napi_env env, UniError err) -> NVal {
        if (err) {
            return { env, err.GetNapiErr(env) };
        }
        HILOGI("cbComplete resultCode : %{public}d", *resultCode);
        return { NVal::CreateInt64(env, *resultCode) };
    };

    HILOGI("napi_value SendFile created thread params count %{public}d", funcArg.GetArgc());
    std::string procedureName = "SendFile";
    NVal thisVar(env, funcArg.GetThisVar());
    if (funcArg.GetArgc() == NARG_CNT::FOUR) {
        HILOGI("promise function");
        return NAsyncWorkPromise(env, thisVar).Schedule(procedureName, cbExec, cbComplete).val_;
    } else if (funcArg.GetArgc() == NARG_CNT::FIVE) {
        HILOGI("callback function");
        NVal cb(env, funcArg[NARG_POS::FIFTH]);
        return NAsyncWorkCallback(env, thisVar, cb).Schedule(procedureName, cbExec, cbComplete).val_;
    }
    HILOGI("napi_value SendFile End");
    return NVal::CreateUndefined(env).val_;
}

int32_t ExecSendFile(const std::string &deviceId, const std::vector<std::string>& srcList,
    const std::vector<std::string>& dstList, uint32_t fileCnt)
{
    sptr<ISystemAbilityManager> systemAbilityMgr =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        HILOGE("BundleService Get ISystemAbilityManager failed ... \n");
        return -1;
    }
    sptr<IRemoteObject> remote = systemAbilityMgr->GetSystemAbility(STORAGE_DISTRIBUTED_FILE_SERVICE_SA_ID);
    if (remote == nullptr) {
        HILOGE("DistributedFileService Get STORAGE_DISTRIBUTED_FILE_SERVICE_SA_ID = %{public}d fail ... \n",
            STORAGE_DISTRIBUTED_FILE_SERVICE_SA_ID);
        return -1;
    }
    HILOGI("ExecSendFile remote %{public}p", remote.GetRefPtr());

    sptr<IDistributedFileService> proxy = iface_cast<IDistributedFileService>(remote);
    if (proxy == nullptr) {
        HILOGE("DistributedFileService == nullptr\n");
        return -1;
    }
    HILOGI("ExecSendFile distributedFileService %{public}p", proxy.GetRefPtr());

    int32_t result = proxy->SendFile(deviceId, srcList, dstList, fileCnt);
    return result;
}
} // namespace ModuleSendFile
} // namespace DistributedFS
} // namespace OHOS