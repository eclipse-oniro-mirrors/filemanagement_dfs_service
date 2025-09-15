/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "multiuser/os_account_observer.h"

#include "common_event_manager.h"
#include "common_event_support.h"
#include "device/device_manager_agent.h"
#include "dfsu_mount_argument_descriptors.h"
#include "os_account_manager.h"
#include "utils_log.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
using namespace std;
namespace {
static const std::string SAME_ACCOUNT = "account";
} // namespace

OsAccountObserver::OsAccountObserver(const EventFwk::CommonEventSubscribeInfo &subscribeInfo)
    : EventFwk::CommonEventSubscriber(subscribeInfo)
{
    LOGI("init first to create network of default user");
    lock_guard<mutex> lock(serializer_);
    curUsrId_ = GetCurrentUserId();
    if (curUsrId_ == INVALID_USER_ID) {
        LOGE("GetCurrentUserId Fail");
        return;
    }
    AddMPInfo(curUsrId_, SAME_ACCOUNT);
    auto dm = DeviceManagerAgent::GetInstance();
    dm->Recv(make_unique<DfsuCmd<DeviceManagerAgent>>(&DeviceManagerAgent::InitDeviceInfos));
    LOGI("init first to create network of user %{public}d, done", curUsrId_);
}

OsAccountObserver::~OsAccountObserver()
{
}

void OsAccountObserver::AddMPInfo(const int id, const std::string &relativePath)
{
    auto smp = make_shared<MountPoint>(Utils::DfsuMountArgumentDescriptors::Alpha(id, relativePath));
    char resolvedPath[PATH_MAX] = {'\0'};
    if (smp == nullptr || realpath(smp->GetMountArgument().GetCtrlPath().c_str(), resolvedPath) == nullptr) {
        LOGI("user id: %{public}d cannot join group", id);
        needAddUserId_.store(id);
        return;
    }
    auto dm = DeviceManagerAgent::GetInstance();
    dm->Recv(make_unique<DfsuCmd<DeviceManagerAgent, weak_ptr<MountPoint>>>(&DeviceManagerAgent::JoinGroup, smp));
    mountPoints_[id].emplace_back(smp);
}

void OsAccountObserver::OnReceiveEvent(const EventFwk::CommonEventData &eventData)
{
    const AAFwk::Want& want = eventData.GetWant();
    std::string action = want.GetAction();
    LOGI("AccountSubscriber: OnReceiveEvent action:%{public}s.", action.c_str());
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED) {
        int32_t id = eventData.GetCode();
        LOGI("user id changed to %{public}d", id);
        lock_guard<mutex> lock(serializer_);
        if (curUsrId_ != -1 && curUsrId_ != id) {
            // first stop curUsrId_ network
            RemoveMPInfo(curUsrId_);
        } else if (curUsrId_ != -1 && curUsrId_ == id) {
            return;
        }

        // then start new network
        curUsrId_ = id;
        AddMPInfo(id, SAME_ACCOUNT);
        LOGI("user id %{public}d, add network done", curUsrId_);
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_UNLOCKED) {
        int32_t id = eventData.GetCode();
        LOGI("user id: %{public}d unlocked", id);
        lock_guard<mutex> lock(serializer_);
        if (needAddUserId_.load() == id) {
            LOGI("user id: %{public}d now begin to join group", id);
            AddMPInfo(id, SAME_ACCOUNT);
        }
        needAddUserId_.store(-1);
    } else {
        LOGE("no expect action");
    }
}

void OsAccountObserver::RemoveMPInfo(const int id)
{
    auto iter = mountPoints_.find(id);
    if (iter == mountPoints_.end()) {
        LOGE("user id %{public}d not find in map", curUsrId_);
        return;
    }

    auto dm = DeviceManagerAgent::GetInstance();
    for (auto smp : iter->second) {
        dm->Recv(make_unique<DfsuCmd<DeviceManagerAgent, shared_ptr<MountPoint>>>(&DeviceManagerAgent::QuitGroup, smp));
    }
    mountPoints_.erase(iter);

    LOGE("remove mount info of user id %{public}d", id);
}

int32_t OsAccountObserver::GetCurrentUserId()
{
    std::vector<int32_t> userIds{};
    auto ret = AccountSA::OsAccountManager::QueryActiveOsAccountIds(userIds);
    if (ret != NO_ERROR || userIds.empty()) {
        LOGE("query active os account id failed, ret = %{public}d", ret);
        return INVALID_USER_ID;
    }
    return userIds[0];
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
