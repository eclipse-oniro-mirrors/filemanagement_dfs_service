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

#include "distributedfile_service_stub.h"

#include <ipc_skeleton.h>
#include <sys/stat.h>

#include "utils_log.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
DistributedFileServiceStub::DistributedFileServiceStub()
{
    memberFuncMap_[SEND_FILE_DISTRIBUTED] = &DistributedFileServiceStub::SendFileStub;
    memberFuncMap_[TEST_CODE] = &DistributedFileServiceStub::test;
}

DistributedFileServiceStub::~DistributedFileServiceStub()
{
    memberFuncMap_.clear();
}

int DistributedFileServiceStub::OnRemoteRequest(uint32_t code,
                                                MessageParcel &data,
                                                MessageParcel &reply,
                                                MessageOption &option)
{
    LOGD("xhl DistributedFileServiceStub : OnRemoteRequest enter, code %{public}d ", code);
    auto itFunc = memberFuncMap_.find(code);
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return (this->*memberFunc)(data, reply);
        }
    }

    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int DistributedFileServiceStub::test(MessageParcel &data, MessageParcel &reply)
{
    LOGD("xhl DistributedFileServiceStub : sendTest enter");
    sendTest();
    return 3;
}

int32_t DistributedFileServiceStub::SendFileStub(MessageParcel &data, MessageParcel &reply)
{
    std::string cid = data.ReadString();
    if (cid.empty()) {
        LOGE("DistributedFileServiceStub : Failed to get app device id, error: invalid device id");
        return DISTRIBUTEDFILE_DIR_NAME_IS_EMPTY;
    }

    int32_t sourceListNumber = data.ReadInt32();
    std::vector<std::string> srcList;
    for (int32_t index = 0; index < sourceListNumber; ++index) {
        srcList.push_back(data.ReadString());
    }
    std::vector<std::string> dstList;
    int32_t destinationListNumber = data.ReadInt32();
    for (int32_t index = 0; index < destinationListNumber; ++index) {
        dstList.push_back(data.ReadString());
    }
    uint32_t fileCount = data.ReadUint32();

    LOGI("DistributedFileServiceStub : cid %{public}s, srcList size %{public}d, dstList size %{public}d,"
        "fileCout %{public}d", cid.c_str(), sourceListNumber, destinationListNumber, fileCount);

    int32_t result = SendFile(cid, srcList, dstList, fileCount);
    LOGD("xhl DistributedFileServiceStub : SendFileStub result = %{public}d", result);
    if (!reply.WriteInt32(result)) {
        LOGE("fail to write parcel");
        return DISTRIBUTEDFILE_WRITE_REPLY_FAIL;
    }

    return result;
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS