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
#pragma once

#include <atomic>
#include <string>

#include "dm_device_info.h"

namespace OHOS {
namespace DistributedFile {
class DeviceInfo final {
public:
    DeviceInfo() = default;
    ~DeviceInfo() = default;
    explicit DeviceInfo(const DistributedHardware::DmDeviceInfo &nodeInfo);
    explicit DeviceInfo(const DeviceInfo &nodeInfo);
    DeviceInfo &operator=(const DistributedHardware::DmDeviceInfo &nodeInfo);

    /**
     * @note Remove the concept iid later
     */
    void SetIid(uint64_t iid);
    void SetCid(const std::string cid);

    uint64_t GetIid() const;
    const std::string &GetCid() const;

private:
    std::atomic<bool> initIidFlag_{false};
    std::atomic<bool> initCidFlag_{false};

    uint64_t iid_{0};
    std::string cid_;
};
} // namespace DistributedFile
} // namespace OHOS