/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "net_link_status.h"
#include "dfs_error.h"
#include "utils_log.h"
#include <unistd.h>
#include <chrono>

namespace OHOS::FileManagement::CloudSync {

int32_t NetworkLinkStatus::GetCurrentNetStatus()
{
    if (getCurrentStatus == nullptr) {
        return E_GET_NETWORK_MANAGER_FAILED;
    }
    int type = 0;
    auto ret = getCurrentStatus(type);
    if (ret) {
        LOGE("dfs_service GetNetworkLinkStatusType failed, ret = %{public}d", ret);
        return E_GET_NETWORK_MANAGER_FAILED;
    }
    netStatus_.store(static_cast<NetMgrClientNetworkLinkStatusType>(type));
    return E_OK;
}

void NetworkLinkStatus::InitNetwork()
{
    if (netMgrLibHandle_ != nullptr) {
        return;
    }

    netMgrLibHandle_ = dlopen(CONFIG_DATA_SERVICE_EXT_PATH, RTLD_LAZY);
    if (netMgrLibHandle_ == nullptr) {
        LOGE("dfs_service dlopen netmanager_enhanced_service_if.z.so failed: %{public}s", dlerror());
        return;
    }

    getCurrentStatus = (GetStatusFunc)dlsym(netMgrLibHandle_, "GetNetworkLinkStatusType");
    if (getCurrentStatus == nullptr) {
        LOGE("dfs_service dlsym GetStatusFunc failed: %{public}s", dlerror());
        dlclose(netMgrLibHandle_);
        netMgrLibHandle_ = nullptr;
        return;
    }

    lastCheckTime_.store(0);
    LOGD("dfs_service Network Link Status Listener Init Success!");
}

int64_t NetworkLinkStatus::GetRelativeTimeMillis()
{
    auto duration = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

bool NetworkLinkStatus::IsAirPlaneMode()
{
    auto currentRelativeTime = GetRelativeTimeMillis();
    if (currentRelativeTime > lastCheckTime_.load() + NETWORK_STATUS_CHECK_INTERVAL) {
        lastCheckTime_.store(currentRelativeTime);
        if (GetCurrentNetStatus() != E_OK) {
            return false;
        }
    }
    return netStatus_.load() == WIFI_AIRPLANE;
}
} // namespace OHOS::FileManagement::CloudSync
