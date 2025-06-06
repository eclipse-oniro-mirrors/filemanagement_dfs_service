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

#ifndef DISTRIBUTED_TRANS_LISTENER_H
#define DISTRIBUTED_TRANS_LISTENER_H

#include <condition_variable>

#include "file_trans_listener_stub.h"
#include "hmdfs_info.h"

constexpr int DFS_NONE = 0;
constexpr int DFS_SUCCESS = 1;
constexpr int DFS_FAILED = 2;
namespace OHOS {
namespace Storage {
namespace DistributedFile {
struct CopyEvent {
    int copyResult = DFS_NONE;
    int32_t errorCode = 0;
};

class TransListener : public Storage::DistributedFile::FileTransListenerStub {
public:
    using ProcessCallback = std::function<void (uint64_t processSize, uint64_t totalSize)>;
    explicit TransListener(const std::string &destUri, ProcessCallback &processCallback);
    virtual ~TransListener();

    int32_t OnFileReceive(uint64_t totalBytes, uint64_t processedBytes) override;
    int32_t OnFinished(const std::string &sessionName) override;
    int32_t OnFailed(const std::string &sessionName, int32_t errorCode) override;

    int32_t WaitForCopyResult();
    int32_t CopyToSandBox(const std::string &srcUri);
    std::string GetNetworkIdFromUri(const std::string &uri);
    int32_t Cancel(const std::string &srcUri, const std::string &destUri);
    int32_t GetErrCode() { return copyEvent_.errorCode; };
public:
    HmdfsInfo hmdfsInfo_{};

private:
    int32_t CreateTmpDir();
    void RmTmpDir();
    std::string CreateDfsCopyPath();
    std::string GetFileName(const std::string &path);

private:
    std::string disSandboxPath_;
    std::mutex cvMutex_;
    std::condition_variable cv_;
    ProcessCallback processCallback_;

    CopyEvent copyEvent_;
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
#endif // DISTRIBUTED_TRANS_LISTENER_H
