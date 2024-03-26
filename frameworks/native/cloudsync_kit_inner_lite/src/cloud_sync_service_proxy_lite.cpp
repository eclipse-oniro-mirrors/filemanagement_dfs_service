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
#include "cloud_sync_service_proxy_lite.h"

#include <sstream>

#include "cloud_file_sync_service_interface_code.h"
#include "dfs_error.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {

constexpr int LOAD_SA_TIMEOUT_MS = 4000;

int32_t CloudSyncServiceProxyLite::TriggerSyncInner(const std::string &bundleName, const int32_t &userId)
{
    return 0;
}

sptr<ICloudSyncServiceLite> CloudSyncServiceProxyLite::GetInstance()
{
    return nullptr;
}

void CloudSyncServiceProxyLite::InvalidInstance()
{
}

void CloudSyncServiceProxyLite::ServiceProxyLoadCallbackLite::OnLoadSystemAbilitySuccess(
    int32_t systemAbilityId,
    const sptr<IRemoteObject> &remoteObject)
{
}

void CloudSyncServiceProxyLite::ServiceProxyLoadCallbackLite::OnLoadSystemAbilityFail(
    int32_t systemAbilityId)
{
}
} // namespace OHOS::FileManagement::CloudSync
