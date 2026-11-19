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
#include "cloud_disk_service_syncfolder.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "cloud_disk_common.h"
#include "cloud_disk_service_error.h"
#include "cloud_disk_service_logfile.h"
#include "dfs_error.h"

namespace OHOS::FileManagement::CloudDiskService {
namespace Test {
// Provided by mock/cloud_disk_service_logfile_mgr_mock.cpp
void SetLogFileMgrPraseRequestResult(int32_t ret);
void SetLogFileMgrChangesData(const std::vector<struct ChangeData> &data);
void ResetLogFileMgrMock();
int32_t GetLogFileMgrProduceRequestCount();
OperationType GetLogFileMgrLastProduceOperateType();
int32_t GetLogFileMgrLastProduceUserId();
uint32_t GetLogFileMgrLastProduceSyncFolderIndex();
} // namespace Test
} // namespace OHOS::FileManagement::CloudDiskService

namespace OHOS::FileManagement::CloudDiskService::Test {
using namespace testing;
using namespace testing::ext;
using namespace std;

class CloudDiskServiceSyncFolderTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void CloudDiskServiceSyncFolderTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
}

void CloudDiskServiceSyncFolderTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
}

void CloudDiskServiceSyncFolderTest::SetUp()
{
    ResetLogFileMgrMock();
}

void CloudDiskServiceSyncFolderTest::TearDown()
{
    ResetLogFileMgrMock();
}

static vector<ChangeData> BuildChangesData(const vector<OperationType> &types)
{
    vector<ChangeData> data;
    for (const auto &type : types) {
        ChangeData item;
        item.operationType = type;
        item.fileId = "file-" + to_string(static_cast<int>(type));
        data.push_back(item);
    }
    return data;
}

/**
 * @tc.name: RegisterSyncFolderTest001
 * @tc.desc: Verify the RegisterSyncFolder function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, RegisterSyncFolderTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RegisterSyncFolderTest001 start";
    try {
        int32_t userId = 1;
        uint32_t syncFolderIndex = 1;
        string path = "path";
        CloudDiskServiceSyncFolder::RegisterSyncFolder(userId, syncFolderIndex, path);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RegisterSyncFolderTest001 failed";
    }
    GTEST_LOG_(INFO) << "RegisterSyncFolderTest001 end";
}

/**
 * @tc.name: UnRegisterSyncFolderTest001
 * @tc.desc: Verify the UnRegisterSyncFolder function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, UnRegisterSyncFolderTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnRegisterSyncFolderTest001 start";
    try {
        int32_t userId = 1;
        uint32_t syncFolderIndex = 1;
        CloudDiskServiceSyncFolder::UnRegisterSyncFolder(userId, syncFolderIndex);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UnRegisterSyncFolderTest001 failed";
    }
    GTEST_LOG_(INFO) << "UnRegisterSyncFolderTest001 end";
}

/**
 * @tc.name: RegisterSyncFolderChangesTest001
 * @tc.desc: Verify the RegisterSyncFolderChanges function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, RegisterSyncFolderChangesTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RegisterSyncFolderChangesTest001 start";
    try {
        int32_t userId = 1;
        uint32_t syncFolderIndex = 1;
        CloudDiskServiceSyncFolder::RegisterSyncFolderChanges(userId, syncFolderIndex);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RegisterSyncFolderChangesTest001 failed";
    }
    GTEST_LOG_(INFO) << "RegisterSyncFolderChangesTest001 end";
}

/**
 * @tc.name: UnRegisterSyncFolderChangesTest001
 * @tc.desc: Verify the UnRegisterSyncFolderChanges function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, UnRegisterSyncFolderChangesTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnRegisterSyncFolderChangesTest001 start";
    try {
        int32_t userId = 1;
        uint32_t syncFolderIndex = 1;
        CloudDiskServiceSyncFolder::UnRegisterSyncFolderChanges(userId, syncFolderIndex);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UnRegisterSyncFolderChangesTest001 failed";
    }
    GTEST_LOG_(INFO) << "UnRegisterSyncFolderChangesTest001 end";
}

/**
 * @tc.name: SetSyncFolderChangesTest001
 * @tc.desc: Verify the SetSyncFolderChanges function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, SetSyncFolderChangesTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetSyncFolderChangesTest001 start";
    try {
        EventInfo eventInfo;
        CloudDiskServiceSyncFolder::SetSyncFolderChanges(eventInfo);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SetSyncFolderChangesTest001 failed";
    }
    GTEST_LOG_(INFO) << "SetSyncFolderChangesTest001 end";
}

/**
 * @tc.name: SetSyncFolderChangesTest002
 * @tc.desc: Verify SetSyncFolderChanges forwards FILE_OPEN EventInfo to LogFileMgr::ProduceRequest
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, SetSyncFolderChangesTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetSyncFolderChangesTest002 start";
    try {
        EventInfo eventInfo(100, 200, OperationType::FILE_OPEN, "/data/service/el2/100/Docs/test.txt");
        int32_t ret = CloudDiskServiceSyncFolder::SetSyncFolderChanges(eventInfo);
        EXPECT_EQ(ret, E_OK);
        EXPECT_EQ(GetLogFileMgrProduceRequestCount(), 1);
        EXPECT_EQ(GetLogFileMgrLastProduceOperateType(), OperationType::FILE_OPEN);
        EXPECT_EQ(GetLogFileMgrLastProduceUserId(), 100);
        EXPECT_EQ(GetLogFileMgrLastProduceSyncFolderIndex(), 200U);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SetSyncFolderChangesTest002 failed";
    }
    GTEST_LOG_(INFO) << "SetSyncFolderChangesTest002 end";
}

/**
 * @tc.name: GetSyncFolderChangesTest001
 * @tc.desc: Verify the GetSyncFolderChanges function returns E_OK with empty changes
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, GetSyncFolderChangesTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesTest001 start";
    try {
        int32_t userId = 1;
        uint32_t syncFolderIndex = 1;
        uint64_t start = 1;
        uint64_t count = 1;
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChanges(
            userId, syncFolderIndex, start, count, changesResult);
        EXPECT_EQ(ret, E_OK);
        EXPECT_TRUE(changesResult.changesData.empty());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesTest001 end";
}

/**
 * @tc.name: GetSyncFolderChangesTest002
 * @tc.desc: Verify legacy GetSyncFolderChanges filters FILE_OPEN events out of the result
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, GetSyncFolderChangesTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesTest002 start";
    try {
        SetLogFileMgrChangesData(BuildChangesData(
            {OperationType::FILE_OPEN, OperationType::CREATE, OperationType::FILE_OPEN,
             OperationType::MOVE_TO}));
        int32_t userId = 1;
        uint32_t syncFolderIndex = 1;
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChanges(
            userId, syncFolderIndex, 0, 10, changesResult);
        EXPECT_EQ(ret, E_OK);
        ASSERT_EQ(changesResult.changesData.size(), 2U);
        EXPECT_EQ(changesResult.changesData[0].operationType, OperationType::CREATE);
        EXPECT_EQ(changesResult.changesData[1].operationType, OperationType::MOVE_TO);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesTest002 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesTest002 end";
}

/**
 * @tc.name: GetSyncFolderChangesTest003
 * @tc.desc: Verify legacy GetSyncFolderChanges returns empty result when all events are FILE_OPEN
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, GetSyncFolderChangesTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesTest003 start";
    try {
        SetLogFileMgrChangesData(BuildChangesData({OperationType::FILE_OPEN, OperationType::FILE_OPEN}));
        int32_t userId = 1;
        uint32_t syncFolderIndex = 1;
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChanges(
            userId, syncFolderIndex, 0, 10, changesResult);
        EXPECT_EQ(ret, E_OK);
        EXPECT_TRUE(changesResult.changesData.empty());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesTest003 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesTest003 end";
}

/**
 * @tc.name: GetSyncFolderChangesTest004
 * @tc.desc: Verify legacy GetSyncFolderChanges propagates PraseRequest failure
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, GetSyncFolderChangesTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesTest004 start";
    try {
        SetLogFileMgrPraseRequestResult(E_INVALID_CHANGE_SEQUENCE);
        int32_t userId = 1;
        uint32_t syncFolderIndex = 1;
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChanges(
            userId, syncFolderIndex, 0, 10, changesResult);
        EXPECT_EQ(ret, E_INVALID_CHANGE_SEQUENCE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesTest004 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesTest004 end";
}

/**
 * @tc.name: CloudDiskServiceClearAllTest001
 * @tc.desc: Verify the CloudDiskServiceClearAll function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, CloudDiskServiceClearAllTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "CloudDiskServiceClearAllTest001 start";
    try {
        CloudDiskServiceSyncFolder::CloudDiskServiceClearAll();
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "CloudDiskServiceClearAllTest001 failed";
    }
    GTEST_LOG_(INFO) << "CloudDiskServiceClearAllTest001 end";
}

/**
 * @tc.name: GetSyncFolderChangesWithEventsTest001
 * @tc.desc: Verify GetSyncFolderChangesWithEvents keeps only events matching operationTypes
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, GetSyncFolderChangesWithEventsTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest001 start";
    try {
        SetLogFileMgrChangesData(BuildChangesData(
            {OperationType::FILE_OPEN, OperationType::CREATE, OperationType::DELETE,
             OperationType::MOVE_FROM, OperationType::FILE_OPEN}));
        int32_t userId = 1;
        uint32_t syncFolderIndex = 1;
        set<OperationType> operationTypes = {OperationType::FILE_OPEN, OperationType::CREATE};
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChangesWithEvents(
            userId, syncFolderIndex, 0, 10, operationTypes, changesResult);
        EXPECT_EQ(ret, E_OK);
        ASSERT_EQ(changesResult.changesData.size(), 3U);
        EXPECT_EQ(changesResult.changesData[0].operationType, OperationType::FILE_OPEN);
        EXPECT_EQ(changesResult.changesData[1].operationType, OperationType::CREATE);
        EXPECT_EQ(changesResult.changesData[2].operationType, OperationType::FILE_OPEN);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest001 end";
}

/**
 * @tc.name: GetSyncFolderChangesWithEventsTest002
 * @tc.desc: Verify GetSyncFolderChangesWithEvents drops events not matching operationTypes
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, GetSyncFolderChangesWithEventsTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest002 start";
    try {
        SetLogFileMgrChangesData(BuildChangesData({OperationType::CREATE, OperationType::DELETE}));
        int32_t userId = 1;
        uint32_t syncFolderIndex = 1;
        set<OperationType> operationTypes = {OperationType::FILE_OPEN};
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChangesWithEvents(
            userId, syncFolderIndex, 0, 10, operationTypes, changesResult);
        EXPECT_EQ(ret, E_OK);
        EXPECT_TRUE(changesResult.changesData.empty());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest002 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest002 end";
}

/**
 * @tc.name: GetSyncFolderChangesWithEventsTest003
 * @tc.desc: Verify GetSyncFolderChangesWithEvents with empty operationTypes set keeps nothing
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, GetSyncFolderChangesWithEventsTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest003 start";
    try {
        SetLogFileMgrChangesData(BuildChangesData({OperationType::FILE_OPEN, OperationType::CREATE}));
        int32_t userId = 1;
        uint32_t syncFolderIndex = 1;
        set<OperationType> operationTypes;
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChangesWithEvents(
            userId, syncFolderIndex, 0, 10, operationTypes, changesResult);
        EXPECT_EQ(ret, E_OK);
        EXPECT_TRUE(changesResult.changesData.empty());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest003 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest003 end";
}

/**
 * @tc.name: GetSyncFolderChangesWithEventsTest004
 * @tc.desc: Verify GetSyncFolderChangesWithEvents with full operationTypes set keeps everything
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, GetSyncFolderChangesWithEventsTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest004 start";
    try {
        vector<OperationType> inputTypes = {OperationType::FILE_OPEN, OperationType::CREATE,
            OperationType::DELETE, OperationType::MOVE_FROM, OperationType::MOVE_TO,
            OperationType::CLOSE_WRITE, OperationType::OH_CLOUD_DISK_CLOSE_MODIFY};
        SetLogFileMgrChangesData(BuildChangesData(inputTypes));
        int32_t userId = 1;
        uint32_t syncFolderIndex = 1;
        set<OperationType> operationTypes(inputTypes.begin(), inputTypes.end());
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChangesWithEvents(
            userId, syncFolderIndex, 0, 10, operationTypes, changesResult);
        EXPECT_EQ(ret, E_OK);
        EXPECT_EQ(changesResult.changesData.size(), inputTypes.size());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest004 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest004 end";
}

/**
 * @tc.name: GetSyncFolderChangesWithEventsTest005
 * @tc.desc: Verify GetSyncFolderChangesWithEvents propagates PraseRequest failure
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceSyncFolderTest, GetSyncFolderChangesWithEventsTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest005 start";
    try {
        SetLogFileMgrPraseRequestResult(E_INVALID_CHANGE_SEQUENCE);
        int32_t userId = 1;
        uint32_t syncFolderIndex = 1;
        set<OperationType> operationTypes = {OperationType::FILE_OPEN};
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChangesWithEvents(
            userId, syncFolderIndex, 0, 10, operationTypes, changesResult);
        EXPECT_EQ(ret, E_INVALID_CHANGE_SEQUENCE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest005 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest005 end";
}
} // namespace OHOS::FileManagement::CloudDiskService::Test
