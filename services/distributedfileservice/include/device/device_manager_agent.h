/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef DEVICE_MANAGER_AGENT_H
#define DEVICE_MANAGER_AGENT_H

#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "device_info.h"
#include "device_manager.h"
#include "network/network_agent_template.h"
#include "nlohmann/json.hpp"
#include "utils_singleton.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
class DeviceManagerAgent final : public DistributedHardware::DmInitCallback,
                                 public DistributedHardware::DeviceStateCallback,
                                 public std::enable_shared_from_this<DeviceManagerAgent>,
                                 public Utils::Singleton<DeviceManagerAgent> {
    DECLARE_SINGLETON(DeviceManagerAgent);

public:
    void Start() override;
    void Stop() override;
    void JoinGroup(std::weak_ptr<MountPoint> mp);
    void QuitGroup(std::weak_ptr<MountPoint> mp);

    void OnDeviceOnline(const DistributedHardware::DmDeviceInfo &deviceInfo) override;
    void OnDeviceOffline(const DistributedHardware::DmDeviceInfo &deviceInfo) override;
    void OnDeviceChanged(const DistributedHardware::DmDeviceInfo &deviceInfo) override;
    void OnDeviceReady(const DistributedHardware::DmDeviceInfo &deviceInfo) override {}

    void OfflineAllDevice();
    void ReconnectOnlineDevices();
    void AuthGroupOnlineProc(const DeviceInfo info);
    void OnRemoteDied() override;

    DeviceInfo &GetLocalDeviceInfo();
    std::vector<DeviceInfo> GetRemoteDevicesInfo();

private:
    void StartInstance() override;
    void StopInstance() override;

    void RegisterToExternalDm();
    void UnregisterFromExternalDm();

    void AuthGroupOfflineProc(const DeviceInfo &info);
    void QueryRelatedGroups(const std::string &networkId, std::vector<GroupInfo> &groupList);
    bool CheckIsAuthGroup(const GroupInfo &group);
    void AllAuthGroupsOfflineProc();
    // We use a mutex instead of a shared_mutex to serialize online/offline procedures
    std::mutex mpToNetworksMutex_;
    std::map<uintptr_t, std::shared_ptr<NetworkAgentTemplate>> mpToNetworks_;
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
#endif // DEVICE_MANAGER_AGENT_H