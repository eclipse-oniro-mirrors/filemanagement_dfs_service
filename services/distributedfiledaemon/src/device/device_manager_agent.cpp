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

#include "device/device_manager_agent.h"
#include "network/softbus/softbus_agent.h"

#include <limits>
#include <sstream>
#include <string>

#include "ipc/i_daemon.h"
#include "softbus_bus_center.h"
#include "utils_exception.h"
#include "utils_log.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
namespace {
constexpr int MAX_RETRY_COUNT = 7;
}
using namespace std;

DeviceManagerAgent::DeviceManagerAgent() : Actor<DeviceManagerAgent>(this, std::numeric_limits<uint32_t>::max()) {}

DeviceManagerAgent::~DeviceManagerAgent()
{
    StopInstance();
}

void DeviceManagerAgent::StartInstance()
{
    StartActor();
}

void DeviceManagerAgent::StopInstance()
{
    StopActor();
}

void DeviceManagerAgent::Start()
{
    RegisterToExternalDm();
    InitLocalNodeInfo();
}

void DeviceManagerAgent::Stop()
{
    UnregisterFromExternalDm();
}

void DeviceManagerAgent::JoinGroup(weak_ptr<MountPoint> mp)
{
    auto smp = mp.lock();
    if (!smp) {
        stringstream ss("Failed to join group: Received empty mountpoint");
        LOGE("%{public}s", ss.str().c_str());
        throw runtime_error(ss.str());
    }

    unique_lock<mutex> lock(mpToNetworksMutex_);
    auto agent = make_shared<SoftbusAgent>(mp);
    auto [ignored, inserted] = mpToNetworks_.insert({smp->GetID(), agent});
    if (!inserted) {
        stringstream ss;
        ss << "Failed to join group: Mountpoint existed" << smp->ToString();
        throw runtime_error(ss.str());
    }

    agent->StartActor();
}

void DeviceManagerAgent::QuitGroup(weak_ptr<MountPoint> mp)
{
    auto smp = mp.lock();
    if (!smp) {
        stringstream ss("Failed to quit group: Received empty mountpoint");
        LOGE("%{public}s", ss.str().c_str());
        throw runtime_error(ss.str());
    }

    unique_lock<mutex> lock(mpToNetworksMutex_);
    auto it = mpToNetworks_.find(smp->GetID());
    if (it == mpToNetworks_.end()) {
        stringstream ss;
        ss << "Failed to quit group: Mountpoint didn't exist " << smp->ToString();
        throw runtime_error(ss.str());
    }

    it->second->StopActor();
    mpToNetworks_.erase(smp->GetID());
}

void DeviceManagerAgent::OfflineAllDevice()
{
    unique_lock<mutex> lock(mpToNetworksMutex_);
    for (auto &&networkAgent : mpToNetworks_) {
        auto cmd = make_unique<Cmd<NetworkAgentTemplate>>(&NetworkAgentTemplate::DisconnectAllDevices);
        cmd->UpdateOption({
            .tryTimes_ = 1,
        });
        networkAgent.second->Recv(move(cmd));
    }
}

void DeviceManagerAgent::ReconnectOnlineDevices()
{
    unique_lock<mutex> lock(mpToNetworksMutex_);
    for (auto &&networkAgent : mpToNetworks_) {
        auto cmd = make_unique<Cmd<NetworkAgentTemplate>>(&NetworkAgentTemplate::ConnectOnlineDevices);
        cmd->UpdateOption({
            .tryTimes_ = MAX_RETRY_COUNT,
        });
        networkAgent.second->Recv(move(cmd));
    }
}

