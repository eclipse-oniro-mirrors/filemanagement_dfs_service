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

#include "ipc/deamon_stub.h"
#include "utils_log.h"

namespace OHOS {
namespace DistributedFile {
DeamonStub::DeamonStub()
{
    opToInterfaceMap_[DFS_DEAMON_CMD_ECHO] = &DeamonStub::EchoServerDemoInner;
}

int32_t DeamonStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    auto interfaceIndex = opToInterfaceMap_.find(code);
    if (interfaceIndex == opToInterfaceMap_.end() || !interfaceIndex->second) {
        LOGE("Cannot response request %d: unknown tranction", code);
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    return (this->*(interfaceIndex->second))(data, reply);
}

int32_t DeamonStub::EchoServerDemoInner(MessageParcel &data, MessageParcel &reply)
{
    LOGI("hello, world");
    return ERR_NONE;
}
} // namespace DistributedFile
} // namespace OHOS