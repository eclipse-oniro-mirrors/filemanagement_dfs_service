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

#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include "utils_dfs_thread.h"
#include "device/device_info.h"
#include "mountpoint/mount_point.h"
#include "network/kernel_talker.h"
#include "network/session_pool.h"

namespace OHOS {
namespace DistributedFile {
class NetworkAgentTemplate {
public:
    explicit NetworkAgentTemplate(std::weak_ptr<MountPoint> mountPoint)
        : mountPoint_(mountPoint),
          kernerlTalker_(mountPoint, [&](NotifyParam &param) { GetSessionProcess(param); }),
          sessionPool_(kernerlTalker_)
    {
    }
    virtual ~NetworkAgentTemplate();
    void Start();
    void Stop();
    void ConnectOnlineDevices();
    void ConnectDeviceAsync(const DeviceInfo &info);
    void DisconnectDevice(const DeviceInfo &info);
    void AcceptSession(std::shared_ptr<BaseSession> session);

protected:
    virtual void JoinDomain() = 0;
    virtual void QuitDomain() = 0;
    virtual void StopTopHalf() = 0;
    virtual void StopBottomHalf() = 0;
    virtual std::shared_ptr<BaseSession> OpenSession(const DeviceInfo &info) = 0;
    virtual void CloseSession(std::shared_ptr<BaseSession> session) = 0;

    std::weak_ptr<MountPoint> mountPoint_;

private:
    void HandleAllNotify(int fd);
    void NotifyHandler(NotifyParam &param);
    void GetSessionProcess(NotifyParam &param);
    void GetSesion(const std::string &cid);

    std::mutex taskMut_;
    std::list<Utils::DfsThread> tasks_;
    KernelTalker kernerlTalker_;
    SessionPool sessionPool_;
};
} // namespace DistributedFile
} // namespace OHOS