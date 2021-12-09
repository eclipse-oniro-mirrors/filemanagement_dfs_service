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

int32_t ServiceProxy::GetBundleDistributedDir(const std::string &dirName)
{
    int32_t error = GET_DISTRIBUTEDFILE_DISTRIBUTED_DIR_FAIL;

    MessageOption option;
    MessageParcel dataParcel;
    MessageParcel replyParcel;
    if (!dataParcel.WriteInterfaceToken(ServiceProxy::GetDescriptor())) {
        LOGE("write descriptor failed");
        return DISTRIBUTEDFILE_WRITE_DESCRIPTOR_TOKEN_FAIL;
    }
    dataParcel.WriteString(dirName);
    if (Remote() == nullptr) {
        LOGE("Remote object address is null");
        return DISTRIBUTEDFILE_REMOTE_ADDRESS_IS_NULL;
    }
    error = Remote()->SendRequest(GET_BUNDLE_DISTRIBUTED_DIR, dataParcel, replyParcel, option);
    if (error != DISTRIBUTEDFILE_NO_ERROR) {
        LOGE("Function GetBundleDistributedDir! errCode:%{public}d", error);
        return DISTRIBUTEDFILE_CONNECT_SYSTEM_ABILITY_STUB_FAIL;
    }

    return replyParcel.ReadInt32();
}

int32_t ServiceProxy::RemoveBundleDistributedDirs(const std::string &dirName)
{
    int32_t error = REMOVE_DISTRIBUTEDFILE_DISTRIBUTEDDIRS_FAIL;
    MessageOption option;
    MessageParcel dataParcel;
    MessageParcel replyParcel;
    if (!dataParcel.WriteInterfaceToken(ServiceProxy::GetDescriptor())) {
        LOGE("write descriptor failed");
        return DISTRIBUTEDFILE_WRITE_DESCRIPTOR_TOKEN_FAIL;
    }

    dataParcel.WriteString(dirName);
    if (Remote() == nullptr) {
        LOGE("Remote object address is null");
        return DISTRIBUTEDFILE_REMOTE_ADDRESS_IS_NULL;
    }

    error = Remote()->SendRequest(REMOVE_BUNDLE_DISTRIBUTED_DIRS, dataParcel, replyParcel, option);
    if (error != DISTRIBUTEDFILE_NO_ERROR) {
        LOGE("Function RemoveBundleDistributedDirs! errCode:%{public}d", error);
        return DISTRIBUTEDFILE_CONNECT_SYSTEM_ABILITY_STUB_FAIL;
    }

    return replyParcel.ReadInt32();
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS