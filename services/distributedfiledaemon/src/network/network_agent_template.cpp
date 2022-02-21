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
constexpr int OPEN_SESSSION_DELAY_TIME = 100;
} // namespace

void NetworkAgentTemplate::Start()
{
    JoinDomain();
    kernerlTalker_->CreatePollThread();
    ConnectOnlineDevices();
}

void NetworkAgentTemplate::Stop()
{
    StopTopHalf();
    StopBottomHalf();
    kernerlTalker_->WaitForPollThreadExited();
}

void NetworkAgentTemplate::ConnectDeviceAsync(const DeviceInfo info)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(
        OPEN_SESSSION_DELAY_TIME)); // Temporary workaround for time sequence issues(offline-onSessionOpened)
    OpenSession(info);
}

void NetworkAgentTemplate::ConnectOnlineDevices()
{
    auto dma = DeviceManagerAgent::GetInstance();
    auto infos = dma->GetRemoteDevicesInfo();
    LOGI("Have %{public}d devices Online", infos.size());
    for (const auto &info : infos) {
        auto cmd =
            make_unique<Cmd<NetworkAgentTemplate, const DeviceInfo>>(&NetworkAgentTemplate::ConnectDeviceAsync, info);
        cmd->UpdateOption( {
            .tryTimes_ = MAX_RETRY_COUNT,
        });
        Recv(move(cmd));
    }
}

void NetworkAgentTemplate::DisconnectAllDevices()
{
    sessionPool_.ReleaseAllSession();
}

void NetworkAgentTemplate::DisconnectDevice(const DeviceInfo info)
{
    LOGI("DeviceOffline, cid:%{public}s", info.GetCid().c_str());
    sessionPool_.ReleaseSession(info.GetCid());
}

void NetworkAgentTemplate::CloseSessionForOneDevice(const string &cid)
{
    LOGI("session closed!");
}

void NetworkAgentTemplate::AcceptSession(shared_ptr<BaseSession> session)
{
    auto cmd = make_unique<Cmd<NetworkAgentTemplate, shared_ptr<BaseSession>>>(
        &NetworkAgentTemplate::AcceptSessionInner, session);
    cmd->UpdateOption({
        .tryTimes_ = 1,
    });
    Recv(move(cmd));
}

void NetworkAgentTemplate::AcceptSessionInner(shared_ptr<BaseSession> session)
{
    auto cid = session->GetCid();
    LOGI("AcceptSesion, cid:%{public}s", cid.c_str());
    sessionPool_.HoldSession(session);
}

void NetworkAgentTemplate::GetSessionProcess(NotifyParam &param)
{
    auto cmd =
        make_unique<Cmd<NetworkAgentTemplate, NotifyParam>>(&NetworkAgentTemplate::GetSessionProcessInner, param);
    cmd->UpdateOption({.tryTimes_ = 1});
    Recv(move(cmd));
}

void NetworkAgentTemplate::GetSessionProcessInner(NotifyParam param)
{
    string cidStr(param.remoteCid, CID_MAX_LEN);
    int fd = param.fd;
    LOGI("NOTIFY_GET_SESSION, old fd %{public}d, remote cid %{public}s", fd, cidStr.c_str());
    sessionPool_.ReleaseSession(fd);
    GetSession(cidStr);
}

void NetworkAgentTemplate::GetSession(const string &cid)
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