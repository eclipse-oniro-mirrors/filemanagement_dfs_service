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

#include <system_ability_definition.h>
#include "device_manager_agent.h"
#include "utils_log.h"
#include "utils_exception.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
using namespace std;

DistributedFileService::DistributedFileService() : SystemAbility(STORAGE_DISTRIBUTED_FILE_SERVICE_SA_ID, false) {}

DistributedFileService::~DistributedFileService() {}

void DistributedFileService::OnDump()
{
    LOGI("OnDump");
}

void DistributedFileService::PublishSA()
{
    LOGI("Begin to init, pull the service"); // todo: 看一下ipc使用方法；是否必须使用单例？； 是否需要加flag?; 抛异常处理？
    bool ret = SystemAbility::Publish(this); // DelayedSingleton<DistributedFileService>::GetInstance().get()
    if (!ret) {
        throw runtime_error("publishing DistributedFileService failed");
    }
    // if (!registerToService_) {
    //     bool ret = SystemAbility::Publish(this);
    //     if (!ret) {
    //         throw runtime_error("Failed to publish the daemon");
    //     }
    //     registerToService_ = true;
    // }
    LOGI("Init finished successfully");
}
void DistributedFileService::StartManagers()
{
    DeviceManagerAgent::GetInstance();
}

void DistributedFileService::OnStart()
{
    try {
        PublishSA();
        StartManagers();
    } catch (const Exception &e) {
        LOGE("%{public}s", e.what());
    }
}

void DistributedFileService::OnStop()
{
    LOGI("DistributedFileService::OnStop");
}

int32_t DistributedFileService::SendFile(int32_t sessionId, const std::string &sourceFileList,
    const std::string &destinationFileList, uint32_t fileCount)
{
    // DistributedFileInfo metaBase;
    // metaBase.name = dirName;
    // if (metaBase.name.empty()) {
    //     LOGE("DistributedFileService-%{public}s: Failed to get app dir, error: invalid app name", __func__);
    //     return DISTRIBUTEDFILE_DIR_NAME_IS_EMPTY;
    // }

    // sptr<ISystemAbilityManager> systemAbilityMgr =
    //     SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    // if (systemAbilityMgr == nullptr) {
    //     LOGE("BundleService Get ISystemAbilityManager failed ... \n");
    //     return DISTRIBUTEDFILE_REMOTE_ADDRESS_IS_NULL;
    // }

    // sptr<IRemoteObject> remote = systemAbilityMgr->CheckSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    // if (remote == nullptr) {
    //     LOGE("BundleService Get IRemoteObject failed ... \n");
    //     return DISTRIBUTEDFILE_REMOTE_ADDRESS_IS_NULL;
    // }

    // auto BundleMgrService = std::make_unique<AppExecFwk::BundleMgrProxy>(remote);
    // if (BundleMgrService.get() == nullptr) {
    //     LOGE("remote iface_cast BundleMgrService failed ... \n");
    //     return DISTRIBUTEDFILE_REMOTE_ADDRESS_IS_NULL;
    // }

    // Utils::ForceRemoveDirectory(metaBase.name);
    //OpenSession(nullptr, nullptr, nullptr, nullptr, nullptr);
    LOGD("DistributedFileService::SendFile");
    return DISTRIBUTEDFILE_SUCCESS;
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
