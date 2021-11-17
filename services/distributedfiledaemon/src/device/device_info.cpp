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

#include "device/device_info.h"
#include "utils_log.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
using namespace std;

DeviceInfo::DeviceInfo(const DistributedHardware::DmDeviceInfo &nodeInfo)
{
    cid_ = string(nodeInfo.deviceId);
}

DeviceInfo &DeviceInfo::operator=(const DistributedHardware::DmDeviceInfo &nodeInfo)
{
    cid_ = string(nodeInfo.deviceId);
    return *this;
}

DeviceInfo::DeviceInfo(const DeviceInfo &nodeInfo) : iid_(nodeInfo.iid_), cid_(nodeInfo.cid_)
{
    initIidFlag_.store(nodeInfo.initIidFlag_.load());
    initCidFlag_.store(nodeInfo.initCidFlag_.load());
}

void DeviceInfo::SetIid(const uint64_t iid)
{
    if (initIidFlag_ == false) {
        iid_ = iid;
        initIidFlag_ = true;
    } else {
        LOGI("Iid is already initializing");
    }
}

void DeviceInfo::SetCid(const string &cid)
{
    if (initCidFlag_ == false) {
        cid_ = cid;
        initCidFlag_ = true;
    } else {
        LOGI("Cid is already initializing");
    }
}

uint64_t DeviceInfo::GetIid() const
{
    if (initIidFlag_ == false) {
        // TODO 抛异常
    }
    return iid_;
}
const string &DeviceInfo::GetCid() const
{
    if (initCidFlag_ == false) {
        // TODO 抛异常
    }
    return cid_;
}
} // namespace DistributedFile
} // namespace Storages
} // namespace OHOS