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

#include <sstream>
#include <string>

#include "ipc/i_deamon.h"
#include "softbus_bus_center.h"
#include "utils_log.h"

namespace OHOS {
namespace DistributedFile {
using namespace std;

DeviceManagerAgent::DeviceManagerAgent() {}

DeviceManagerAgent::~DeviceManagerAgent()
{
    StopInstance();
}

void DeviceManagerAgent::Start()
{
    RegisterToExternalDm(); // TODO  Catch?
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
        LOGE("%s", ss.str().c_str());
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

    agent->Start();
}

void DeviceManagerAgent::QuitGroup(weak_ptr<MountPoint> mp)
{
    auto smp = mp.lock();
    if (!smp) {
        stringstream ss("Failed to quit group: Received empty mountpoint");
        LOGE("%s", ss.str().c_str());
        throw runtime_error(ss.str());
    }

    unique_lock<mutex> lock(mpToNetworksMutex_);
    auto it = mpToNetworks_.find(smp->GetID());
    if (it == mpToNetworks_.end()) {
        stringstream ss;
        ss << "Failed to quit group: Mountpoint didn't exist " << smp->ToString();
        throw runtime_error(ss.str());
    }

    it->second->Stop();
    mpToNetworks_.erase(smp->GetID());
}

void DeviceManagerAgent::OnDeviceOnline(const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    LOGI("OnDeviceOnline begin");
    auto dm = DeviceManagerAgent::GetInstance();
    unique_lock<mutex> lock(dm->mpToNetworksMutex_);
    for (auto &&networkAgent : dm->mpToNetworks_) {
        DeviceInfo info(deviceInfo);
        networkAgent.second->ConnectDeviceAsync(info);
    }
    LOGI("OnDeviceOnline end");
}

void DeviceManagerAgent::OnDeviceOffline(const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    LOGI("OnDeviceOffline begin");
    auto dm = DeviceManagerAgent::GetInstance();

    unique_lock<mutex> lock(dm->mpToNetworksMutex_);
    for (auto &&networkAgent : dm->mpToNetworks_) {
        DeviceInfo info(deviceInfo);
        networkAgent.second->DisconnectDevice(info);
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
    int errCode = GetLocalNodeDeviceInfo(IDeamon::SERVICE_NAME.c_str(), &tmpNodeInfo);
    if (errCode != 0) {
        stringstream ss;
        ss << "Failed to get local cid: error code reads " << errCode;
        LOGW("%s", ss.str().c_str());
        throw runtime_error(ss.str());
    }
    localDeviceInfo_.SetCid(string(tmpNodeInfo.networkId));
    InitLocalIid();
}

void DeviceManagerAgent::OnRemoteDied()
{
    LOGI("device manager service died");
    RegisterToExternalDm(); // ! TODO
}

void DeviceManagerAgent::InitLocalIid()
{
    localDeviceInfo_.SetIid(0x12345678); // TODO 随机产生
}

DeviceInfo &DeviceManagerAgent::GetLocalDeviceInfo()
{
    return localDeviceInfo_;
}

vector<DeviceInfo> DeviceManagerAgent::GetRemoteDevicesInfo()
{
    string extra = "";
    string pkgName = IDeamon::SERVICE_NAME;
    vector<DistributedHardware::DmDeviceInfo> deviceList;

    auto &deviceManager = DistributedHardware::DeviceManager::GetInstance();
    int errCode = deviceManager.GetTrustedDeviceList(pkgName, extra, deviceList);
    if (errCode) {
        stringstream ss;
        ss << "Failed to get info of remote devices: the error code reads " << errCode;
        LOGE("%s", ss.str().c_str());
        throw runtime_error(ss.str());
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
    string pkgName = IDeamon::SERVICE_NAME;
    int errCode = deviceManager.InitDeviceManager(pkgName, shared_from_this());
    if (errCode) {
        stringstream ss;
        ss << "Failed to InitDeviceManager: the error code reads " << errCode;
        LOGE("%s", ss.str().c_str());
        throw runtime_error(ss.str());
    }
    string extra = "";
    errCode = deviceManager.RegisterDevStateCallback(pkgName, extra, shared_from_this());
    if (errCode) {
        stringstream ss;
        ss << "Failed to RegisterDevStateCallback: the error code reads " << errCode;
        LOGE("%s", ss.str().c_str());
        throw runtime_error(ss.str());
    }
}

void DeviceManagerAgent::UnregisterFromExternalDm()
{
    auto &deviceManager = DistributedHardware::DeviceManager::GetInstance();
    string pkgName = IDeamon::SERVICE_NAME;
    int errCode = deviceManager.UnRegisterDevStateCallback(pkgName);
    if (errCode) {
        stringstream ss;
        ss << "Failed to UnRegisterDevStateCallback: the error code reads " << errCode;
        LOGE("%s", ss.str().c_str());
        throw runtime_error(ss.str());
    }
    errCode = deviceManager.UnInitDeviceManager(pkgName);
    if (errCode) {
        stringstream ss;
        ss << "Failed to UnInitDeviceManager: the error code reads " << errCode;
        LOGE("%s", ss.str().c_str());
        throw runtime_error(ss.str());
    }
}
} // namespace DistributedFile
} // namespace OHOS