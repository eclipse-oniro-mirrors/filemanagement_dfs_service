/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef DAEMON_H
#define DAEMON_H

#include <memory>
#include <mutex>
#include <string>

#include "accesstoken_kit.h"
#include "daemon_stub.h"
#include "dm_device_info.h"
#include "ipc/i_daemon.h"
#include "iremote_stub.h"
#include "multiuser/os_account_observer.h"
#include "nocopyable.h"
#include "refbase.h"
#include "system_ability.h"
#include "accesstoken_kit.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
using HapTokenInfo = OHOS::Security::AccessToken::HapTokenInfo;
enum class ServiceRunningState { STATE_NOT_START, STATE_RUNNING };

class Daemon final : public SystemAbility, public DaemonStub, protected NoCopyable {
    DECLARE_SYSTEM_ABILITY(Daemon);

public:
    explicit Daemon(int32_t saID, bool runOnCreate = true) : SystemAbility(saID, runOnCreate) {};
    virtual ~Daemon() = default;

    void OnStart() override;
    void OnStop() override;
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    ServiceRunningState QueryServiceState() const
    {
        return state_;
    }

    int32_t OpenP2PConnection(const DistributedHardware::DmDeviceInfo &deviceInfo) override;
    int32_t CloseP2PConnection(const DistributedHardware::DmDeviceInfo &deviceInfo) override;
    int32_t PrepareSession(const std::string &srcUri,
                           const std::string &dstUri,
                           const std::string &srcDeviceId,
                           const sptr<IRemoteObject> &listener) override;
    int32_t RequestSendFile(const std::string &srcUri,
                            const std::string &dstPath,
                            const std::string &dstDeviceId,
                            const std::string &sessionName) override;
    int32_t GetRealPath(const std::string &dstUri,
                        const HapTokenInfo &hapTokenInfo,
                        const std::string &sessionName,
                        std::string &physicalPath);

private:
    Daemon();
    ServiceRunningState state_ { ServiceRunningState::STATE_NOT_START };
    static sptr<Daemon> instance_;
    static std::mutex instanceLock_;
    bool registerToService_ { false };
    std::shared_ptr<OsAccountObserver> subScriber_;
    void PublishSA();
    void RegisterOsAccount();
    int32_t LoadRemoteSA(const std::string &srcUri,
                         const std::string &dstPath,
                         const std::string &localDeviceId,
                         const std::string &remoteDeviceId,
                         const std::string &sessionName);
    void RemoveSession(const std::string &sessionName);
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
#endif // DAEMON_H