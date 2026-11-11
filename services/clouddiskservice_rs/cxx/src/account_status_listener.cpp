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

#include "account_status_listener.h"

#include <set>
#include <utility>

#include "account.rs.h"
#include "os_account_manager.h"
#include "os_account_subscriber.h"

namespace OHOS::FileManagement::CloudDiskService {
using AccountSA::OsAccountManager;
using AccountSA::OsAccountState;

class RustAccountStatusSubscriber final : public AccountSA::OsAccountSubscriber {
public:
    explicit RustAccountStatusSubscriber(rust::Box<AccountStatusCallback> callback)
        : OsAccountSubscriber(AccountSA::OsAccountSubscribeInfo(
            std::set<OsAccountState>{OsAccountState::STOPPED, OsAccountState::SWITCHED})),
          callback_(std::move(callback))
    {
    }

    void OnStateChanged(const AccountSA::OsAccountStateData &data) override
    {
        if (data.state == OsAccountState::SWITCHED || data.state == OsAccountState::STOPPED) {
            callback_->on_state_changed(data.state == OsAccountState::SWITCHED, data.toId);
        }
    }

    void Deactivate()
    {
        callback_->deactivate();
    }

private:
    rust::Box<AccountStatusCallback> callback_;
};

AccountStatusSubscription::AccountStatusSubscription(std::shared_ptr<RustAccountStatusSubscriber> subscriber)
    : subscriber_(std::move(subscriber))
{
}

AccountStatusSubscription::~AccountStatusSubscription()
{
    Stop();
}

int32_t AccountStatusSubscription::Stop()
{
    if (subscriber_ == nullptr) {
        return 0;
    }
    subscriber_->Deactivate();
    int32_t ret = OsAccountManager::UnsubscribeOsAccount(subscriber_);
    subscriber_.reset();
    return ret;
}

std::unique_ptr<AccountStatusSubscription> SubscribeAccountStatus(
    rust::Box<AccountStatusCallback> callback, int32_t &result)
{
    auto subscriber = std::make_shared<RustAccountStatusSubscriber>(std::move(callback));
    result = OsAccountManager::SubscribeOsAccount(subscriber);
    if (result != 0) {
        subscriber->Deactivate();
        return nullptr;
    }
    return std::make_unique<AccountStatusSubscription>(std::move(subscriber));
}
} // namespace OHOS::FileManagement::CloudDiskService