void DeviceManagerAgent::OnDeviceOnline(const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    LOGI("OnDeviceOnline begin");
    DeviceInfo info(deviceInfo);
    unique_lock<mutex> lock(mpToNetworksMutex_);
    for (auto &&networkAgent : mpToNetworks_) {
        auto cmd =
            make_unique<Cmd<NetworkAgentTemplate, const DeviceInfo>>(&NetworkAgentTemplate::ConnectDeviceAsync, info);
        cmd->UpdateOption({
            .tryTimes_ = MAX_RETRY_COUNT,
        });
        networkAgent.second->Recv(move(cmd));
    }
    LOGI("OnDeviceOnline end");
}

void DeviceManagerAgent::OnDeviceOffline(const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    LOGI("OnDeviceOffline begin");
    DeviceInfo info(deviceInfo);
    unique_lock<mutex> lock(mpToNetworksMutex_);
    for (auto &&networkAgent : mpToNetworks_) {
        auto cmd =
            make_unique<Cmd<NetworkAgentTemplate, const DeviceInfo>>(&NetworkAgentTemplate::DisconnectDevice, info);
        cmd->UpdateOption({
            .tryTimes_ = 1,
        });
        networkAgent.second->Recv(move(cmd));
    }
    LOGI("OnDeviceOffline end");
}

void DeviceManagerAgent::OnDeviceChanged(const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    LOGI("OnDeviceInfoChanged");
}

void DeviceManagerAgent::InitLocalNodeInfo()
{
    NodeBasicInfo tmpNodeInfo;
    int errCode = GetLocalNodeDeviceInfo(IDaemon::SERVICE_NAME.c_str(), &tmpNodeInfo);
    if (errCode != 0) {
        ThrowException(errCode, "Failed to get info of local devices");
    }
    localDeviceInfo_.SetCid(string(tmpNodeInfo.networkId));
}

void DeviceManagerAgent::OnRemoteDied()
{
    LOGI("device manager service died");
    StopInstance();
    OfflineAllDevice(); // cannot commit a cmd to queue
    StartInstance();
}

DeviceInfo &DeviceManagerAgent::GetLocalDeviceInfo()
{
    return localDeviceInfo_;
}

vector<DeviceInfo> DeviceManagerAgent::GetRemoteDevicesInfo()
{
    string extra = "";
    string pkgName = IDaemon::SERVICE_NAME;
    vector<DistributedHardware::DmDeviceInfo> deviceList;

    auto &deviceManager = DistributedHardware::DeviceManager::GetInstance();
    int errCode = deviceManager.GetTrustedDeviceList(pkgName, extra, deviceList);
    if (errCode) {
        ThrowException(errCode, "Failed to get info of remote devices");
    }

    vector<DeviceInfo> res;
    for (const auto &item : deviceList) {
        res.push_back(DeviceInfo(item));
    }
    return res;
}

void DeviceManagerAgent::RegisterToExternalDm()
{
    auto &deviceManager = DistributedHardware::DeviceManager::GetInstance();
    string pkgName = IDaemon::SERVICE_NAME;
    int errCode = deviceManager.InitDeviceManager(pkgName, shared_from_this());
    if (errCode != 0) {
        ThrowException(errCode, "Failed to InitDeviceManager");
    }
    string extra = "";
    errCode = deviceManager.RegisterDevStateCallback(pkgName, extra, shared_from_this());
    if (errCode != 0) {
        ThrowException(errCode, "Failed to RegisterDevStateCallback");
    }
    LOGI("RegisterToExternalDm Succeed");
}

void DeviceManagerAgent::UnregisterFromExternalDm()
{
    auto &deviceManager = DistributedHardware::DeviceManager::GetInstance();
    string pkgName = IDaemon::SERVICE_NAME;
    int errCode = deviceManager.UnRegisterDevStateCallback(pkgName);
    if (errCode != 0) {
        ThrowException(errCode, "Failed to UnRegisterDevStateCallback");
    }
    errCode = deviceManager.UnInitDeviceManager(pkgName);
    if (errCode != 0) {
        ThrowException(errCode, "Failed to UnInitDeviceManager");
    }
    LOGI("UnregisterFromExternalDm Succeed");
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS