/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef MOCK_SERVICE_PROXY_H
#define MOCK_SERVICE_PROXY_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "service_proxy.h"

namespace OHOS {
namespace FileManagement::CloudSync {

class IServiceProxy {
public:
    virtual sptr<ICloudSyncService> GetInstance() = 0;
    virtual ~IServiceProxy() = default;
    static inline std::shared_ptr<IServiceProxy> proxy_{nullptr};
};

class MockServiceProxy : public IServiceProxy {
public:
    MOCK_METHOD0(GetInstance, sptr<ICloudSyncService>());
};

sptr<ICloudSyncService> ServiceProxy::GetInstance()
{
    return IServiceProxy::proxy_->GetInstance();
}

void ServiceProxy::InvaildInstance() {}
} // FileManagement::CloudSync
} // OHOS
#endif // MOCK_SERVICE_PROXY_H