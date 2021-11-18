/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#include "distributedfile_service.h"

#include <exception>
#include <system_ability_definition.h>
#include <stdio.h>

#include "bundle_mgr_interface.h"
#include "bundle_mgr_proxy.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "message_parcel.h"
#include "parcel.h"
#include "utils_directory.h"
#include "utils_log.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
using namespace std;
const bool g_registerResult =
    SystemAbility::MakeAndRegisterAbility(DelayedSingleton<DistributedFileService>::GetInstance().get());

DistributedFileService::DistributedFileService() : SystemAbility(STORAGE_DISTRIBUTED_FILE_SERVICE_SA_ID, false) {}

DistributedFileService::~DistributedFileService() {}

void DistributedFileService::OnDump()
{
    LOGI("OnDump");
}

void DistributedFileService::OnStart()
{
    bool ret = SystemAbility::Publish(DelayedSingleton<DistributedFileService>::GetInstance().get());
    if (!ret) {
        LOGE("Leave, publishing DistributedFileService failed!");
        return;
    }
}

void DistributedFileService::OnStop()
{
    LOGI("DistributedFileService::OnStop start");
}

int32_t DistributedFileService::GetBundleDistributedDir(const std::string &dirName)
{
    std::string path = dirName;
    if (path.empty()) {
        LOGE("DistributedFileService-%{public}s: Failed to get app dir, error: invalid app name", __func__);
        return DISTRIBUTEDFILE_DIR_NAME_IS_EMPTY;
    }

    sptr<ISystemAbilityManager> systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        LOGE("BundleService Get ISystemAbilityManager failed ... \n");
        return DISTRIBUTEDFILE_REMOTE_ADDRESS_IS_NULL;
    }

    sptr<IRemoteObject> remote = systemAbilityMgr->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remote == nullptr) {
        LOGE("BundleService Get IRemoteObject failed ... \n");
        return DISTRIBUTEDFILE_REMOTE_ADDRESS_IS_NULL;
    }

    int callingUid = IPCSkeleton::GetCallingUid();
    auto BundleMgrService = std::make_unique<AppExecFwk::BundleMgrProxy>(remote);
    if (BundleMgrService.get() == nullptr) {
        LOGE("remote iface_cast BundleMgrService failed ... %{public}s\n", strerror(errno));
        return DISTRIBUTEDFILE_REMOTE_ADDRESS_IS_NULL;
    }

    int32_t uid = BundleMgrService->GetUidByBundleName(path, callingUid);
    if (uid < 0) {
        LOGE("DistributedFileService-%{public}s: Failed to get uid", __func__);
        return DISTRIBUTEDFILE_NAME_NOT_FOUND;
    }

    if (callingUid != 0) {
        if (uid != callingUid) {
            LOGE("DistributedFileService-%{public}s: Bundle failed to create dir", __func__);
            return DISTRIBUTEDFILE_PERMISSION_DENIED;
        }
    }

    LOGI("DistributedFileService::GetBundleDistributedDir path : %{public}s", path.c_str());
    Utils::ForceCreateDirectory(path);
    return DISTRIBUTEDFILE_SUCCESS;
}

int32_t DistributedFileService::RemoveBundleDistributedDirs(const std::string &dirName)
{
    std::string path = dirName;
    if (path.empty()) {
        LOGE("DistributedFileService-%{public}s: Failed to get app dir, error: invalid app name", __func__);
        return DISTRIBUTEDFILE_DIR_NAME_IS_EMPTY;
    }

    sptr<ISystemAbilityManager> systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        LOGE("BundleService Get ISystemAbilityManager failed ... \n");
        return DISTRIBUTEDFILE_REMOTE_ADDRESS_IS_NULL;
    }

    sptr<IRemoteObject> remote = systemAbilityMgr->CheckSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remote == nullptr) {
        LOGE("BundleService Get IRemoteObject failed ... \n");
        return DISTRIBUTEDFILE_REMOTE_ADDRESS_IS_NULL;
    }

    auto BundleMgrService = std::make_unique<AppExecFwk::BundleMgrProxy>(remote);
    if (BundleMgrService.get() == nullptr) {
        LOGE("remote iface_cast BundleMgrService failed ... \n");
        return DISTRIBUTEDFILE_REMOTE_ADDRESS_IS_NULL;
    }

    Utils::ForceRemoveDirectory(path);
    return DISTRIBUTEDFILE_SUCCESS;
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
