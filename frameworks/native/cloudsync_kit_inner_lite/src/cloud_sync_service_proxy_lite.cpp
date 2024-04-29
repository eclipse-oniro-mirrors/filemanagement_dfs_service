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
#include "cloud_sync_service_proxy_lite.h"

#include <sstream>

#include "cloud_file_sync_service_interface_code.h"
#include "dfs_error.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {

constexpr int LOAD_SA_TIMEOUT_MS = 4000;

int32_t CloudSyncServiceProxyLite::TriggerSyncInner(const std::string &bundleName, const int32_t &userId)
{
    LOGI("Trigger Sync");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOGE("Failed to write interface token");
        return E_BROKEN_IPC;
    }

    if (!data.WriteString(bundleName)) {
        LOGE("Failed to send the bundle name");
        return E_INVAL_ARG;
    }

    if (!data.WriteInt32(userId)) {
        LOGE("Failed to send the user id");
        return E_INVAL_ARG;
    }

    auto remote = Remote();
    if (!remote) {
        LOGE("remote is nullptr");
        return E_BROKEN_IPC;
    }

    int32_t ret = remote->SendRequest(
        static_cast<uint32_t> (CloudFileSyncServiceInterfaceCode::SERVICE_CMD_TRIGGER_SYNC),
        data, reply, option);
    if (ret != E_OK) {
        LOGE("Failed to send out the request, errno: %{public}d", ret);
        return E_BROKEN_IPC;
    }
    LOGI("TriggerSyncInner Success");
    return reply.ReadInt32();
}

sptr<ICloudSyncServiceLite> CloudSyncServiceProxyLite::GetInstance()
{
    LOGI("GetInstance");
    std::unique_lock<std::mutex> lock(instanceMutex_);
    if (serviceProxy_ != nullptr) {
        return serviceProxy_;
    }

    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        LOGE("Samgr is nullptr");
        return nullptr;
    }
    sptr<ServiceProxyLoadCallbackLite> cloudSyncLoadCallback = new ServiceProxyLoadCallbackLite();
    if (cloudSyncLoadCallback == nullptr) {
        LOGE("cloudSyncLoadCallback is nullptr");
        return nullptr;
    }
    int32_t ret = samgr->LoadSystemAbility(FILEMANAGEMENT_CLOUD_SYNC_SERVICE_SA_ID, cloudSyncLoadCallback);
    if (ret != E_OK) {
        LOGE("Failed to load System Ability, systemAbilityId: %{public}d, ret code: %{public}d",
            FILEMANAGEMENT_CLOUD_SYNC_SERVICE_SA_ID, ret);
        return nullptr;
    }
    std::unique_lock<std::mutex> proxyLock(proxyMutex_);
    auto waitStatus = cloudSyncLoadCallback->proxyConVar_.wait_for(
        proxyLock, std::chrono::milliseconds(LOAD_SA_TIMEOUT_MS),
        [cloudSyncLoadCallback]() { return cloudSyncLoadCallback->isLoadSuccess_.load(); });
    if (!waitStatus) {
        LOGE("Load CloudSync SA timeout");
        return nullptr;
    }
    return serviceProxy_;
}

void CloudSyncServiceProxyLite::InvalidInstance()
{
    LOGI("Invalid Instance");
    std::unique_lock<std::mutex> lock(instanceMutex_);
    serviceProxy_ = nullptr;
}

void CloudSyncServiceProxyLite::ServiceProxyLoadCallbackLite::OnLoadSystemAbilitySuccess(
    int32_t systemAbilityId,
    const sptr<IRemoteObject> &remoteObject) __attribute__((no_sanitize("cfi")))
{
    LOGI("Load CloudSync SA success, systemAbilityId: %{public}d, remote Obj result: %{private}s",
        systemAbilityId, (remoteObject == nullptr ? "false" : "true"));
    std::unique_lock<std::mutex> lock(proxyMutex_);
    if (serviceProxy_ != nullptr) {
        LOGE("CloudSync SA proxy has been loaded");
    } else {
        serviceProxy_ = iface_cast<ICloudSyncServiceLite>(remoteObject);
    }
    isLoadSuccess_.store(true);
    proxyConVar_.notify_one();
}

void CloudSyncServiceProxyLite::ServiceProxyLoadCallbackLite::OnLoadSystemAbilityFail(
    int32_t systemAbilityId)
{
    LOGI("Load CloudSync SA failed, systemAbilityId: %{public}d", systemAbilityId);
    std::unique_lock<std::mutex> lock(proxyMutex_);
    serviceProxy_ = nullptr;
    isLoadSuccess_.store(false);
    proxyConVar_.notify_one();
}
} // namespace OHOS::FileManagement::CloudSync
