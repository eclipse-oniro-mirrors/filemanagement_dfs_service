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
#ifndef DFS_DEVICE_MANAGER_AGENT_H
#define DFS_DEVICE_MANAGER_AGENT_H

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

#include "device_manager.h"
#include "utils_singleton.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
class DeviceManagerAgent : public DistributedHardware::DmInitCallback,
                                 public DistributedHardware::DeviceStateCallback,
                                 public std::enable_shared_from_this<DeviceManagerAgent>,
                                 public Utils::Singleton<DeviceManagerAgent> {
    DECLARE_SINGLETON(DeviceManagerAgent);

public:
    void OnDeviceOnline(const DistributedHardware::DmDeviceInfo &deviceInfo)
        override; // 加入set，创建softbus 各种监听，需要标志位，注册过就不需要重复注册
    void OnDeviceOffline(
        const DistributedHardware::DmDeviceInfo &deviceInfo) override; // 从set中删除此cid, 若set为空则解注册softbus
    void OnDeviceChanged(const DistributedHardware::DmDeviceInfo &deviceInfo) override {}
    void OnDeviceReady(const DistributedHardware::DmDeviceInfo &deviceInfo) override {}
    void OfflineAllDevice();
    void OnRemoteDied() override;
    std::set<std::string> getOnlineDevs() const 
    {
        return alreadyOnlineDev_;
    }
    // std::vector<std::string> GetRemoteDevicesInfo();

private:
    void StartInstance() override; // 注册dm监听
    void StopInstance() override;

    void RegisterToExternalDm();
    void UnregisterFromExternalDm();

    std::atomic<bool> alreadyRegis_{false};
    std::set<std::string> alreadyOnlineDev_;

    std::string pkgName_{"ohos.storage.distributedfile.service"};
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
#endif // DFS_DEVICE_MANAGER_AGENT_H