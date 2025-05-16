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

#include "network/softbus/softbus_asset_recv_listener.h"

#include <filesystem>
#include <memory>

#include "accesstoken_kit.h"
#include "all_connect/all_connect_manager.h"
#include "asset_callback_manager.h"
#include "dfs_error.h"
#include "ipc_skeleton.h"
#include "network/softbus/softbus_handler_asset.h"
#include "os_account_manager.h"
#include "refbase.h"
#include "utils_log.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
const std::string HMDFS_PATH = "/mnt/hmdfs/{userId}/account/device_view/local/data/";
const std::string USRT_ID_FLAG = "{userId}";
const std::string TEMP_DIR = "ASSET_TEMP/";
const std::string ASSET_FLAG_SINGLE = ".asset_single?";
const std::string ASSET_FLAG_ZIP = ".asset_zip?";
void SoftbusAssetRecvListener::OnFile(int32_t socket, FileEvent *event)
{
    if (event == nullptr) {
        LOGE("invalid parameter");
        return;
    }
    switch (event->type) {
        case FILE_EVENT_RECV_UPDATE_PATH:
            event->UpdateRecvPath = GetRecvPath;
            break;
        case FILE_EVENT_RECV_START:
            OnRecvAssetStart(socket, event->files, event->fileCnt);
            break;
        case FILE_EVENT_RECV_PROCESS:
            OnRecvAssetProcess(socket, event->files, event->bytesTotal, event->bytesProcessed);
            break;
        case FILE_EVENT_RECV_FINISH:
            OnRecvAssetFinished(socket, event->files, event->fileCnt);
            break;
        case FILE_EVENT_RECV_ERROR:
            OnRecvAssetError(socket, event->errorCode, event->files, event->fileCnt);
            break;
        default:
            LOGI("Other situations");
            break;
    }
}

const char* SoftbusAssetRecvListener::GetRecvPath()
{
    int32_t userId = GetCurrentUserId();

    std::string path = HMDFS_PATH;
    auto pos = path.find(USRT_ID_FLAG);
    if (pos == std::string::npos) {
        return "";
    }
    path_ = path.replace(pos, USRT_ID_FLAG.length(), std::to_string(userId));
    const char* recvPath = path_.c_str();
    LOGI("SoftbusAssetRecvListener GetRecvPath: %{public}s", recvPath);
    return recvPath;
}

void SoftbusAssetRecvListener::OnRecvAssetStart(int32_t socketId, const char **fileList, int32_t fileCnt)
{
    std::lock_guard<std::mutex> lock(mtx_);
    LOGI("OnRecvFileStart, sessionId = %{public}d, fileCnt = %{public}d", socketId, fileCnt);
    if (fileCnt == 0) {
        LOGE("fileList has no file");
        return;
    }
    auto srcNetworkId = SoftBusHandlerAsset::GetInstance().GetClientInfo(socketId);
    if (srcNetworkId.empty()) {
        LOGE("get srcNetworkId fail");
        return;
    }
    std::string filePath(path_ + fileList[0]);
    sptr<AssetObj> assetObj (new (std::nothrow) AssetObj());
    if (assetObj == nullptr) {
        LOGE("new assetObj failed!");
        return;
    }
    int32_t ret = SoftBusHandlerAsset::GetInstance().GenerateAssetObjInfo(socketId, filePath, assetObj);
    if (ret != FileManagement::ERR_OK) {
        LOGE("Generate assetObjInfo fail");
        return;
    }
    SoftBusHandlerAsset::GetInstance().AddAssetObj(socketId, assetObj);
    AssetCallbackManager::GetInstance().NotifyAssetRecvStart(srcNetworkId,
                                                             assetObj->dstNetworkId_,
                                                             assetObj->sessionId_,
                                                             assetObj->dstBundleName_);
}

