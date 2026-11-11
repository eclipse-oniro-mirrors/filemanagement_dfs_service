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

#ifndef CLOUD_DISK_SERVICE_RS_ACCOUNT_STATUS_LISTENER_H
#define CLOUD_DISK_SERVICE_RS_ACCOUNT_STATUS_LISTENER_H

#include <cstdint>
#include <memory>

#include "cxx.h"

namespace OHOS::FileManagement::CloudDiskService {
struct AccountStatusCallback;
class RustAccountStatusSubscriber;

class AccountStatusSubscription {
public:
    explicit AccountStatusSubscription(std::shared_ptr<RustAccountStatusSubscriber> subscriber);
    ~AccountStatusSubscription();
    AccountStatusSubscription(const AccountStatusSubscription &) = delete;
    AccountStatusSubscription &operator=(const AccountStatusSubscription &) = delete;
    int32_t Stop();

private:
    std::shared_ptr<RustAccountStatusSubscriber> subscriber_;
};

std::unique_ptr<AccountStatusSubscription> SubscribeAccountStatus(
    rust::Box<AccountStatusCallback> callback, int32_t &result);
} // namespace OHOS::FileManagement::CloudDiskService

#endif // CLOUD_DISK_SERVICE_RS_ACCOUNT_STATUS_LISTENER_H
