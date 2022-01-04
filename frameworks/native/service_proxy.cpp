/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "service_proxy.h"
#include "utils_log.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
ServiceProxy::ServiceProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IDistributedFileService>(impl) {}
ServiceProxy::~ServiceProxy() {}

int32_t ServiceProxy::SendFile(int32_t sessionId, const std::string &sourceFileList,
        const std::string &destinationFileList, uint32_t fileCount)
{
    int32_t error = SEND_FILE_FAIL;
    MessageOption option;
    MessageParcel dataParcel;
    MessageParcel replyParcel;
    if (!dataParcel.WriteInterfaceToken(ServiceProxy::GetDescriptor())) {
        LOGE("write descriptor failed");
        return SEND_FILE_DISTRIBUTED_DESCRIPTION_FAIL;
    }

    dataParcel.WriteInt32(sessionId);
    dataParcel.WriteString(sourceFileList);
    dataParcel.WriteString(destinationFileList);
    dataParcel.WriteUint32(fileCount);
    if (Remote() == nullptr) {
        LOGE("Remote object address is null");
        return DISTRIBUTEDFILE_REMOTE_ADDRESS_IS_NULL;
    }

    error = Remote()->SendRequest(SEND_FILE_DISTRIBUTED, dataParcel, replyParcel, option);
    if (error != DISTRIBUTEDFILE_NO_ERROR) {
        LOGE("Function RemoveBundleDistributedDirs! errCode:%{public}d", error);
        return DISTRIBUTEDFILE_CONNECT_SYSTEM_ABILITY_STUB_FAIL;
    }

    return replyParcel.ReadInt32();
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS