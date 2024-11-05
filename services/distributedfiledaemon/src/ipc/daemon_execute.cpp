/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "daemon_execute.h"

#include <memory>

#include "asset_callback_manager.h"
#include "dfs_error.h"
#include "network/softbus/softbus_asset_send_listener.h"
#include "network/softbus/softbus_handler_asset.h"
#include "network/softbus/softbus_session_pool.h"
#include "refbase.h"
#include "sandbox_helper.h"
#include "utils_directory.h"
#include "utils_log.h"

#include "all_connect/all_connect_manager.h"
#include "network/softbus/softbus_handler.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
using namespace OHOS::AppFileService;
using namespace OHOS::FileManagement;
const std::string ZIP_FILENAME = "pushData.zip";
DaemonExecute::DaemonExecute()
{
    LOGI("DaemonExecute begin.");
    executeFuncMap_[DEAMON_EXECUTE_PUSH_ASSET] = &DaemonExecute::ExecutePushAsset;
    executeFuncMap_[DEAMON_EXECUTE_REQUEST_SEND_FILE] = &DaemonExecute::ExecuteRequestSendFile;
}

void DaemonExecute::ProcessEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        LOGE("event is nullptr.");
        return;
    }
    std::lock_guard<std::mutex> lock(executeFuncMapMutex_);
    auto itFunc = executeFuncMap_.find(event->GetInnerEventId());
    if (itFunc == executeFuncMap_.end()) {
        LOGE("not find execute func.");
        return;
    }

    auto executeFunc = itFunc->second;
    (this->*executeFunc)(event);
}

void DaemonExecute::ExecutePushAsset(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        LOGI("eventhandler fail.");
        return;
    }
    auto pushData = event->GetSharedObject<PushAssetData>();
    if (pushData == nullptr) {
        LOGE("pushData is nullptr.");
        return;
    }
    int32_t userId = pushData->userId_;
    auto assetObj = pushData->assetObj_;
    if (assetObj == nullptr) {
        LOGE("assetObj is nullptr.");
        return;
    }
    int32_t socketId;
    auto ret = SoftBusHandlerAsset::GetInstance().AssetBind(assetObj->dstNetworkId_, socketId);
    if (ret != E_OK) {
        LOGE("ExecutePushAsset AssetBind failed, ret %{public}d", ret);
        auto taskId = assetObj->srcBundleName_ + assetObj->sessionId_;
        AssetCallbackManager::GetInstance().NotifyAssetSendResult(taskId, assetObj, FileManagement::E_EVENT_HANDLER);
        AssetCallbackManager::GetInstance().RemoveSendCallback(taskId);
        return;
    }
    SoftBusHandlerAsset::GetInstance().AddAssetObj(socketId, assetObj);

    auto fileList = GetFileList(assetObj->uris_, userId, assetObj->srcBundleName_);
    if (fileList.empty()) {
        LOGE("get fileList is empty.");
        HandlePushAssetFail(socketId, assetObj);
        return;
    }

    std::string sendFileName;
    bool isSingleFile;
    ret = HandleZip(fileList, assetObj->srcBundleName_, sendFileName, isSingleFile);
    if (ret != E_OK) {
        LOGE("zip files fail. socketId is %{public}d", socketId);
        HandlePushAssetFail(socketId, assetObj);
        SoftBusHandlerAsset::GetInstance().RemoveFile(sendFileName, !isSingleFile);
        return;
    }

    ret = SoftBusHandlerAsset::GetInstance().AssetSendFile(socketId, sendFileName, isSingleFile);
    if (ret != E_OK) {
        LOGE("ExecutePushAsset send file fail, ret %{public}d", ret);
        HandlePushAssetFail(socketId, assetObj);
        SoftBusHandlerAsset::GetInstance().RemoveFile(sendFileName, !isSingleFile);
        return;
    }
}

void DaemonExecute::ExecuteRequestSendFile(const AppExecFwk::InnerEvent::Pointer &event)
{
    if (event == nullptr) {
        LOGI("eventhandler fail.");
        return;
    }
    auto requestSendFileData = event->GetSharedObject<RequestSendFileData>();
    if (requestSendFileData == nullptr) {
        LOGE("requestSendFileData is nullptr.");
        return;
    }

    auto requestSendFileBlock  = requestSendFileData->requestSendFileBlock_;
    if (requestSendFileBlock == nullptr) {
        LOGE("requestSendFileBlock is nullptr.");
        return;
    }
    std::string srcUri = requestSendFileData->srcUri_;
    std::string dstPath  = requestSendFileData->dstPath_;
    std::string dstDeviceId  = requestSendFileData->dstDeviceId_;
    std::string sessionName  = requestSendFileData->sessionName_;
    if (srcUri.empty() || dstDeviceId.empty() || sessionName.empty()) {
        LOGE("params empty. %{public}s, %{public}s", sessionName.c_str(), Utils::GetAnonyString(dstDeviceId).c_str());
        requestSendFileBlock->SetValue(ERR_BAD_VALUE);
        return;
    }

    requestSendFileBlock->SetValue(RequestSendFileInner(srcUri, dstPath, dstDeviceId, sessionName));
}

