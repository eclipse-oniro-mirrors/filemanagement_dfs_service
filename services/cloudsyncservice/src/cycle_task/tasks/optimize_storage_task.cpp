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

#include "optimize_storage_task.h"

#include <charconv>

#include "cloud_file_kit.h"
#include "cloud_file_fault_event.h"
#include "settings_data_manager.h"
#include "system_load.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement {
namespace CloudSync {
static const std::string VALID_DAYS_KEY = "validDays";
static const std::string DATA_AGING_POLICY_KEY = "dataAgingPolicy";
static const int32_t DEFAULT_AGING_DAYS = 30;
static const int32_t DEFAULT_DATA_AGING_POLICY = 1;
static bool GetInt32FromParam(const std::map<std::string, std::string>& param, const std::string& key, int32_t& val)
{
    auto found = param.find(key);
    if (found == param.end()) {
        LOGE("%{public}s not found in param", key.c_str());
        return false;
    }
    auto res = std::from_chars(found->second.data(), found->second.data() + found->second.size(), val);
    if (res.ec != std::errc{}) {
        LOGE("invalid int32_t value(%{public}s) for %{public}s ", found->second.c_str(), key.c_str());
        return false;
    }
    return true;
}

OptimizeStorageTask::OptimizeStorageTask(std::shared_ptr<CloudFile::DataSyncManager> dataSyncManager)
    : CycleTask("optimize_storage_task", {GALLERY_BUNDLE_NAME, HDC_BUNDLE_NAME}, ONE_DAY, dataSyncManager)
{
}

int32_t OptimizeStorageTask::RunTaskForBundle(int32_t userId, std::string bundleName)
{
    if (!SystemLoadStatus::IsSystemLoadAllowed()) {
        LOGE("OptimizeStorageTask::RunTaskForBundle system load is over hot");
        return E_STOP;
    }
    auto instance = CloudFile::CloudFileKit::GetInstance();
    if (instance == nullptr) {
        LOGE("get cloud file helper instance failed");
        return E_NULLPTR;
    }
    auto dataSyncManager_ = GetDataSyncManager();
    if (dataSyncManager_->CleanRemainFile(bundleName, userId) != E_OK) {
        LOGW(" clean reamin file fail");
    }

    int32_t agingDays = -1;
    int32_t agingPolicy = -1;

    std::map<std::string, std::string> param;
    auto ret = instance->GetAppConfigParams(userId, bundleName, param);
    if (ret != E_OK) {
        LOGE("GetAppConfigParams failed");
        return ret;
    }
    if (!GetInt32FromParam(param, VALID_DAYS_KEY, agingDays)) {
        LOGI("validDays invalid in param, use default");
        agingDays = DEFAULT_AGING_DAYS;
        CLOUD_SYNC_FAULT_REPORT({"", CloudFile::FaultScenarioCode::CLOUD_OPTIMMIZE_STORAGE,
            CloudFile::FaultType::DRIVERKIT, E_DATA,
            "validDays is " + param[VALID_DAYS_KEY]});
    }
    if (!GetInt32FromParam(param, DATA_AGING_POLICY_KEY, agingPolicy)) {
        LOGI("dataAgingPolicy invalid in param, use default");
        agingPolicy = DEFAULT_DATA_AGING_POLICY;
        CLOUD_SYNC_FAULT_REPORT({"", CloudFile::FaultScenarioCode::CLOUD_OPTIMMIZE_STORAGE,
            CloudFile::FaultType::DRIVERKIT, E_DATA,
            "dataAgingPolicy is " + param[DATA_AGING_POLICY_KEY]});
    }
    if (agingPolicy == 0) {
        return dataSyncManager_->OptimizeStorage(bundleName, userId, agingDays);
    }
    return E_OK;
}
} // namespace CloudSync
} // namespace FileManagement
} // namespace OHOS