void SoftbusAssetRecvListener::OnRecvAssetProcess(int32_t socketId, const char **fileList, uint64_t bytesTotal, uint64_t bytesUpload)
{
    std::lock_guard<std::mutex> lock(mtx_);

    LOGI("OnRecvFileProcess, sessionId = %{public}d, bytesTotal = %{public}llu, bytesUpload = %{public}llu", socketId, bytesTotal, bytesUpload);

    auto srcNetworkId = SoftBusHandlerAsset::GetInstance().GetClientInfo(socketId);
    if (srcNetworkId.empty()) {
        LOGE("get srcNetworkId fail");
        return;
    }

    std::string filePath(path_ + fileList[0]);
    sptr<AssetObj> assetObj (new (std::nothrow) AssetObj());
    if (assetObj == nullptr) {
        LOGE("new assetObj failed!");
        return;
    }
    int32_t ret = SoftBusHandlerAsset::GetInstance().GenerateAssetObjInfo(socketId, filePath, assetObj);
    if (ret != FileManagement::ERR_OK) {
        LOGE("Generate assetObjInfo fail");
        return;
    }
    AssetCallbackManager::GetInstance().NotifyAssetRecvProcess(srcNetworkId,
                                                               assetObj,
                                                               bytesTotal,
                                                               bytesUpload);

}

void SoftbusAssetRecvListener::OnRecvAssetFinished(int32_t socketId, const char **fileList, int32_t fileCnt)
{
    std::lock_guard<std::mutex> lock(mtx_);
    LOGI("OnRecvFileFinished, sessionId = %{public}d, fileCnt = %{public}d", socketId, fileCnt);
    if (fileCnt == 0) {
        LOGE("fileList has no file");
        return;
    }
    auto srcNetworkId = SoftBusHandlerAsset::GetInstance().GetClientInfo(socketId);
    if (srcNetworkId.empty()) {
        LOGE("get srcNetworkId fail");
        return;
    }
    std::string firstFilePath(path_ + fileList[0]);
    sptr<AssetObj> assetObj (new (std::nothrow) AssetObj());
    if (assetObj == nullptr) {
        LOGE("new assetObj failed!");
        return;
    }
    int32_t ret = SoftBusHandlerAsset::GetInstance().GenerateAssetObjInfo(socketId, firstFilePath, assetObj);
    if (ret != FileManagement::ERR_OK) {
        LOGE("Generate assetObjInfo fail");
        return;
    }
    for (int32_t i = 0; i < fileCnt; i++) {
        std::string filePath(path_ + fileList[i]);
        if (JudgeSingleFile(filePath)) {
            ret = HandleSingleFile(socketId, filePath, assetObj);
        } else {
            ret = HandleZipFile(socketId, filePath, assetObj);
        }

        if (ret != FileManagement::ERR_OK) {
            LOGE("MoveAsset fail, socket %{public}d", socketId);
            AssetCallbackManager::GetInstance().NotifyAssetRecvFinished(srcNetworkId, assetObj,
                                                                        FileManagement::ERR_BAD_VALUE);
            SoftBusHandlerAsset::GetInstance().RemoveClientInfo(socketId);
            return;
        }
    }
    RemoveAsset(firstFilePath);
    AssetCallbackManager::GetInstance().NotifyAssetRecvFinished(srcNetworkId, assetObj, FileManagement::ERR_OK);
    SoftBusHandlerAsset::GetInstance().RemoveClientInfo(socketId);
}

void SoftbusAssetRecvListener::OnRecvAssetError(int32_t socketId, int32_t errorCode,
                                                const char **fileList, int32_t fileCnt)
{
    std::lock_guard<std::mutex> lock(mtx_);
    LOGE("OnRecvAssetError, sessionId = %{public}d, errorCode = %{public}d", socketId, errorCode);
    auto srcNetworkId = SoftBusHandlerAsset::GetInstance().GetClientInfo(socketId);
    if (srcNetworkId.empty()) {
        LOGE("get srcNetworkId fail");
        return;
    }
    sptr<AssetObj> assetObj (new (std::nothrow) AssetObj());
    if (assetObj == nullptr) {
        LOGE("new assetObj failed!");
        return;
    }
    if (fileCnt != 0) {
        std::string firstFilePath(path_ + fileList[0]);
        SoftBusHandlerAsset::GetInstance().GenerateAssetObjInfo(socketId, firstFilePath, assetObj);
    }

    AssetCallbackManager::GetInstance().NotifyAssetRecvFinished(srcNetworkId, assetObj, FileManagement::ERR_BAD_VALUE);
    SoftBusHandlerAsset::GetInstance().RemoveClientInfo(socketId);
}

