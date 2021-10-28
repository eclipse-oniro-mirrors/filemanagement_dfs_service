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

#include "deamon_stub.h"

#include <mutex>

#include "i_deamon.h"
#include "iremote_stub.h"
#include "system_ability.h"

namespace OHOS {
namespace DistributedFile {
enum class ServiceRunningState { STATE_NOT_START, STATE_RUNNING };

class Deamon : public SystemAbility, public DeamonStub, protected NoCopyable {
    DECLARE_SYSTEM_ABILITY(Deamon);

public:
    explicit Deamon(int32_t saID, bool runOnCreate = true) : SystemAbility(saID, runOnCreate){};
    ~Deamon() = default;

    void OnStart() override;
    void OnStop() override;
    ServiceRunningState QueryServiceState() const
    {
        return state_;
    }

    int32_t EchoServerDemo(const std::string &echoStr) override;

private:
    Deamon();
    ServiceRunningState state_{ServiceRunningState::STATE_NOT_START};
    static sptr<Deamon> instance_;
    static std::mutex instanceLock_;
    bool registerToService_{false};

    void PublishSA();
    void StartManagers();
};
} // namespace DistributedFile
} // namespace OHOS