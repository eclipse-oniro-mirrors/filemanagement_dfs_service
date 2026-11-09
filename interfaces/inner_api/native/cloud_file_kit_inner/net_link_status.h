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

#ifndef OHOS_NETWORK_LINK_STATUS_LISTENER_H
#define OHOS_NETWORK_LINK_STATUS_LISTENER_H

#include <atomic>
#include <cstdint>
#include <dlfcn.h>

namespace OHOS::FileManagement::CloudSync {

enum NetMgrClientNetworkLinkStatusType {
    NET_NORMAL = 0,
    WIFI_AIRPLANE = 1,
    CELL_CONGESTION = 2
};

static constexpr int COMM_NET_ENHANCED_SYS_ABILITY_ID = 1181;


class NetworkLinkStatus {
public:
    static void InitNetwork();
    static bool IsAirPlaneMode();

    typedef int (*GetStatusFunc)(int &type);
    static inline GetStatusFunc getCurrentStatus = nullptr;

private:
    static int32_t GetCurrentNetStatus();
    static int64_t GetRelativeTimeMillis();

    static constexpr int64_t NETWORK_STATUS_CHECK_INTERVAL = 10000;
    static constexpr const char *CONFIG_DATA_SERVICE_EXT_PATH = "/system/lib64/libnetmanager_enhanced_service_if.z.so";

    static inline std::atomic<NetMgrClientNetworkLinkStatusType> netStatus_{NET_NORMAL};
    static inline void *netMgrLibHandle_ = nullptr;
    static inline std::atomic<int64_t> lastCheckTime_;
};
} // namespace OHOS::FileManagement::CloudSync
#endif
