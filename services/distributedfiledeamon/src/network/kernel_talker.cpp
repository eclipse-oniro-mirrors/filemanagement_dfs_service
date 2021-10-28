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

#include "network/kernel_talker.h"
#include "device/device_manager_agent.h"
#include "securec.h"
#include "utils_log.h"

namespace OHOS {
namespace DistributedFile {
constexpr int KEY_MAX_LEN = 32;
constexpr int HMDFS_ACCOUNT_HASH_MAX_LEN = 21;
constexpr int POLL_TIMEOUT_MS = 200;
constexpr int NONE_EVENT = -1;
constexpr int READ_EVENT = 1;
constexpr int TIME_OUT_EVENT = 0;

struct InitParam {
    int32_t cmd;
    uint64_t localIid;
    uint8_t currentAccout[HMDFS_ACCOUNT_HASH_MAX_LEN];
} __attribute__((packed));

struct UpdateSocketParam {
    int32_t cmd;
    int32_t newfd;
    uint64_t localIid;
    uint8_t status;
    uint8_t protocol;
    uint16_t udpPort;
    uint8_t deviceType;
    uint8_t masterKey[KEY_MAX_LEN];
    char cid[CID_MAX_LEN];
    int32_t linkType;
    int32_t binderFd;
} __attribute__((packed));

struct OfflineParam {
    int32_t cmd;
    char remoteCid[CID_MAX_LEN];
} __attribute__((packed));

enum {
    CMD_INIT = 0,
    CMD_UPDATE_SOCKET,
    CMD_OFF_LINE,
    CMD_SET_ACCOUNT,
    CMD_OFF_LINE_ALL,
    CMD_UPDATE_CAPABILITY,
    CMD_GET_P2P_SESSION_FAIL,
    CMD_DELETE_CONNECTION,
    CMD_CNT
};

enum {
    SOCKET_STAT_ACCEPT = 0,
    SOCKET_STAT_OPEN,
};

enum {
    TCP_TRANSPORT_PROTO = 0,
    UDP_TRANSPORT_PROTO,
};

enum Notify {
    NOTIFY_HS_DONE = 0,
    NOTIFY_OFFLINE,
    NOTIFY_OFFLINE_IID,
    NOTIFY_GET_SESSION,
    NOTIFY_GOT_UDP_PORT,
    NOTIFY_NONE,
    NOTIFY_DISCONNECT,
    NOTIFY_D2DP_FAILED,
    NOTIFY_CNT,
};

void KernelTalker::SinkSessionTokernel(std::shared_ptr<BaseSession> session)
{
    int socketFd = session->GetHandle();
    auto masterkey = session->GetKey();
    auto cid = session->GetCid();
    LOGD("sink session to kernel success, cid:%{public}s, socketFd:%{public}d, key[0]:%{public}x", cid.c_str(),
         socketFd, *(uint32_t *)masterkey.data());

    uint8_t status = (session->IsFromServer() ? SOCKET_STAT_ACCEPT : SOCKET_STAT_OPEN);

    UpdateSocketParam cmd = {
        .cmd = CMD_UPDATE_SOCKET,
        .newfd = socketFd,
        .localIid = DeviceManagerAgent::GetInstance().GetLocalDeviceInfo().GetIid(),
        .status = status,
        .protocol = TCP_TRANSPORT_PROTO,
        .linkType = 0,
        .binderFd = -1,
    };
    if (memcpy_s(cmd.masterKey, KEY_MAX_LEN, masterkey.data(), KEY_MAX_LEN) != EOK) {
        return; // !抛异常
    }

    if (memcpy_s(cmd.cid, CID_MAX_LEN, cid.c_str(), CID_MAX_LEN)) {
        return; // !抛异常
    }
    session->DisableSessionListener();
    SetCmd(cmd);
}

void KernelTalker::SinkInitCmdToKernel(uint64_t iid)
{
    InitParam cmd = {
        .cmd = CMD_INIT,
        .localIid = iid,
    };

    SetCmd(cmd);
}

void KernelTalker::SinkOfflineCmdToKernel(std::string cid)
{
    OfflineParam cmd = {
        .cmd = CMD_OFF_LINE,
    };

    if (cid.length() < CID_MAX_LEN) {
        return; // ! 抛异常
    }

    if (memcpy_s(cmd.remoteCid, CID_MAX_LEN, cid.c_str(), CID_MAX_LEN) != EOK) {
        return; // ! 抛异常
    }
    SetCmd(cmd);
}

std::unordered_set<int> KernelTalker::GetKernelSesions()
{
    return {};
}

void KernelTalker::CreatePollThread()
{
    isRunning_ = true;
    if (pollThread_ != nullptr) {
        LOGE("pollTread is not null");
        return;
    }
    pollThread_ = std::make_unique<std::thread>(&KernelTalker::PollRun, this);
    LOGI("Create pollThread OK");
}

void KernelTalker::WaitForPollThreadExited()
{
    isRunning_ = false;
    if (pollThread_ == nullptr) {
        LOGE("pollTread is null");
        return;
    }

    if (pollThread_->joinable()) {
        LOGI("pollThread->joinable is true");
        pollThread_->join();
    }
    pollThread_ = nullptr;
    LOGI("pollTread exit ok");
}

void KernelTalker::PollRun()
{
    struct pollfd fileFd;
    int cmdFd = -1;

    LOGI("entry");
    auto spt = mountPoint_.lock();
    if (spt == nullptr) {
        LOGE("mountPoint is not exist! bad weak_ptr");
        return; // ! 抛异常
    }
    std::string ctrlPath = spt->GetMountArgument().GetCtrlPath();
    cmdFd = open(ctrlPath.c_str(), O_RDWR);
    if (cmdFd < 0) {
        LOGE("Open node file error %{public}d", errno);
        return; // ! 待审视，此处不能抛异常，用户态还没有通知到内核时，这个文件可能就不存在
    }

    LOGI("Open node file success");

    while (isRunning_) {
        fileFd.fd = cmdFd;
        fileFd.events = POLLPRI;
        fileFd.revents = 0;
        int ret = poll(&fileFd, 1, POLL_TIMEOUT_MS);
        switch (ret) {
            case NONE_EVENT:
                LOGI("none event, poll exit");
                break;
            case TIME_OUT_EVENT:
                break;
            case READ_EVENT:
                HandleAllNotify(cmdFd);
                break;
            default:
                LOGI("poll exit");
        }
    }
    close(cmdFd);
    LOGI("exit");
    return;
}

void KernelTalker::HandleAllNotify(int fd)
{
    NotifyParam param;

    while (isRunning_) {
        lseek(fd, 0, SEEK_SET);
        param.notify = NOTIFY_NONE;
        int readSize = read(fd, &param, sizeof(NotifyParam));
        if ((readSize < (int)sizeof(NotifyParam)) || (param.notify == NOTIFY_NONE)) {
            return;
        }
        NotifyHandler(param);
    }
}

void KernelTalker::NotifyHandler(NotifyParam &param)
{
    int cmd = param.notify;
    std::string cidStr(param.remoteCid, CID_MAX_LEN);
    switch (cmd) {
        case NOTIFY_HS_DONE:
            LOGI("NOTIFY_HS_DONE, remote cid %{public}s", cidStr.c_str());
            break;
        case NOTIFY_OFFLINE:
        case NOTIFY_OFFLINE_IID:
            LOGI("%{public}s, remote cid %{public}s", (cmd == NOTIFY_OFFLINE) ? "NOTIFY_OFFLINE" : "NOTIFY_OFFLINE_IID",
                 cidStr.c_str());
            break;
        case NOTIFY_GET_SESSION:
            GetSessionCallback_(param);
            break;
        default:
            LOGI("cmd %{public}d not support now", cmd);
            break;
    }
}
} // namespace DistributedFile
} // namespace OHOS