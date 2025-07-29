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

#include "cloud_status.h"

#include "cloud_file_kit.h"
#include "dfs_error.h"
#include "utils_log.h"
#include "settings_data_manager.h"

namespace OHOS::FileManagement::CloudSync {
int32_t CloudStatus::GetCurrentCloudInfo(const std::string &bundleName, const int32_t userId)
{
    auto instance = CloudFile::CloudFileKit::GetInstance();
    if (instance == nullptr) {
        LOGE("get cloud file helper instance failed");
        return E_NULLPTR;
    }

    int32_t ret = instance->GetCloudUserInfo(userId, userInfo_);
    if (ret != E_OK) {
        LOGE("GetCloudUserInfo failed");
        return ret;
    }

    bool switchStatus = false;
    SwitchStatus curSwitch = SettingsDataManager::GetSwitchStatusByCache();
    if (bundleName == HDC_BUNDLE_NAME) {
        switchStatus = curSwitch == SwitchStatus::AI_FAMILY;
    } else if (bundleName == GALLERY_BUNDLE_NAME) {
        switchStatus = curSwitch == SwitchStatus::CLOUD_SPACE;
    } else {
        ret = instance->GetAppSwitchStatus(bundleName, userId, switchStatus);
        if (ret != E_OK) {
            LOGE("GetAppSwitchStatus failed");
            return ret;
        }
    }
    /* insert key-value */
    appSwitches_.insert(std::make_pair(bundleName, switchStatus));
    userId_ = userId;
    return E_OK;
}

std::pair<uint64_t, uint64_t> CloudStatus::GetCurrentSpaceInfo(const int32_t userId, const std::string &bundleName)
{
    auto instance = CloudFile::CloudFileKit::GetInstance();
    return instance->GetSpaceInfo(userId, bundleName);
}

bool CloudStatus::IsCloudStatusOkay(const std::string &bundleName, const int32_t userId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    /* User switching */
    if (userId_ != userId) {
        appSwitches_.erase(bundleName);
    }

    /* Obtain cloud information only during first sync */
    auto iter = appSwitches_.find(bundleName);
    if (iter == appSwitches_.end()) {
        LOGI("appSwitches unknown, bundleName: %{private}s, userId: %{public}d", bundleName.c_str(), userId);
        int32_t ret = GetCurrentCloudInfo(bundleName, userId);
        if (ret != E_OK) {
            return false;
        }
    }

    LOGI("bundleName: %{private}s, cloudSatus: %{public}d, switcheStatus: %{public}d", bundleName.c_str(),
         userInfo_.enableCloud, appSwitches_[bundleName]);
    return appSwitches_[bundleName];
}

int32_t CloudStatus::ChangeAppSwitch(const std::string &bundleName, const int32_t userId, bool appSwitchStatus)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (appSwitchStatus == true) {
        auto iter = appSwitches_.find(bundleName);
        if (iter != appSwitches_.end()) {
            LOGI("change app swtich, originStatus:%{public}d, currentStatus:%{public}d", appSwitches_[bundleName],
                 appSwitchStatus);
            appSwitches_[bundleName] = appSwitchStatus;
        }
    } else {
        /* Actively obtaining cloud information when next sync */
        appSwitches_.erase(bundleName);
    }

    return E_OK;
}

bool CloudStatus::IsAccountIdChanged(const std::string &accountId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if ((userInfo_.accountId != "") && (userInfo_.accountId != accountId)) {
        /* accountId Changed, clear init flag */
        appSwitches_.clear();
        return true;
    }
    return false;
}
} // namespace OHOS::FileManagement::CloudSync