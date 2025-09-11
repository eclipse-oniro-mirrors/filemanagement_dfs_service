/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "ipc/cloud_daemon.h"

#include <exception>
#include <stdexcept>
#include <thread>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include "appstate_observer.h"
#include "cloud_file_fault_event.h"
#include "dfs_error.h"
#include "ffrt_inner.h"
#include "fuse_manager/fuse_manager.h"
#include "iremote_object.h"
#include "parameters.h"
#include "plugin_loader.h"
#include "setting_data_helper.h"
#include "system_ability_definition.h"
#include "utils_directory.h"
#include "utils_log.h"
#ifdef HICOLLIE_ENABLE
#include "xcollie_helper.h"
#endif

namespace OHOS {
namespace FileManagement {
namespace CloudFile {
using namespace std;
using namespace CloudDisk;

namespace {
    static const string IO_MESSAGE_DIR = "/data/service/el1/public/cloudfile/io/";
    static const string BETA_VERSION = "beta";
    static const string CN_REGION = "CN";
    static const string GLOBAL_REGION = "const.global.region";
    static const string KEY_VERSION_TYPE = "const.logsystem.versiontype";
    static const string LOCAL_PATH_DATA_SERVICE_EL2 = "/data/service/el2/";
    static const string LOCAL_PATH_HMDFS_DENTRY_CACHE = "/hmdfs/cache/account_cache/dentry_cache/";
    static const string LOCAL_PATH_HMDFS_CACHE_CLOUD = "/hmdfs/cache/account_cache/dentry_cache/cloud";
    static const int32_t STAT_MODE_DIR = 0771;
    static const int32_t STAT_MODE_DIR_DENTRY_CACHE = 02771;
    static const int32_t OID_DFS = 1009;
}
REGISTER_SYSTEM_ABILITY_BY_ID(CloudDaemon, FILEMANAGEMENT_CLOUD_DAEMON_SERVICE_SA_ID, true);

CloudDaemon::CloudDaemon(int32_t saID, bool runOnCreate) : SystemAbility(saID, runOnCreate)
{
    accountStatusListener_ = make_shared<AccountStatusListener>();
}

void CloudDaemon::PublishSA()
{
    LOGI("Begin to init");
    if (!registerToService_) {
        bool ret = SystemAbility::Publish(this);
        if (!ret) {
            throw runtime_error("Failed to publish the daemon");
        }
        registerToService_ = true;
    }
    LOGI("Init finished successfully");
}

static bool CheckDeviceInLinux()
{
    struct utsname uts;
    if (uname(&uts) == -1) {
        LOGE("uname get failed.");
        return false;
    }
    if (strcmp(uts.sysname, "Linux") == 0) {
        LOGI("uname system is linux.");
        return true;
    }
    return false;
}

static void ModSysParam()
{
    if (CheckDeviceInLinux()) {
        const string photos = "persist.kernel.bundle_name.photos";
        const string clouddrive = "persist.kernel.bundle_name.clouddrive";
        system::SetParameter(photos, "");
        system::SetParameter(clouddrive, "");
    }
}

static bool ShouldRegisterListener()
{
    const string versionType = system::GetParameter(KEY_VERSION_TYPE, "");
    const string region = system::GetParameter(GLOBAL_REGION, "");
    return versionType == BETA_VERSION && region == CN_REGION;
}

void CloudDaemon::OnStart()
{
    LOGI("Begin to start service");
    if (state_ == ServiceRunningState::STATE_RUNNING) {
        LOGI("Daemon has already started");
        return;
    }

    try {
        PublishSA();
        AddSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
        AddSystemAbilityListener(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
        mode_t mode = 002;
        umask(mode);
        ModSysParam();
    } catch (const exception &e) {
        LOGE("%{public}s", e.what());
    }

    state_ = ServiceRunningState::STATE_RUNNING;
    /* load cloud file ext plugin */
    CloudFile::PluginLoader::GetInstance().LoadCloudKitPlugin();

    if (ShouldRegisterListener()) {
        auto bundleNameList = vector<string>{};
        std::thread listenThread([bundleNameList] {
            if (filesystem::exists(IO_MESSAGE_DIR)) {
                CloudDisk::AppStateObserverManager::GetInstance().SubscribeAppState(bundleNameList);
                return;
            }
            std::error_code errCode;
            filesystem::create_directories(IO_MESSAGE_DIR, errCode);
            if (error_code.value() != 0) {
                LOGE("Mkdir for io failed");
                return;               
            }
            CloudDisk::AppStateObserverManager::GetInstance().SubscribeAppState(bundleNameList);
        });
        listenThread.detach();
    }
    LOGI("Start service successfully");
}

void HandleStartMove(int32_t userId)
{
    const string moveFile = "persist.kernel.move.finish";
    system::SetParameter(moveFile, "false");
    const std::string filemanagerKey = "persist.kernel.bundle_name.filemanager";
    string subList[] = {"com.ohos.photos", system::GetParameter(filemanagerKey, "")};
    string srcBase = "/data/service/el1/public/cloudfile/" + to_string(userId);
    string dstBase = "/data/service/el2/" + to_string(userId) + "/hmdfs/cloudfile_manager";
    string removePath = srcBase + "/" + subList[1] + "/backup";
    bool ret = Storage::DistributedFile::Utils::ForceRemoveDirectoryDeepFirst(removePath);
    if (!ret) {
        LOGE("remove failed path: %{public}s", GetAnonyString(removePath).c_str());
    }
    const auto copyOptions = filesystem::copy_options::overwrite_existing | filesystem::copy_options::recursive;
    for (auto sub : subList) {
        string srcPath = srcBase + '/' + sub;
        string dstPath = dstBase + '/' + sub;
        if (access(srcPath.c_str(), F_OK) != 0) {
            LOGI("srcPath %{public}s not found", GetAnonyString(srcPath).c_str());
            continue;
        }
        LOGI("Begin to move path: %{public}s", GetAnonyString(srcPath).c_str());
        error_code errCode;
        filesystem::copy(srcPath, dstPath, copyOptions, errCode);
        if (errCode.value() != 0) {
            LOGE("copy failed path: %{public}s, errCode: %{public}d",
                GetAnonyString(srcPath).c_str(), errCode.value());
        }
        LOGI("End move path: %{public}s", GetAnonyString(srcPath).c_str());
        bool ret = Storage::DistributedFile::Utils::ForceRemoveDirectoryDeepFirst(srcPath);
        if (!ret) {
            LOGE("remove failed path: %{public}s", GetAnonyString(srcPath).c_str());
        }
    }
    system::SetParameter(moveFile, "true");
}

void CloudDaemon::OnStop()
{
    LOGI("Begin to stop");
    state_ = ServiceRunningState::STATE_NOT_START;
    registerToService_ = false;
    CloudDisk::AppStateObserverManager::GetInstance().UnSubscribeAppState();
    LOGI("Stop finished successfully");
}

void CloudDaemon::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    LOGI("OnAddSystemAbility systemAbilityId:%{public}d added!", systemAbilityId);
    if (systemAbilityId == COMMON_EVENT_SERVICE_ID) {
        accountStatusListener_->Start();
    } else if (systemAbilityId == DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID) {
        // get setting data when sa load
        bool ret = SettingDataHelper::GetInstance().InitActiveBundle();
        LOGI("SERVICE LOAD: Init active bundle, ret: %{public}d", ret);
    }
}

void CloudDaemon::ExecuteStartFuse(int32_t userId, int32_t devFd, const std::string& path)
{
    ffrt::submit([=]() {
        int32_t ret = FuseManager::GetInstance().StartFuse(userId, devFd, path);
        CLOUD_FILE_FAULT_REPORT(CloudFileFaultInfo{"", CloudFile::FaultOperation::SESSION,
            CloudFile::FaultType::FILE, ret, "start fuse, ret = " + std::to_string(ret)});
        });
}

int32_t CloudDaemon::StartFuse(int32_t userId, int32_t devFd, const string &path)
{
#ifdef HICOLLIE_ENABLE
    const int32_t TIMEOUT_S = 2;
    int32_t xcollieId = XCollieHelper::SetTimer("CloudFileDaemon_StartFuse", TIMEOUT_S, nullptr, nullptr, true);
#endif
    LOGI("Start Fuse");
    ExecuteStartFuse(userId, devFd, path);

    string dentryPath = LOCAL_PATH_DATA_SERVICE_EL2 + to_string(userId) + LOCAL_PATH_HMDFS_CACHE_CLOUD;
    if (access(dentryPath.c_str(), F_OK) != 0) {
        string cachePath = LOCAL_PATH_DATA_SERVICE_EL2 + to_string(userId) + LOCAL_PATH_HMDFS_DENTRY_CACHE;
        if (mkdir(cachePath.c_str(), STAT_MODE_DIR) != 0 && errno != EEXIST) {
#ifdef HICOLLIE_ENABLE
            XCollieHelper::CancelTimer(xcollieId);
#endif
            CLOUD_FILE_FAULT_REPORT(CloudFileFaultInfo{"", CloudFile::FaultOperation::SESSION,
                CloudFile::FaultType::FILE, errno, "create accout_cache path error " + std::to_string(errno)});
            return E_PATH;
        }
        if (chmod(cachePath.c_str(), STAT_MODE_DIR_DENTRY_CACHE) != 0) {
            CLOUD_FILE_FAULT_REPORT(CloudFileFaultInfo{"", CloudFile::FaultOperation::SESSION,
                CloudFile::FaultType::FILE, errno, "chmod cachepath error " + std::to_string(errno)});
        }
        if (chown(cachePath.c_str(), OID_DFS, OID_DFS) != 0) {
            CLOUD_FILE_FAULT_REPORT(CloudFileFaultInfo{"", CloudFile::FaultOperation::SESSION,
                CloudFile::FaultType::FILE, errno, "chown cachepath error " + std::to_string(errno)});
        }
        if (mkdir(dentryPath.c_str(), STAT_MODE_DIR) != 0 && errno != EEXIST) {
#ifdef HICOLLIE_ENABLE
            XCollieHelper::CancelTimer(xcollieId);
#endif
            CLOUD_FILE_FAULT_REPORT(CloudFileFaultInfo{"", CloudFile::FaultOperation::SESSION,
                CloudFile::FaultType::FILE, errno, "create dentrypath " + std::to_string(errno)});
            return E_PATH;
        }
        if (chown(dentryPath.c_str(), OID_DFS, OID_DFS) != 0) {
            CLOUD_FILE_FAULT_REPORT(CloudFileFaultInfo{"", CloudFile::FaultOperation::SESSION,
                CloudFile::FaultType::FILE, errno, "chown cachepath error " + std::to_string(errno)});
        }
    }
    if (path.find("cloud_fuse") != string::npos) {
        std::thread([userId]() {
            HandleStartMove(userId);
        }).detach();
    }
#ifdef HICOLLIE_ENABLE
    XCollieHelper::CancelTimer(xcollieId);
#endif
    return E_OK;
}
} // namespace CloudFile
} // namespace FileManagement
} // namespace OHOS
