/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "device_auth.h"
#include "device_info.h"
#include "device_manager.h"
#include "dfsu_actor.h"
#include "dfsu_singleton.h"
#include "dfsu_startable.h"
#include "mountpoint/mount_point.h"
#include "network/network_agent_template.h"
#include "nlohmann/json.hpp"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
struct GroupInfo {
    std::string groupName;
    std::string groupId;
    std::string groupOwner;
    int32_t groupType;
    GroupInfo() : groupType(0) {}
    GroupInfo(std::string name, std::string id, std::string owner, int32_t type)
        : groupName(name), groupId(id), groupOwner(owner), groupType(type)
    {
    }
};

void from_json(const nlohmann::json &jsonObject, GroupInfo &groupInfo);

class DeviceManagerAgent final : public DistributedHardware::DmInitCallback,
                                 public DistributedHardware::DeviceStateCallback,
                                 public std::enable_shared_from_this<DeviceManagerAgent>,
                                 public DfsuStartable,
                                 public DfsuActor<DeviceManagerAgent>,
                                 public Utils::DfsuSingleton<DeviceManagerAgent> {
    DECLARE_SINGLETON(DeviceManagerAgent);

public:
    enum P2PErrCode:int32_t {
        P2P_SUCCESS,
        P2P_FAILED,
    };
    void Start() override;
    void Stop() override;
    void JoinGroup(std::weak_ptr<MountPoint> mp);
    void QuitGroup(std::weak_ptr<MountPoint> mp);

    void InitDeviceInfos();

    void OnDeviceOnline(const DistributedHardware::DmDeviceInfo &deviceInfo) override;
    void OnDeviceOffline(const DistributedHardware::DmDeviceInfo &deviceInfo) override;
    void OnDeviceChanged(const DistributedHardware::DmDeviceInfo &deviceInfo) override;
    void OnDeviceReady(const DistributedHardware::DmDeviceInfo &deviceInfo) override {}

    int32_t OnDeviceP2POnline(const DistributedHardware::DmDeviceInfo &deviceInfo);
    int32_t OnDeviceP2POffline(const DistributedHardware::DmDeviceInfo &deviceInfo);

    void OfflineAllDevice();
    void ReconnectOnlineDevices();
    void OnRemoteDied() override;

    DeviceInfo &GetLocalDeviceInfo();
    std::vector<DeviceInfo> GetRemoteDevicesInfo();

private:
    void StartInstance() override;
    void StopInstance() override;
    void InitLocalNodeInfo();

    void RegisterToExternalDm();
    void UnregisterFromExternalDm();

    int32_t GetNetworkType(const std::string &cid);
    bool IsWifiNetworkType(int32_t networkType);

    void QueryRelatedGroups(const std::string &udid, const std::string &networkId, bool isP2P = false);
    bool CheckIsAccountless(const GroupInfo &group);
    bool NetworkIsTrusted(std::shared_ptr<NetworkAgentTemplate> net);
    std::shared_ptr<NetworkAgentTemplate> FindNetworkBaseTrustRelation(bool isAccountless, bool isP2P = false);
    // We use a mutex instead of a shared_mutex to serialize online/offline procedures
    std::mutex mpToNetworksMutex_;
    std::map<uintptr_t, std::shared_ptr<NetworkAgentTemplate>> mpToNetworks_;
    std::map<uintptr_t, std::shared_ptr<NetworkAgentTemplate>> mpToP2PNetworks_;
    DeviceInfo localDeviceInfo_;

    // cid-->same_account/accoutless's network
    std::unordered_map<std::string, std::shared_ptr<NetworkAgentTemplate>> cidNetTypeRecord_;
    std::unordered_map<std::string, int32_t> cidNetworkType_;
    std::unordered_map<std::string, std::shared_ptr<NetworkAgentTemplate>> cidP2PNetTypeRecord_;
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
#endif // DEVICE_MANAGER_AGENT_H