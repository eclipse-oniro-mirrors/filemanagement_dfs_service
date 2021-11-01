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
#include "utils_log.h"

namespace OHOS {
namespace DistributedFile {
using namespace std;
namespace {
constexpr int MAX_RETRY_COUNT = 7;
constexpr int SLEEP_TIME = 1500;
} // namespace

NetworkAgentTemplate::~NetworkAgentTemplate() {}

void NetworkAgentTemplate::Start()
{
    JoinDomain(); // TODO 考虑下软总线是否有可能还没有起来
    ConnectOnlineDevices();
    kernerlTalker_.CreatePollThread();
}

void NetworkAgentTemplate::Stop()
{
    StopTopHalf();
    StopBottomHalf();
    kernerlTalker_.WaitForPollThreadExited();
}

void NetworkAgentTemplate::ConnectDeviceAsync(const DeviceInfo &info)
{
    kernerlTalker_.SinkInitCmdToKernel(info.GetIid());

    unique_lock<mutex> taskLock(taskMut_);
    tasks_.emplace_back();
    tasks_.back().RunLoopFlexible(
        [info{DeviceInfo(info)}, this](uint64_t &sleepTime) {  // ! 不能使用this,待解决
            auto session = OpenSession(info);
            if (session == nullptr) {
                LOGE("open session fail, retry, cid:%{public}s", info.GetCid().c_str());
                return false;
            }
            LOGI("open session success, cid:%{public}s", info.GetCid().c_str());
            return true;
        },
        SLEEP_TIME, MAX_RETRY_COUNT);
}

void NetworkAgentTemplate::ConnectOnlineDevices()
{
    auto infos = DeviceManagerAgent::GetInstance()->GetRemoteDevicesInfo();
    LOGI("Have %{public}d devices Online", infos.size());
    for (const auto &info : infos) {
        ConnectDeviceAsync(info);
    }
}

void NetworkAgentTemplate::DisconnectDevice(const DeviceInfo &info)
{
    LOGI("DeviceOffline, cid:%{public}s", info.GetCid().c_str());
    kernerlTalker_.SinkOfflineCmdToKernel(info.GetCid());
    sessionPool_.RefreshSessionPoolBasedOnKernel();
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
    sessionPool_.RefreshSessionPoolBasedOnKernel();
    GetSesion(cidStr);
}

void NetworkAgentTemplate::GetSesion(const string &cid)
{
    DeviceInfo deviceInfo;
    deviceInfo.SetCid(cid);
    auto session = OpenSession(deviceInfo);
    if (session == nullptr) {
        LOGE("open session fail, retry, cid:%{public}s", cid.c_str());
        return;
    }
    LOGI("open session success, cid:%{public}s", cid.c_str());
}
} // namespace DistributedFile
} // namespace OHOS