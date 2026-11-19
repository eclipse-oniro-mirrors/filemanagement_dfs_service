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

#include "cloud_disk_service_logfile.h"

#include <vector>

#include "dfs_error.h"

namespace OHOS::FileManagement::CloudDiskService {
namespace {
int32_t g_praseRequestRet = E_OK;
std::vector<struct ChangeData> g_changesData;
int32_t g_produceRequestCount = 0;
OperationType g_lastProduceOperateType = OperationType::OPERATION_MAX;
int32_t g_lastProduceUserId = -1;
uint32_t g_lastProduceSyncFolderIndex = 0;
} // namespace

namespace Test {
void SetLogFileMgrPraseRequestResult(int32_t ret)
{
    g_praseRequestRet = ret;
}

void SetLogFileMgrChangesData(const std::vector<struct ChangeData> &data)
{
    g_changesData = data;
}

void ResetLogFileMgrMock()
{
    g_praseRequestRet = E_OK;
    g_changesData.clear();
    g_produceRequestCount = 0;
    g_lastProduceOperateType = OperationType::OPERATION_MAX;
    g_lastProduceUserId = -1;
    g_lastProduceSyncFolderIndex = 0;
}

int32_t GetLogFileMgrProduceRequestCount()
{
    return g_produceRequestCount;
}

OperationType GetLogFileMgrLastProduceOperateType()
{
    return g_lastProduceOperateType;
}

int32_t GetLogFileMgrLastProduceUserId()
{
    return g_lastProduceUserId;
}

uint32_t GetLogFileMgrLastProduceSyncFolderIndex()
{
    return g_lastProduceSyncFolderIndex;
}
} // namespace Test

LogFileMgr &LogFileMgr::GetInstance()
{
    static LogFileMgr instance;
    return instance;
}

std::shared_ptr<CloudDiskServiceLogFile> LogFileMgr::GetCloudDiskServiceLogFile(const int32_t userId,
                                                                                const uint32_t syncFolderIndex)
{
    (void)userId;
    (void)syncFolderIndex;
    return nullptr;
}

int32_t LogFileMgr::ProduceRequest(const struct EventInfo &eventInfo)
{
    g_produceRequestCount++;
    g_lastProduceOperateType = eventInfo.operateType;
    g_lastProduceUserId = eventInfo.userId;
    g_lastProduceSyncFolderIndex = eventInfo.syncFolderIndex;
    return E_OK;
}

int32_t LogFileMgr::PraseRequest(const int32_t userId,
                                 const uint32_t syncFolderIndex,
                                 const uint64_t start,
                                 const uint64_t count,
                                 struct ChangesResult &changesResult)
{
    (void)userId;
    (void)syncFolderIndex;
    changesResult.changesData = g_changesData;
    changesResult.nextUsn = start + count;
    changesResult.isEof = true;
    return g_praseRequestRet;
}

int32_t LogFileMgr::RegisterSyncFolder(const int32_t userId, const uint32_t syncFolderIndex, const std::string &path)
{
    (void)userId;
    (void)syncFolderIndex;
    (void)path;
    return E_OK;
}

int32_t LogFileMgr::UnRegisterSyncFolder(const int32_t userId, const uint32_t syncFolderIndex)
{
    (void)userId;
    (void)syncFolderIndex;
    return E_OK;
}

void LogFileMgr::RegisterSyncFolderChanges(const int32_t userId, const uint32_t syncFolderIndex)
{
    (void)userId;
    (void)syncFolderIndex;
}

void LogFileMgr::UnRegisterSyncFolderChanges(const int32_t userId, const uint32_t syncFolderIndex)
{
    (void)userId;
    (void)syncFolderIndex;
}

void LogFileMgr::CloudDiskServiceClearAll()
{
}
} // namespace OHOS::FileManagement::CloudDiskService
