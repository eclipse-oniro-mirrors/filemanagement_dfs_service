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

#include "network/softbus/softbus_session_dispatcher.h"

#include <sstream>

#include "network/softbus/softbus_agent.h"
#include "session.h"
#include "utils_log.h"

namespace OHOS {
namespace DistributedFile {
constexpr int32_t SESSION_NAME_SIZE_MAX = 256;

std::mutex SoftbusSessionDispatcher::softbusAgentMutex_;
std::map<std::string, std::weak_ptr<SoftbusAgent>> SoftbusSessionDispatcher::busNameToAgent_;

void SoftbusSessionDispatcher::RegisterSessionListener(const std::string busName,
                                                       std::weak_ptr<SoftbusAgent> softbusAgent)
{
    if (busName == "") {
        std::stringstream ss;
        ss << "Failed to register session to softbus";
        LOGE("%{public}s", ss.str().c_str());
        throw std::runtime_error(ss.str());
    }
    std::lock_guard<std::mutex> lock(softbusAgentMutex_);
    auto agent = busNameToAgent_.find(busName);
    if (agent != busNameToAgent_.end()) {
        std::stringstream ss;
        ss << "this softbusAgent is not exist, busName: " << busName.c_str();
        LOGE("%{public}s", ss.str().c_str());
        throw std::runtime_error(ss.str());
    } else {
        busNameToAgent_.insert(std::make_pair(busName, softbusAgent));
    }
    LOGD("RegisterSessionListener SUCCESS, busName:%{public}s", busName.c_str());
}
void SoftbusSessionDispatcher::UnregisterSessionListener(const std::string busName, std::weak_ptr<SoftbusAgent>)
{
    std::lock_guard<std::mutex> lock(softbusAgentMutex_);
    auto agent = busNameToAgent_.find(busName);
    if (agent != busNameToAgent_.end()) {
        busNameToAgent_.erase(busName);
    } else {
        std::stringstream ss;
        ss << "this softbusAgent is not exist, busName: " << busName.c_str();
        LOGE("%{public}s", ss.str().c_str());
        throw std::runtime_error(ss.str());
    }
    LOGD("UnregisterSessionListener SUCCESS, busName:%{public}s", busName.c_str());
}
std::weak_ptr<SoftbusAgent> SoftbusSessionDispatcher::GetAgent(int sessionId)
{
    char peeSessionName[SESSION_NAME_SIZE_MAX];
    int ret = GetPeerSessionName(sessionId, peeSessionName, sizeof(peeSessionName));
    if (ret != 0) { // ! TODO 魔术字
        LOGE("Get my peer session name failed, session id is %{public}d.", sessionId);
        return {}; // ! TODO
    }
    auto agent = busNameToAgent_.find(std::string(peeSessionName));
    if (agent != busNameToAgent_.end()) {
        LOGD("Get softbus Agent Success, busName:%{public}s", peeSessionName);
        return agent->second;
    }
    LOGE("Get Session Agent fail, not exist! sessionId:%{public}d, busName:%{public}s", sessionId, peeSessionName);
    return {}; // ! TODO
}
int SoftbusSessionDispatcher::OnSessionOpened(int sessionId, int result)
{
    auto agent = GetAgent(sessionId);
    if (auto spt = agent.lock()) {
        return spt->OnSessionOpened(sessionId, result);
    } else {
        LOGE("session not exist!, session id is %{public}d", sessionId);
        return -1;
    }
}
void SoftbusSessionDispatcher::OnSessionClosed(int sessionId)
{
    auto agent = GetAgent(sessionId);
    if (auto spt = agent.lock()) {
        spt->OnSessionClosed(sessionId);
    } else {
        LOGE("session not exist!, session id is %{public}d", sessionId);
    }
}
} // namespace DistributedFile
} // namespace OHOS