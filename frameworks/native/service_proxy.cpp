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

int32_t ServiceProxy::SendFile(const std::string &cid,
                               const std::vector<std::string> &sourceFileList,
                               const std::vector<std::string> &destinationFileList,
                               const uint32_t fileCount)
{
    LOGI("xhl sendFile enter");
    int32_t result = SEND_FILE_FAIL;
    MessageOption option;
    MessageParcel dataParcel;
    MessageParcel replyParcel;

    dataParcel.WriteString(cid);
    int32_t sourceListNumber = sourceFileList.size();
    dataParcel.WriteInt32(sourceListNumber);
    for (int32_t index = 0; index < sourceListNumber; ++index) {
        dataParcel.WriteString(sourceFileList.at(index));
    }
    int32_t destinationListNumber = destinationFileList.size();
    dataParcel.WriteInt32(destinationListNumber);
    for (int32_t index = 0; index < destinationListNumber; ++index) {
        dataParcel.WriteString(destinationFileList.at(index));
    }
    dataParcel.WriteUint32(fileCount);

    if (Remote() == nullptr) {
        LOGE("Remote object address is null");
        return DISTRIBUTEDFILE_REMOTE_ADDRESS_IS_NULL;
    }

    result = Remote()->SendRequest(SEND_FILE_DISTRIBUTED, dataParcel, replyParcel, option);
    if (result != DISTRIBUTEDFILE_NO_ERROR) {
        LOGE("Function RemoveBundleDistributedDirs! errCode:%{public}d", result);
        return DISTRIBUTEDFILE_CONNECT_SYSTEM_ABILITY_STUB_FAIL;
    }
    LOGE("xhl sendfile sendRequest done %{public}d", result);

    return replyParcel.ReadInt32();
}

int32_t ServiceProxy::sendTest()
{
    LOGE("xhl sendTest enter");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int ret = Remote()->SendRequest(TEST_CODE, data, reply, option);
    LOGE("xhl sendTest sendrequest done %{public}d", ret);
    return ret;
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS