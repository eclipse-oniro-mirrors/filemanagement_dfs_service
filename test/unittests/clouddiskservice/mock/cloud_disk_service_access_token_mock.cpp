/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "cloud_disk_service_access_token_mock.h"

namespace OHOS::FileManagement::CloudDiskService {
using namespace std;
bool CloudDiskServiceAccessToken::CheckCallerPermission(const std::string &permissionName)
{
    return FileManagement::CloudDiskService::CloudDiskServiceAccessTokenVirtual::
        dfsuAccessToken->CheckCallerPermission(permissionName);
}

bool CloudDiskServiceAccessToken::CheckPermission(uint32_t tokenId, const std::string &permissionName)
{
    return FileManagement::CloudDiskService::CloudDiskServiceAccessTokenVirtual::
        dfsuAccessToken->CheckPermission(tokenId, permissionName);
}

int32_t CloudDiskServiceAccessToken::GetCallerBundleName(std::string &bundleName)
{
    return FileManagement::CloudDiskService::CloudDiskServiceAccessTokenVirtual::
        dfsuAccessToken->GetCallerBundleName(bundleName);
}

int32_t CloudDiskServiceAccessToken::GetBundleNameByToken(uint32_t tokenId, std::string &bundleName)
{
    return FileManagement::CloudDiskService::CloudDiskServiceAccessTokenVirtual::
        dfsuAccessToken->GetBundleNameByToken(tokenId, bundleName);
}

int32_t CloudDiskServiceAccessToken::GetUserId()
{
    return FileManagement::CloudDiskService::CloudDiskServiceAccessTokenVirtual::
        dfsuAccessToken->GetUserId();
}

int32_t CloudDiskServiceAccessToken::GetAccountId(int32_t &userId)
{
    return FileManagement::CloudDiskService::CloudDiskServiceAccessTokenVirtual::
        dfsuAccessToken->GetAccountId(userId);
}

bool CloudDiskServiceAccessToken::IsUserVerifyed(const int32_t userId)
{
    return FileManagement::CloudDiskService::CloudDiskServiceAccessTokenVirtual::
        dfsuAccessToken->IsUserVerifyed(userId);
}

} // namespace OHOS::FileManagement