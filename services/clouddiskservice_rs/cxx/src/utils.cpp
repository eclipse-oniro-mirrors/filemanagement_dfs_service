/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "utils.h"

#include <vector>

#include "accesstoken_kit.h"
#include "errors.h"
#include "os_account_manager.h"

namespace OHOS::FileManagement::CloudDiskService {
int32_t GetTokenTypeFlag(uint32_t tokenId)
{
    return static_cast<int32_t>(Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId));
}

int32_t VerifyAccessToken(uint32_t tokenId, const std::string &permissionName)
{
    return Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenId, permissionName);
}

int32_t GetHapBundleName(uint32_t tokenId, std::string &bundleName, int32_t &instIndex)
{
    Security::AccessToken::HapTokenInfo info;
    int32_t ret = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, info);
    if (ret == ERR_OK) {
        bundleName = info.bundleName;
        instIndex = info.instIndex;
    }
    return ret;
}

int32_t GetNativeProcessName(uint32_t tokenId, std::string &processName)
{
    Security::AccessToken::NativeTokenInfo info;
    int32_t ret = Security::AccessToken::AccessTokenKit::GetNativeTokenInfo(tokenId, info);
    if (ret == ERR_OK) {
        processName = info.processName;
    }
    return ret;
}

bool GetActiveUser(int32_t &userId)
{
    std::vector<int32_t> activeUsers;
    if (AccountSA::OsAccountManager::QueryActiveOsAccountIds(activeUsers) != ERR_OK || activeUsers.empty()) {
        return false;
    }
    userId = activeUsers.front();
    return true;
}

bool IsUserVerified(int32_t userId)
{
    bool verified = false;
    return AccountSA::OsAccountManager::IsOsAccountVerified(userId, verified) == ERR_OK && verified;
}
} // namespace OHOS::FileManagement::CloudDiskService
