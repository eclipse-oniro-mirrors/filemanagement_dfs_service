/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#pragma once

#include <map>

#include "i_deamon.h"
#include "iremote_stub.h"

namespace OHOS {
namespace DistributedFile {
class DeamonStub : public IRemoteStub<IDeamon> {
public:
    DeamonStub();
    ~DeamonStub() = default;
    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    using DeamonInterface = int32_t (DeamonStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, DeamonInterface> opToInterfaceMap_;

    int32_t EchoServerDemoInner(MessageParcel &data, MessageParcel &reply);
};
} // namespace DistributedFile
} // namespace OHOS