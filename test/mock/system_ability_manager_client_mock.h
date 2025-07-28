/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TEST_SYSTEM_ABILITY_MANAGER_CLENT_MOCK_H
#define TEST_SYSTEM_ABILITY_MANAGER_CLENT_MOCK_H

#include <gmock/gmock.h>
#include "if_system_ability_manager_mock.h"
#include "iservice_registry.h"

namespace OHOS {
class ISystemAbilityManagerClient {
public:
    virtual sptr<ISystemAbilityManager> GetSystemAbilityManager() = 0;
    virtual ~ISystemAbilityManagerClient() = default;
    static inline std::shared_ptr<ISystemAbilityManagerClient> smc = nullptr;
};

class SystemAbilityManagerClientMock : public ISystemAbilityManagerClient {
public:
    MOCK_METHOD0(GetSystemAbilityManager, sptr<ISystemAbilityManager>());
};
}

#endif