int32_t DaemonExecute::RequestSendFileInner(const std::string &srcUri,
                                            const std::string &dstPath,
                                            const std::string &dstDeviceId,
                                            const std::string &sessionName)
{
    LOGI("RequestSendFile begin dstDeviceId: %{public}s", Utils::GetAnonyString(dstDeviceId).c_str());
    auto resourceReq = AllConnectManager::GetInstance().BuildResourceRequest();
    int32_t ret = AllConnectManager::GetInstance().ApplyAdvancedResource(dstDeviceId, resourceReq.get());
    if (ret != E_OK) {
        LOGE("ApplyAdvancedResource fail, ret is %{public}d", ret);
        return ERR_APPLY_RESULT;
    }

    int32_t socketId;
    ret = SoftBusHandler::GetInstance().OpenSession(sessionName, sessionName, dstDeviceId, socketId);
    if (ret != E_OK) {
        LOGE("OpenSession failed, ret is %{public}d", ret);
        return ret;
    }
    AllConnectManager::GetInstance().PublishServiceState(dstDeviceId,
                                                         ServiceCollaborationManagerBussinessStatus::SCM_CONNECTED);
    LOGI("RequestSendFile OpenSession success");

    ret = SoftBusHandler::GetInstance().CopySendFile(socketId, sessionName, srcUri, dstPath);
    if (ret != E_OK) {
        LOGE("CopySendFile failed, ret is %{public}d", ret);
        SoftBusHandler::GetInstance().CloseSession(socketId, sessionName);
        return ret;
    }
    return E_OK;
}

std::vector<std::string> DaemonExecute::GetFileList(const std::vector<std::string> &uris,
                                                    int32_t userId,
                                                    const std::string &srcBundleName)
{
    std::vector<std::string> fileList;
    for (const auto &uri : uris) {
        size_t pos = uri.find(srcBundleName);
        if (pos == std::string::npos) {
            LOGE("srcBundleName not find in uri.");
            return {};
        }

        std::string physicalPath;
        int32_t ret = SandboxHelper::GetPhysicalPath(uri, std::to_string(userId), physicalPath);
        if (ret != E_OK) {
            LOGE("invalid uri, ret = %{public}d", ret);
            return {};
        }
        if (!SandboxHelper::CheckValidPath(physicalPath)) {
            LOGE("invalid path : %{public}s", GetAnonyString(physicalPath).c_str());
            return {};
        }

        if (OHOS::Storage::DistributedFile::Utils::IsFolder(physicalPath)) {
            LOGE("uri is folder are not supported now.");
            return {};
        }
        fileList.emplace_back(physicalPath);
    }
    LOGI("GetFileList success, file num is %{public}s", std::to_string(fileList.size()).c_str());
    return fileList;
}

int32_t DaemonExecute::HandleZip(const std::vector<std::string> &fileList,
                                 const std::string &srcBundleName,
                                 std::string &sendFileName,
                                 bool &isSingleFile)
{
    if (fileList.size() > 1) {
        size_t pos = fileList[0].find(srcBundleName);
        if (pos == std::string::npos) {
            LOGE("srcBundleName not find in uri.");
            return E_ZIP;
        }
        std::string relativePath = fileList[0].substr(0, pos + srcBundleName.length()) + "/";
        sendFileName = relativePath + ZIP_FILENAME;
        int32_t ret = SoftBusHandlerAsset::GetInstance().CompressFile(fileList, relativePath, sendFileName);
        if (ret != E_OK) {
            LOGE("zip ffiles fail.");
            return E_ZIP;
        }
        isSingleFile = false;
        SoftBusAssetSendListener::isSingleFile_ = false;
        return E_OK;
    } else {
        sendFileName = fileList[0];
        isSingleFile = true;
        SoftBusAssetSendListener::isSingleFile_ = true;
        return E_OK;
    }
}

void DaemonExecute::HandlePushAssetFail(int32_t socketId, const sptr<AssetObj> &assetObj)
{
    auto taskId = assetObj->srcBundleName_ + assetObj->sessionId_;
    AssetCallbackManager::GetInstance().NotifyAssetSendResult(taskId, assetObj, FileManagement::E_EVENT_HANDLER);
    SoftBusHandlerAsset::GetInstance().closeAssetBind(socketId);
    AssetCallbackManager::GetInstance().RemoveSendCallback(taskId);
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