void SoftbusAssetRecvListener::OnAssetRecvBind(int32_t sessionId, PeerSocketInfo info)
{
    LOGI("OnAssetRecvBind begin, sessionId %{public}d", sessionId);
    SoftBusHandlerAsset::GetInstance().OnAssetRecvBind(sessionId, info.networkId);
}

int32_t SoftbusAssetRecvListener::GetCurrentUserId()
{
    std::vector<int32_t> userIds{};
    auto ret = AccountSA::OsAccountManager::QueryActiveOsAccountIds(userIds);
    if (ret != NO_ERROR || userIds.empty()) {
        LOGE("query active os account id failed, ret = %{public}d", ret);
        return FileManagement::E_GET_USER_ID;
    }
    LOGI("GetCurrentUserId end.");
    return userIds[0];
}

bool SoftbusAssetRecvListener::MoveAsset(const std::vector<std::string> &fileList, bool isSingleFile)
{
    if (isSingleFile) {
        std::string oldPath = fileList[0];
        std::string newPath = oldPath;
        size_t pos = newPath.find(TEMP_DIR);
        if (pos == std::string::npos) {
            LOGE("get asset temp dir fail, file name is %{public}s", GetAnonyString(oldPath).c_str());
            return false;
        }
        newPath.replace(pos, TEMP_DIR.length(), "");
        pos = newPath.find(ASSET_FLAG_SINGLE);
        if (pos == std::string::npos) {
            LOGE("get asset flag fail, file name is %{public}s", GetAnonyString(oldPath).c_str());
            return false;
        }
        newPath.resize(pos);
        try {
            SoftBusHandlerAsset::GetInstance().MkDirRecurse(newPath, S_IRWXU | S_IRWXG | S_IXOTH);
            std::filesystem::rename(oldPath.c_str(), newPath.c_str());
        } catch (const std::filesystem::filesystem_error &e) {
            LOGE("rename file fail, file name is %{public}s", e.what());
            return false;
        }
        return true;
    }

    for (auto oldPath : fileList) {
        std::string newPath = oldPath;
        size_t pos = newPath.find(TEMP_DIR);
        if (pos == std::string::npos) {
            LOGE("get asset temp dir fail, file name is %{public}s", GetAnonyString(oldPath).c_str());
            return false;
        }
        newPath.replace(pos, TEMP_DIR.length(), "");
        try {
            SoftBusHandlerAsset::GetInstance().MkDirRecurse(newPath, S_IRWXU | S_IRWXG | S_IXOTH);
            std::filesystem::rename(oldPath.c_str(), newPath.c_str());
        } catch (const std::filesystem::filesystem_error &e) {
            LOGE("rename file fail, file name is %{public}s", e.what());
            return false;
        }
    }
    return true;
}

bool SoftbusAssetRecvListener::RemoveAsset(const std::string &file)
{
    size_t pos = file.find(TEMP_DIR);
    if (pos == std::string::npos) {
        LOGE("get asset temp dir fail, file name is %{public}s", GetAnonyString(file).c_str());
        return false;
    }
    std::string removePath = file.substr(0, pos + TEMP_DIR.length() - 1);
    bool ret = std::filesystem::remove_all(removePath.c_str());
    if (!ret) {
        LOGE("remove file fail, remove path is %{public}s", GetAnonyString(removePath).c_str());
        return false;
    }
    return true;
}

int32_t SoftbusAssetRecvListener::HandleSingleFile(int32_t socketId,
                                                   const std::string &filePath,
                                                   const sptr<AssetObj> &assetObj)
{
    LOGI("HandleSingleFile begin.");
    std::vector<std::string> fileList = {filePath};
    auto uris = SoftBusHandlerAsset::GetInstance().GenerateUris(fileList, assetObj->dstBundleName_, true);
    if (uris.size() == 0) {
        LOGE("Generate uris fail");
        return FileManagement::ERR_BAD_VALUE;
    }
    assetObj->uris_.emplace_back(uris[0]);

    bool moveRet = MoveAsset(fileList, true);
    if (!moveRet) {
        LOGE("MoveAsset fail, socket %{public}d", socketId);
        return FileManagement::ERR_BAD_VALUE;
    }
    LOGI("HandleOneFile end.");
    return FileManagement::ERR_OK;
}

