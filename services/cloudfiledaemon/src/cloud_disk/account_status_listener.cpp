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

#include "account_status_listener.h"

#include "common_event_manager.h"
#include "common_event_support.h"
#include "account_status.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement {
namespace CloudDisk {

AccountStatusSubscriber::AccountStatusSubscriber(const EventFwk::CommonEventSubscribeInfo &subscribeInfo)
    : EventFwk::CommonEventSubscriber(subscribeInfo)
{
}

void AccountStatusSubscriber::OnReceiveEvent(const EventFwk::CommonEventData &eventData)
{
    auto action = eventData.GetWant().GetAction();
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_HWID_LOGIN) {
        LOGI("Account Login!");
        AccountStatus::SetAccountState(AccountStatus::AccountState::ACCOUNT_LOGIN);
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_HWID_LOGOUT) {
        LOGI("Account Logout!");
        AccountStatus::SetAccountState(AccountStatus::AccountState::ACCOUNT_LOGOUT);
    }
}

AccountStatusListener::~AccountStatusListener()
{
    Stop();
}

void AccountStatusListener::Start()
{
    /* subscribe Account login and logout status */
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_HWID_LOGIN);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_HWID_LOGOUT);
    EventFwk::CommonEventSubscribeInfo info(matchingSkills);
    commonEventSubscriber_ = std::make_shared<AccountStatusSubscriber>(info);
    auto subRet = EventFwk::CommonEventManager::SubscribeCommonEvent(commonEventSubscriber_);
    LOGI("Subscriber end, SubscribeResult = %{public}d", subRet);
}

void AccountStatusListener::Stop()
{
    if (commonEventSubscriber_ != nullptr) {
        EventFwk::CommonEventManager::UnSubscribeCommonEvent(commonEventSubscriber_);
        commonEventSubscriber_ = nullptr;
    }
}
} // namespace CloudDisk
} // namespace FileManagement
} // namespace OHOS