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
#include "network/softbus/softbus_agent.h"

#include <sstream>

#include "ipc/i_deamon.h"
#include "network/softbus/softbus_session.h"
#include "network/softbus/softbus_session_dispatcher.h"
#include "network/softbus/softbus_session_name.h"
#include "session.h"
#include "utils_log.h"

namespace OHOS {
namespace DistributedFile {
using namespace std;

SoftbusAgent::SoftbusAgent(weak_ptr<MountPoint> mountPoint) : NetworkAgentTemplate(mountPoint)
{
    auto spt = mountPoint.lock();
    if (spt == nullptr) {
        LOGE("mountPoint is not exist! bad weak_ptr");
        sessionName_ = "";
        return;
    }

    string path = spt->GetMountArgument().GetFullDst();
    SoftbusSessionName sessionName(path);
    sessionName_ = sessionName.ToString();
}

void SoftbusAgent::JoinDomain()
{
    ISessionListener sessionListener = {
        .OnSessionOpened = SoftbusSessionDispatcher::OnSessionOpened,
        .OnSessionClosed = SoftbusSessionDispatcher::OnSessionClosed,
        .OnBytesReceived = nullptr,
        .OnMessageReceived = nullptr,
        .OnStreamReceived = nullptr,
    };

    SoftbusSessionDispatcher::RegisterSessionListener(sessionName_, shared_from_this());
    int ret = ::CreateSessionServer(IDeamon::SERVICE_NAME.c_str(), sessionName_.c_str(), &sessionListener);
    if (ret != 0) { // ! 错误码
        stringstream ss;
        ss << "Failed to CreateSessionServer, errno:" << ret;
        LOGE("%{public}s, sessionName:%{public}s", ss.str().c_str(), sessionName_.c_str());
        throw runtime_error(ss.str());
    }
    LOGD("Succeed to JoinDomain, busName:%{public}s", sessionName_.c_str());
}

void SoftbusAgent::QuitDomain()
{
    int ret = ::RemoveSessionServer(IDeamon::SERVICE_NAME.c_str(), sessionName_.c_str());
    if (ret != 0) { // ! 错误码
        stringstream ss;
        ss << "Failed to RemoveSessionServer, errno:" << ret;
        LOGE("%{public}s", ss.str().c_str());
        throw runtime_error(ss.str());
    }

    SoftbusSessionDispatcher::UnregisterSessionListener(sessionName_.c_str(), shared_from_this());
    LOGD("Succeed to QuitDomain, busName:%{public}s", sessionName_.c_str());
}
void SoftbusAgent::StopTopHalf()
{
    QuitDomain();
}
void SoftbusAgent::StopBottomHalf() {}
shared_ptr<BaseSession> SoftbusAgent::OpenSession(const DeviceInfo &info)
{
    SessionAttribute attr;
    attr.dataType = TYPE_BYTES;
    attr.unique = true;

    LOGD("Start to Open Session, cid:%{public}s", info.GetCid().c_str());

    int sessionId =
        ::OpenSession(sessionName_.c_str(), sessionName_.c_str(), info.GetCid().c_str(), "hmdfs_wifiGroup", &attr);
    if (sessionId < 0) {
        LOGE("Failed to open session, cid:%{public}s, sessionId:%{public}d", info.GetCid().c_str(), sessionId);
        return nullptr;
    }
    return make_shared<SoftbusSession>(sessionId);
}

void SoftbusAgent::CloseSession(shared_ptr<BaseSession> session)
{
    if (session == nullptr) {
        LOGE("Failed to close session, error:invalid session");
        return;
    }
    session->Release();
}

int SoftbusAgent::OnSessionOpened(const int sessionId, const int result)
{
    auto session = make_shared<SoftbusSession>(sessionId);
    auto cid = session->GetCid();
    if (!session->IsFromServer()) {
        if (result != 0) {
            // !是否加OpenSession重试？
            LOGE("open session failed, result:%{public}d", result);
            return 0;
        }
    }
    int socket_fd = session->GetHandle();
    LOGI(
        "accept sesion, sessionid:%{public}d, Is %{public}s Side, fd %{public}d, from cid %{public}s, result "
        "%{public}d",
        sessionId, (session->IsFromServer() == true) ? "Server" : "Client", socket_fd, cid.c_str(), result);
    session->DisableSessionListener();
    AcceptSession(session);
    return 0;
}

void SoftbusAgent::OnSessionClosed(int sessionId)
{
    auto session = make_shared<SoftbusSession>(sessionId);
    auto cid = session->GetCid();
    LOGI("Session to %{public}s closed by unknown reason, Is %{public}s Side", cid.c_str(),
         (session->IsFromServer() == true) ? "Server" : "Client");
}
} // namespace DistributedFile
} // namespace OHOS