int32_t SoftbusAssetRecvListener::HandleZipFile(int32_t socketId,
                                                const std::string &filePath,
                                                const sptr<AssetObj> &assetObj)
{
    LOGI("HandleZipFile begin.");
    size_t pos = filePath.find(ASSET_FLAG_ZIP);
    if (pos == std::string::npos) {
        LOGE("filePath is not a zip file : %{public}s", GetAnonyString(filePath).c_str());
        return FileManagement::ERR_BAD_VALUE;
    }
    std::string zipfilePath = filePath.substr(0, pos);
    try {
        std::filesystem::rename(filePath.c_str(), zipfilePath.c_str());
    } catch (const std::filesystem::filesystem_error &e) {
        LOGE("rename file fail, file name is %{public}s", e.what());
        return FileManagement::ERR_BAD_VALUE;
    }
    pos = zipfilePath.rfind("/");
    if (pos == std::string::npos) {
        LOGE("filePath is not a zip file : %{public}s", GetAnonyString(filePath).c_str());
        return FileManagement::ERR_BAD_VALUE;
    }
    std::string relativePath = zipfilePath.substr(0, pos + 1);
    std::vector<std::string> fileList = SoftBusHandlerAsset::GetInstance().DecompressFile(zipfilePath, relativePath);
    if (fileList.empty()) {
        LOGE("unzip fail");
        return FileManagement::ERR_BAD_VALUE;
    }

    auto uris = SoftBusHandlerAsset::GetInstance().GenerateUris(fileList, assetObj->dstBundleName_, false);
    if (uris.size() == 0) {
        LOGE("Generate uris fail");
        return FileManagement::ERR_BAD_VALUE;
    }
    assetObj->uris_.insert(assetObj->uris_.end(), uris.begin(), uris.end());

    bool moveRet = MoveAsset(fileList, false);
    if (!moveRet) {
        LOGE("MoveAsset fail, socket %{public}d", socketId);
        return FileManagement::ERR_BAD_VALUE;
    }
    LOGI("HandleMoreFile end.");
    return FileManagement::ERR_OK;
}

bool SoftbusAssetRecvListener::JudgeSingleFile(const std::string &filePath)
{
    size_t pos = filePath.find(ASSET_FLAG_SINGLE);
    if (pos == std::string::npos) {
        LOGI("this file is zip file");
        return false;
    }
    return true;
}

void SoftbusAssetRecvListener::DisConnectByAllConnect(const std::string &peerNetworkId)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto socketIds = SoftBusHandlerAsset::GetInstance().GetSocketIdFromClientInfo(peerNetworkId);
    for (auto socketId : socketIds) {
        auto assetObj = SoftBusHandlerAsset::GetInstance().GetAssetObj(socketId);
        if (assetObj == nullptr) {
            LOGE("OnSendAssetError  get assetObj is nullptr");
            continue;
        }
        AssetCallbackManager::GetInstance().NotifyAssetRecvFinished(
            peerNetworkId, assetObj, FileManagement::ERR_BAD_VALUE);
        Shutdown(socketId);
        SoftBusHandlerAsset::GetInstance().RemoveClientInfo(socketId);
    }
}

void SoftbusAssetRecvListener::OnRecvShutdown(int32_t sessionId, ShutdownReason reason)
{
    std::lock_guard<std::mutex> lock(mtx_);
    LOGI("OnSessionClosed, sessionId = %{public}d, reason = %{public}d", sessionId, reason);
    auto srcNetworkId = SoftBusHandlerAsset::GetInstance().GetClientInfo(sessionId);
    if (srcNetworkId.empty()) {
        LOGW("get srcNetworkId fail");
        return;
    }
    auto assetObj = SoftBusHandlerAsset::GetInstance().GetAssetObj(sessionId);
    if (assetObj == nullptr) {
        LOGW("get assetObj is nullptr");
        SoftBusHandlerAsset::GetInstance().RemoveClientInfo(sessionId);
        return;
    }
    AssetCallbackManager::GetInstance().NotifyAssetRecvFinished(
        assetObj->dstNetworkId_, assetObj, FileManagement::ERR_BAD_VALUE);
    SoftBusHandlerAsset::GetInstance().RemoveClientInfo(sessionId);
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS