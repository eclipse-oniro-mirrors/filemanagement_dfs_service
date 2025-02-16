/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "distributed_file_daemon_manager_impl.h"
#include "dm_device_info_ext.h"

#include "asset/asset_adapter_sa_client.h"
#include "copy/file_copy_manager.h"
#include "copy/file_size_utils.h"
#include "dfs_error.h"
#include "daemon_proxy.h"
#include "utils_log.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0xD001600
#define LOG_TAG "distributedfile_daemon"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
using namespace OHOS::Storage;
using namespace std;
DistributedFileDaemonManagerImpl &DistributedFileDaemonManagerImpl::GetInstance()
{
    static DistributedFileDaemonManagerImpl instance;
    return instance;
};

int32_t DistributedFileDaemonManagerImpl::OpenP2PConnection(const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    LOGI("DistributedFileDaemonManagerImpl::OpenP2PConnection, deviceInfo.networkId:%{public}s", deviceInfo.networkId);
    auto distributedFileDaemonProxy = DistributedFileDaemonManagerImpl::GetDaemonInterface();
    if (!distributedFileDaemonProxy) {
        LOGE("proxy is null");

        return OHOS::FileManagement::E_SA_LOAD_FAILED;
    }
    DmDeviceInfoExt deviceInfoExt(deviceInfo);
    return distributedFileDaemonProxy->OpenP2PConnection(deviceInfoExt);
}

int32_t DistributedFileDaemonManagerImpl::CloseP2PConnection(const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    LOGI("DistributedFileDaemonManagerImpl::CloseP2PConnection");
    LOGI("deviceInfo.networkId:%{public}s", deviceInfo.networkId);
    auto distributedFileDaemonProxy = DistributedFileDaemonManagerImpl::GetDaemonInterface();
    if (!distributedFileDaemonProxy) {
        LOGE("proxy is null");

        return OHOS::FileManagement::E_SA_LOAD_FAILED;
    }
    DmDeviceInfoExt deviceInfoExt(deviceInfo);
    return distributedFileDaemonProxy->CloseP2PConnection(deviceInfo);
}

int32_t DistributedFileDaemonManagerImpl::OpenP2PConnectionEx(const std::string &networkId,
                                                              sptr<IFileDfsListener> remoteReverseObj)
{
    LOGI("DistributedFileDaemonManagerImpl::OpenP2PConnectionEx");
    LOGI("networkId:%{public}s, flags:%{public}s",
         networkId.c_str(), (remoteReverseObj == nullptr) ? "NULL" : "Not NULL");
    auto distributedFileDaemonProxy = DistributedFileDaemonManagerImpl::GetDaemonInterface();
    if (!distributedFileDaemonProxy) {
        LOGE("proxy is null.");
        return OHOS::FileManagement::E_SA_LOAD_FAILED;
    }
    return distributedFileDaemonProxy->OpenP2PConnectionEx(networkId, remoteReverseObj);
}

int32_t DistributedFileDaemonManagerImpl::CloseP2PConnectionEx(const std::string &networkId)
{
    LOGI("DistributedFileDaemonManagerImpl::CloseP2PConnectionEx, networkId:%{public}s", networkId.c_str());
    auto distributedFileDaemonProxy = DistributedFileDaemonManagerImpl::GetDaemonInterface();
    if (!distributedFileDaemonProxy) {
        LOGE("proxy is null.");
        return OHOS::FileManagement::E_SA_LOAD_FAILED;
    }
    return distributedFileDaemonProxy->CloseP2PConnectionEx(networkId);
}

int32_t DistributedFileDaemonManagerImpl::PrepareSession(const std::string &srcUri,
                                                         const std::string &dstUri,
                                                         const std::string &srcDeviceId,
                                                         const sptr<IRemoteObject> &listener,
                                                         HmdfsInfo &info)
{
    LOGI("DistributedFileDaemonManagerImpl::PrepareSession");
    LOGI("srcUri:%{public}s", srcUri.c_str());
    LOGI("dstUri:%{public}s", dstUri.c_str());
    LOGI("srcDeviceId:%{public}s", srcDeviceId.c_str());
    LOGI("listener:%{public}s", (listener == nullptr) ? "NULL" : "Not NULL");
    LOGI("info.sessionName:%{public}s", info.sessionName.c_str());
    auto distributedFileDaemonProxy = DistributedFileDaemonManagerImpl::GetDaemonInterface();
    if (distributedFileDaemonProxy == nullptr) {
        LOGE("proxy is null");
        return OHOS::FileManagement::E_SA_LOAD_FAILED;
    }
    HmdfsInfoExt infoExt(info);
    auto ret = distributedFileDaemonProxy->PrepareSession(srcUri, dstUri, srcDeviceId, listener, infoExt);
    info = convertExttoInfo(infoExt);
    return ret;
}

int32_t DistributedFileDaemonManagerImpl::RequestSendFile(const std::string &srcUri,
                                                          const std::string &dstPath,
                                                          const std::string &remoteDeviceId,
                                                          const std::string &sessionName)
{
    LOGI("DistributedFileDaemonManagerImpl::RequestSendFile");
    LOGI("srcUri:%{public}s, dstPath:%{public}s, remoteDeviceId:%{public}s, sessionName:%{public}s", srcUri.c_str(),
         dstPath.c_str(), remoteDeviceId.c_str(), sessionName.c_str());
    auto distributedFileDaemonProxy = DistributedFileDaemonManagerImpl::GetDaemonInterface();
    if (distributedFileDaemonProxy == nullptr) {
        LOGE("proxy is null");
        return OHOS::FileManagement::E_SA_LOAD_FAILED;
    }
    return distributedFileDaemonProxy->RequestSendFile(srcUri, dstPath, remoteDeviceId, sessionName);
}

int32_t DistributedFileDaemonManagerImpl::GetRemoteCopyInfo(const std::string &srcUri, bool &isFile, bool &isDir)
{
    LOGI("DistributedFileDaemonManagerImpl::GetRemoteCopyInfo");
    LOGI("srcUri:%{public}s, isFile:%{public}s, isDir:%{public}s",
        srcUri.c_str(), isFile ? "true" : "false", isDir ? "true" : "false");
    auto distributedFileDaemonProxy = DistributedFileDaemonManagerImpl::GetDaemonInterface();
    if (distributedFileDaemonProxy == nullptr) {
        LOGE("proxy is null");
        return OHOS::FileManagement::E_SA_LOAD_FAILED;
    }
    isFile = false;
    isDir = false;
    return distributedFileDaemonProxy->GetRemoteCopyInfo(srcUri, isFile, isDir);
}

int32_t DistributedFileDaemonManagerImpl::CancelCopyTask(const std::string &sessionName)
{
    LOGI("DistributedFileDaemonManagerImpl::CancelCopyTask");
    LOGI("sessionName:%{public}s", sessionName.c_str());
    auto distributedFileDaemonProxy = DistributedFileDaemonManagerImpl::GetDaemonInterface();
    if (distributedFileDaemonProxy == nullptr) {
        LOGE("proxy is null");
        return OHOS::FileManagement::E_SA_LOAD_FAILED;
    }
    return distributedFileDaemonProxy->CancelCopyTask(sessionName);
}

int32_t DistributedFileDaemonManagerImpl::PushAsset(int32_t userId,
                                                    const sptr<AssetObj> &assetObj,
                                                    const sptr<IAssetSendCallback> &sendCallback)
{
    LOGI("DistributedFileDaemonManagerImpl PushAsset enter.");
    LOGI("userId:%{public}d, assetObj:%{public}s, sendCallback:%{public}s",
        userId,
        (assetObj == nullptr) ? "NULL" : "Not NULL",
        (sendCallback == nullptr) ? "NULL" : "Not NULL");
    auto distributedFileDaemonProxy = DistributedFileDaemonManagerImpl::GetDaemonInterface();
    if (distributedFileDaemonProxy == nullptr) {
        LOGE("proxy is null");
        return OHOS::FileManagement::E_SA_LOAD_FAILED;
    }
    return distributedFileDaemonProxy->PushAsset(userId, *assetObj, sendCallback);
}

int32_t DistributedFileDaemonManagerImpl::RegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback)
{
    LOGI("DistributedFileDaemonManagerImpl registerAssetCallback enter.");
    LOGI("recvCallback:%{public}s", (recvCallback == nullptr) ? "NULL" : "Not NULL");
    if (recvCallback == nullptr) {
        LOGE("Register IAssetRecvCallback is null.");
        return OHOS::FileManagement::E_NULLPTR;
    }
    int32_t ret = AssetAdapterSaClient::GetInstance().AddListener(recvCallback);
    if (ret != OHOS::FileManagement::E_OK) {
        LOGE("AssetAdapterSaClient addListener fail.");
        return ret;
    }
    return OHOS::FileManagement::E_OK;
}

int32_t DistributedFileDaemonManagerImpl::UnRegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback)
{
    LOGI("DistributedFileDaemonManagerImpl unRegisterAssetCallback enter.");
    LOGI("recvCallback:%{public}s", (recvCallback == nullptr) ? "NULL" : "Not NULL");
    if (recvCallback == nullptr) {
        LOGE("UnRegister IAssetRecvCallback is null.");
        return OHOS::FileManagement::E_NULLPTR;
    }
    int32_t ret = AssetAdapterSaClient::GetInstance().RemoveListener(recvCallback);
    if (ret != OHOS::FileManagement::E_OK) {
        LOGE("AssetAdapterSaClient removeListener fail.");
        return ret;
    }
    return OHOS::FileManagement::E_OK;
}

int32_t DistributedFileDaemonManagerImpl::GetSize(const std::string &uri, bool isSrcUri, uint64_t &size)
{
    return FileSizeUtils::GetSize(uri, isSrcUri, size);
}

int32_t DistributedFileDaemonManagerImpl::IsDirectory(const std::string &uri, bool isSrcUri, bool &isDirectory)
{
    return FileSizeUtils::IsDirectory(uri, isSrcUri, isDirectory);
}

int32_t DistributedFileDaemonManagerImpl::Copy(const std::string &srcUri,
    const std::string &destUri, ProcessCallback processCallback)
{
    return FileCopyManager::GetInstance()->Copy(srcUri, destUri, processCallback);
}

int32_t DistributedFileDaemonManagerImpl::Cancel(const std::string &srcUri, const std::string &destUri)
{
    return FileCopyManager::GetInstance()->Cancel(srcUri, destUri);
}

int32_t DistributedFileDaemonManagerImpl::Cancel()
{
    return FileCopyManager::GetInstance()->Cancel();
}

sptr<IDaemon> DistributedFileDaemonManagerImpl::DistributedFileDaemonManagerImpl::GetDaemonInterface()
{
    LOGI("daemon manager getinstance start");
    unique_lock<mutex> lock(proxyMutex_);
    if (daemonProxy_ != nullptr) {
    LOGI("GetDaemonInterface daemonProxy_ != nullptr");
        return daemonProxy_;
    }

    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        LOGE("Samgr is nullptr");
        return nullptr;
    }

    auto object = samgr->CheckSystemAbility(FILEMANAGEMENT_DISTRIBUTED_FILE_DAEMON_SA_ID);
    if (object == nullptr) {
        LOGE("object == nullptr");
        return nullptr;
    }

    if (deathRecipient_ == nullptr) {
        deathRecipient_ = new (std::nothrow) DaemonDeathRecipient();
        if (deathRecipient_ == nullptr) {
            LOGE("new death recipient failed");
            return nullptr;
        }
    }
    deathRecipient_->SetDeathRecipient([](const wptr<IRemoteObject> &remote) { OnRemoteSaDied(remote); });
    if ((object->IsProxyObject()) && (!object->AddDeathRecipient(deathRecipient_))) {
        LOGE("failed to add death recipient.");
        return nullptr;
    }

    daemonProxy_ = iface_cast<IDaemon>(object);
    if (daemonProxy_ == nullptr) {
        LOGE("service == nullptr");
        return nullptr;
    }
    return daemonProxy_;
}

void DistributedFileDaemonManagerImpl::OnRemoteSaDied(const wptr<IRemoteObject> &remote)
{
    LOGI("dfs service death");
    unique_lock<mutex> lock(proxyMutex_);
    daemonProxy_ = nullptr;
}

void DistributedFileDaemonManagerImpl::DaemonDeathRecipient::SetDeathRecipient(RemoteDiedHandler handler)
{
    handler_ = std::move(handler);
}

void DistributedFileDaemonManagerImpl::DaemonDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    LOGI("start");
    if (handler_ != nullptr) {
        handler_(remote);
    }
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
