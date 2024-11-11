/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "system_ability_manager_client_mock.h"
#include "utils_log.h"

using namespace OHOS::Storage::DistributedFile;

namespace OHOS {
SystemAbilityManagerClient& SystemAbilityManagerClient::GetInstance()
{
    static auto instance = std::make_shared<SystemAbilityManagerClient>();
    return *instance;
}

sptr<ISystemAbilityManager> SystemAbilityManagerClient::GetSystemAbilityManager()
{
    if (DfsSystemAbilityManagerClient::smc == nullptr) {
        return nullptr;
    }
    return DfsSystemAbilityManagerClient::smc->GetSystemAbilityManager();
}
}