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

#include "network/network_agent_template.h"
#include "device/device_manager_agent.h"
#include "utils_exception.h"
#include "utils_log.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
using namespace std;
namespace {
constexpr int MAX_RETRY_COUNT = 7;
}

void NetworkAgentTemplate::Start()
{
    JoinDomain();
    ConnectOnlineDevices();
    kernerlTalker_.CreatePollThread();
}

void NetworkAgentTemplate::Stop()
{
    StopTopHalf();
    StopBottomHalf();
    kernerlTalker_.WaitForPollThreadExited();
}

void NetworkAgentTemplate::ConnectDeviceAsync(const DeviceInfo info)
{
    kernerlTalker_.SinkInitCmdToKernel(info.GetIid());
    OpenSession(info);
}

void NetworkAgentTemplate::ConnectOnlineDevices()
{
    auto infos = DeviceManagerAgent::GetInstance()->GetRemoteDevicesInfo();
    LOGI("Have %{public}d devices Online", infos.size());
    for (const auto &info : infos) {
        auto cmd =
            make_unique<Cmd<NetworkAgentTemplate, const DeviceInfo>>(&NetworkAgentTemplate::ConnectDeviceAsync, info);
        cmd->UpdateOption({
            .tryTimes_ = MAX_RETRY_COUNT,
        });
        Recv(move(cmd));
    }
}

void NetworkAgentTemplate::DisconnectDevice(const DeviceInfo info)
{
    LOGI("DeviceOffline, cid:%{public}s", info.GetCid().c_str());
    kernerlTalker_.SinkOfflineCmdToKernel(info.GetCid());
    sessionPool_.ReleaseSession(info.GetCid());
}

void NetworkAgentTemplate::CloseAllSession(const string &cid)
{
    sessionPool_.ReleaseSession(cid);
}

void NetworkAgentTemplate::AcceptSession(shared_ptr<BaseSession> session)
{
    unique_lock<mutex> taskLock(taskMut_);
    tasks_.emplace_back();
    tasks_.back().Run([=] {
        auto cid = session->GetCid();
        LOGI("AcceptSession thread run, cid:%{public}s", cid.c_str());
        sessionPool_.HoldSession(session);
        return true;
    });
}

void NetworkAgentTemplate::GetSessionProcess(NotifyParam &param)
{
    string cidStr(param.remoteCid, CID_MAX_LEN);
    LOGI("NOTIFY_GET_SESSION, old fd %{public}d, remote cid %{public}s", param.fd, cidStr.c_str());
    sessionPool_.ReleaseSession(param.fd);
    GetSesion(cidStr);
}

void NetworkAgentTemplate::GetSesion(const string &cid)
{
    DeviceInfo deviceInfo;
    deviceInfo.SetCid(cid);
    try {
        OpenSession(deviceInfo);
    } catch (const Exception &e) {
        LOGE("reget session failed, code: %{public}d", e.code());
    }
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS