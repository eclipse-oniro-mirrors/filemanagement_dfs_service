/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef NETWORK_AGENT_TEMPLATE_H
#define NETWORK_AGENT_TEMPLATE_H

#include <list>
#include <memory>
#include <mutex>
#include <vector>
#include <string_view>

#include "device/device_info.h"
#include "dfsu_actor.h"
#include "dfsu_startable.h"
#include "dfsu_thread.h"
#include "mountpoint/mount_point.h"
#include "network/kernel_talker.h"
#include "network/session_pool.h"
#include "network/softbus/softbus_session_dispatcher.h"
#include "nlohmann/json.hpp"
#include "utils_directory.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
constexpr uint8_t LINK_TYPE_AP = 1;
constexpr uint8_t LINK_TYPE_P2P = 2;
constexpr std::string_view GROUP_TYPE_AP = "hmdfs_WifiGroup";
constexpr std::string_view GROUP_TYPE_P2P = "hmdfs_P2PGroup";

class NetworkAgentTemplate : public DfsuStartable, public DfsuActor<NetworkAgentTemplate> {
public:
    explicit NetworkAgentTemplate(std::weak_ptr<MountPoint> mountPoint)
        : DfsuActor<NetworkAgentTemplate>(this),
          mountPoint_(mountPoint),
          kernerlTalker_(std::make_shared<KernelTalker>(
              mountPoint,
              [&](NotifyParam &param) { GetSessionProcess(param); },
              [&](const std::string &cid) { CloseSessionForOneDevice(cid); })),
          sessionPool_(kernerlTalker_)
    {
    }
    virtual ~NetworkAgentTemplate() {}
    void Start();
    void Stop();
    void ConnectOnlineDevices();
    void DisconnectAllDevices();
    void DisconnectDeviceByP2P(const std::string networkId);
    void DisconnectDeviceByP2PHmdfs(const std::string networkId);
    void AcceptSession(std::shared_ptr<BaseSession> session, const std::string backStage);
    void ConnectDeviceByP2PAsync(const DeviceInfo info);
    bool FindSocketId(int32_t socketId);
    std::shared_ptr<MountPoint> GetMountPoint()
    {
        return mountPoint_.lock();
    };
protected:
    virtual void JoinDomain() = 0;
    virtual void QuitDomain() = 0;
    virtual void StopTopHalf() = 0;
    virtual void StopBottomHalf() = 0;
    virtual int32_t OpenSession(const DeviceInfo &info, const uint8_t &linkType) = 0;
    virtual void CloseSession(std::shared_ptr<BaseSession> session) = 0;
    void CloseSessionForOneDevice(const std::string &cid);
    std::weak_ptr<MountPoint> mountPoint_;

private:
    void HandleAllNotify(int fd);
    void NotifyHandler(NotifyParam &param);
    void GetSessionProcess(NotifyParam &param);
    void GetSession(const std::string &cid);
    
    void CloseSessionForOneDeviceInner(std::string cid);
    void GetSessionProcessInner(NotifyParam param);

    std::mutex taskMut_;
    std::list<Utils::DfsuThread> tasks_;
    std::shared_ptr<KernelTalker> kernerlTalker_;
    SessionPool sessionPool_;
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
#endif // NETWORK_AGENT_TEMPLATE_H