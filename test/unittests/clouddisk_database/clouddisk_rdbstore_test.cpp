/*
* Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

#include "cloud_file_utils.h"
#include "cloud_status.h"
#include "clouddisk_notify_utils_mock.h"
#include "clouddisk_rdb_utils.h"
#include "clouddisk_rdbstore.h"
#include "dfs_error.h"
#include "rdb_assistant.h"
#include "result_set_mock.h"
#include "transaction_mock.cpp"

namespace OHOS::FileManagement::CloudDisk::Test {
using namespace testing;
using namespace testing::ext;
using namespace std;
using namespace NativeRdb;

static const string BUNDLE_NAME = "com.ohos.photos";
static const int32_t USER_ID = 100;
class CloudDiskRdbStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static inline shared_ptr<CloudDiskRdbStore> clouddiskrdbStore_ = nullptr;
    static inline shared_ptr<AssistantMock> insMock = nullptr;
};

void CloudDiskRdbStoreTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
    insMock = make_shared<AssistantMock>();
    Assistant::ins = insMock;
    clouddiskrdbStore_ = make_shared<CloudDiskRdbStore>(BUNDLE_NAME, USER_ID);
}

void CloudDiskRdbStoreTest::TearDownTestCase(void)
{
    Assistant::ins = nullptr;
    insMock = nullptr;
    clouddiskrdbStore_ = nullptr;
    GTEST_LOG_(INFO) << "TearDownTestCase";
}

void CloudDiskRdbStoreTest::SetUp(void)
{
    GTEST_LOG_(INFO) << "SetUp";
}

void CloudDiskRdbStoreTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "TearDown";
}

/**
 * @tc.name: ReBuildDatabase
 * @tc.desc: Verify the CloudDiskRdbStore::ReBuildDatabase function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, ReBuildDatabaseTest1, TestSize.Level1)
{
    string databasePath = "/data";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*insMock, DeleteRdbStore(_)).WillOnce(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->ReBuildDatabase(databasePath);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: ReBuildDatabase
 * @tc.desc: Verify the CloudDiskRdbStore::ReBuildDatabase function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, ReBuildDatabaseTest2, TestSize.Level1)
{
    string databasePath = "/data";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*insMock, DeleteRdbStore(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(*insMock, GetRdbStore(_, _, _, _)).WillOnce(Return(nullptr));

    int32_t ret = clouddiskrdbStore_->ReBuildDatabase(databasePath);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: ReBuildDatabase
 * @tc.desc: Verify the CloudDiskRdbStore::ReBuildDatabase function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, ReBuildDatabaseTest3, TestSize.Level1)
{
    string databasePath = "/data";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*insMock, DeleteRdbStore(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(*insMock, GetRdbStore(_, _, _, _)).WillOnce(Return(rdb));

    int32_t ret = clouddiskrdbStore_->ReBuildDatabase(databasePath);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: RdbInit
 * @tc.desc: Verify the CloudDiskRdbStore::RdbInit function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RdbInitTest1, TestSize.Level1)
{
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*insMock, GetRdbStore(_, _, _, _)).WillOnce(Return(nullptr));

    int32_t ret = clouddiskrdbStore_->RdbInit();
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: RdbInit
 * @tc.desc: Verify the CloudDiskRdbStore::RdbInit function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RdbInitTest2, TestSize.Level1)
{
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*insMock, GetRdbStore(_, _, _, _)).WillOnce(Return(rdb));

    int32_t ret = clouddiskrdbStore_->RdbInit();
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: LookUp
 * @tc.desc: Verify the CloudDiskRdbStore::LookUp function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, LookUpTest1, TestSize.Level1)
{
    const std::string parentCloudId = "";
    const std::string fileName = "test";
    CloudDiskFileInfo info;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->LookUp(parentCloudId, fileName, info);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: LookUp
 * @tc.desc: Verify the CloudDiskRdbStore::LookUp function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, LookUpTest2, TestSize.Level1)
{
    const std::string parentCloudId = "100";
    const std::string fileName = "";
    CloudDiskFileInfo info;
    info.name = "mock";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->LookUp(parentCloudId, fileName, info);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: LookUp
 * @tc.desc: Verify the CloudDiskRdbStore::LookUp function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, LookUpTest3, TestSize.Level1)
{
    const std::string parentCloudId = "rootId";
    const std::string fileName = "test";
    CloudDiskFileInfo info;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->LookUp(parentCloudId, fileName, info);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: LookUp
 * @tc.desc: Verify the CloudDiskRdbStore::LookUp function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, LookUpTest4, TestSize.Level1)
{
    const std::string parentCloudId = "rootId";
    const std::string fileName = "test";
    CloudDiskFileInfo info;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetRow(_)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->LookUp(parentCloudId, fileName, info);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetAttr
 * @tc.desc: Verify the CloudDiskRdbStore::GetAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetAttrTest1, TestSize.Level1)
{
    const std::string cloudId = "";
    CloudDiskFileInfo info;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->GetAttr(cloudId, info);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetAttr
 * @tc.desc: Verify the CloudDiskRdbStore::GetAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetAttrTest2, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    CloudDiskFileInfo info;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->GetAttr(cloudId, info);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetAttr
 * @tc.desc: Verify the CloudDiskRdbStore::GetAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetAttrTest3, TestSize.Level1)
{
    const std::string cloudId = "100";
    CloudDiskFileInfo info;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->GetAttr(cloudId, info);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetAttr
 * @tc.desc: Verify the CloudDiskRdbStore::GetAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetAttrTest4, TestSize.Level1)
{
    const std::string cloudId = "100";
    CloudDiskFileInfo info;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetRow(_)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->GetAttr(cloudId, info);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: SetAttr
 * @tc.desc: Verify the CloudDiskRdbStore::SetAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SetAttrTest1, TestSize.Level1)
{
    const std::string fileName = "Test";
    const std::string parentCloudId = "100";
    const std::string cloudId = "";
    const unsigned long long size = 0;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->SetAttr(fileName, parentCloudId, cloudId, size);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: SetAttr
 * @tc.desc: Verify the CloudDiskRdbStore::SetAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SetAttrTest2, TestSize.Level1)
{
    const std::string fileName = "Test";
    const std::string parentCloudId = "100";
    const std::string cloudId = "rootId";
    const unsigned long long size = 0;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->SetAttr(fileName, parentCloudId, cloudId, size);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: SetAttr
 * @tc.desc: Verify the CloudDiskRdbStore::SetAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SetAttrTest3, TestSize.Level1)
{
    const std::string fileName = "mock";
    const std::string parentCloudId = "100";
    const std::string cloudId = "100";
    const unsigned long long size = 0;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));

    int32_t ret = clouddiskrdbStore_->SetAttr(fileName, parentCloudId, cloudId, size);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: SetAttr
 * @tc.desc: Verify the CloudDiskRdbStore::SetAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SetAttrTest4, TestSize.Level1)
{
    const std::string fileName = "mock";
    const std::string parentCloudId = "100";
    const std::string cloudId = "100";
    const unsigned long long size = 0;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->SetAttr(fileName, parentCloudId, cloudId, size);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: SetAttr
 * @tc.desc: Verify the CloudDiskRdbStore::SetAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SetAttrTest5, TestSize.Level1)
{
    const std::string fileName = "test";
    const std::string parentCloudId = "100";
    const std::string cloudId = "100";
    const unsigned long long size = 0;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->SetAttr(fileName, parentCloudId, cloudId, size);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: ReadDir
 * @tc.desc: Verify the CloudDiskRdbStore::ReadDir function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, ReadDirTest1, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    vector<CloudDiskFileInfo> infos;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->ReadDir(cloudId, infos);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: Create
 * @tc.desc: Verify the CloudDiskRdbStore::Create function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CreateTest1, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    const std::string parentCloudId = "100";
    const std::string fileName = "";
    bool noNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->Create(cloudId, parentCloudId, fileName, noNeedUpload);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: Create
 * @tc.desc: Verify the CloudDiskRdbStore::Create function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CreateTest2, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    const std::string parentCloudId = "100";
    const std::string fileName = "<|>";
    bool noNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->Create(cloudId, parentCloudId, fileName, noNeedUpload);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: Create
 * @tc.desc: Verify the CloudDiskRdbStore::Create function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CreateTest3, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    const std::string parentCloudId = "100";
    const std::string fileName = " test";
    bool noNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->Create(cloudId, parentCloudId, fileName, noNeedUpload);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: Create
 * @tc.desc: Verify the CloudDiskRdbStore::Create function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CreateTest4, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string parentCloudId = "";
    const std::string fileName = "test";
    bool noNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->Create(cloudId, parentCloudId, fileName, noNeedUpload);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: Create
 * @tc.desc: Verify the CloudDiskRdbStore::Create function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CreateTest5, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    const std::string parentCloudId = "100";
    const std::string fileName = "test";
    bool noNeedUpload = false;
    clouddiskrdbStore_->userId_ = 0;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->Create(cloudId, parentCloudId, fileName, noNeedUpload);
    EXPECT_EQ(ret, E_PATH);
}

/**
 * @tc.name: Create
 * @tc.desc: Verify the CloudDiskRdbStore::Create function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CreateTest6, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    const std::string parentCloudId = "100";
    const std::string fileName = "test";
    bool noNeedUpload = true;
    clouddiskrdbStore_->userId_ = 1;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Insert(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));

    int32_t ret = clouddiskrdbStore_->Create(cloudId, parentCloudId, fileName, noNeedUpload);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: Create
 * @tc.desc: Verify the CloudDiskRdbStore::Create function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CreateTest7, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    const std::string parentCloudId = "100";
    const std::string fileName = "mock";
    bool noNeedUpload = true;
    clouddiskrdbStore_->userId_ = 1;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Insert(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->Create(cloudId, parentCloudId, fileName, noNeedUpload);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: Create
 * @tc.desc: Verify the CloudDiskRdbStore::Create function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CreateTest8, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    const std::string parentCloudId = "100";
    const std::string fileName = "test";
    bool noNeedUpload = true;
    clouddiskrdbStore_->userId_ = 1;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Insert(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));

    int32_t ret = clouddiskrdbStore_->Create(cloudId, parentCloudId, fileName, noNeedUpload);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: MkDir
 * @tc.desc: Verify the CloudDiskRdbStore::MkDir function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, MkDirTest1, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string parentCloudId = "100";
    const std::string directoryName = "";
    bool noNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->MkDir(cloudId, parentCloudId, directoryName, noNeedUpload);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: MkDir
 * @tc.desc: Verify the CloudDiskRdbStore::MkDir function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, MkDirTest2, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string parentCloudId = "100";
    const std::string directoryName = ".trash";
    bool noNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->MkDir(cloudId, parentCloudId, directoryName, noNeedUpload);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: MkDir
 * @tc.desc: Verify the CloudDiskRdbStore::MkDir function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, MkDirTest3, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string parentCloudId = "";
    const std::string directoryName = "test";
    bool noNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->MkDir(cloudId, parentCloudId, directoryName, noNeedUpload);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: MkDir
 * @tc.desc: Verify the CloudDiskRdbStore::MkDir function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, MkDirTest4, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string parentCloudId = "100";
    const std::string directoryName = "mock";
    bool noNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Insert(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));

    int32_t ret = clouddiskrdbStore_->MkDir(cloudId, parentCloudId, directoryName, noNeedUpload);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: MkDir
 * @tc.desc: Verify the CloudDiskRdbStore::MkDir function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, MkDirTest5, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string parentCloudId = "100";
    const std::string directoryName = "mock";
    bool noNeedUpload = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Insert(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));
    
    int32_t ret = clouddiskrdbStore_->MkDir(cloudId, parentCloudId, directoryName, noNeedUpload);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: MkDir
 * @tc.desc: Verify the CloudDiskRdbStore::MkDir function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, MkDirTest6, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string parentCloudId = "100";
    const std::string directoryName = "test";
    bool noNeedUpload = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Insert(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));
    
    int32_t ret = clouddiskrdbStore_->MkDir(cloudId, parentCloudId, directoryName, noNeedUpload);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: Write
 * @tc.desc: Verify the CloudDiskRdbStore::Write function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, WriteTest1, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string parentCloudId = "100";
    const std::string fileName = "file";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->Write(fileName, parentCloudId, cloudId);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: Write
 * @tc.desc: Verify the CloudDiskRdbStore::Write function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, WriteTest2, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    const std::string fileName = "file";
    const std::string parentCloudId = "rootId";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->Write(fileName, parentCloudId, cloudId);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: Write
 * @tc.desc: Verify the CloudDiskRdbStore::Write function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, WriteTest3, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string fileName = "file";
    const std::string parentCloudId = "rootId";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->userId_ = 0;
    
    int32_t ret = clouddiskrdbStore_->Write(fileName, parentCloudId, cloudId);
    EXPECT_EQ(ret, E_PATH);
}

/**
 * @tc.name: Write
 * @tc.desc: Verify the CloudDiskRdbStore::Write function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, WriteTest4, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string fileName = "mock";
    const std::string parentCloudId = "rootId";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->userId_ = 1;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->Write(fileName, parentCloudId, cloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: Write
 * @tc.desc: Verify the CloudDiskRdbStore::Write function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, WriteTest5, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string fileName = "mock";
    const std::string parentCloudId = "rootId";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->userId_ = 1;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));

    int32_t ret = clouddiskrdbStore_->Write(fileName, parentCloudId, cloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: Write
 * @tc.desc: Verify the CloudDiskRdbStore::Write function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, WriteTest6, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string fileName = "mock";
    const std::string parentCloudId = "rootId";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->userId_ = 1;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->Write(fileName, parentCloudId, cloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: LocationSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::LocationSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, LocationSetXattrTest1, TestSize.Level1)
{
    const std::string name = "test";
    const std::string parentCloudId = "100";
    const std::string cloudId = "100";
    const std::string value = "4";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->LocationSetXattr(name, parentCloudId, cloudId, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: LocationSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::LocationSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, LocationSetXattrTest2, TestSize.Level1)
{
    const std::string name = "mock";
    const std::string parentCloudId = "100";
    const std::string cloudId = "100";
    const std::string value = "1";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));

    int32_t ret = clouddiskrdbStore_->LocationSetXattr(name, parentCloudId, cloudId, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: LocationSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::LocationSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, LocationSetXattrTest3, TestSize.Level1)
{
    const std::string name = "mock";
    const std::string parentCloudId = "100";
    const std::string cloudId = "100";
    const std::string value = "1";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->LocationSetXattr(name, parentCloudId, cloudId, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: LocationSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::LocationSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, LocationSetXattrTest4, TestSize.Level1)
{
    const std::string name = "test";
    const std::string parentCloudId = "100";
    const std::string cloudId = "100";
    const std::string value = "1";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));

    int32_t ret = clouddiskrdbStore_->LocationSetXattr(name, parentCloudId, cloudId, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: HasTHMSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::HasTHMSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, HasTHMSetXattrTest1, TestSize.Level1)
{
    const std::string name = "test";
    const std::string key = "100";
    const std::string cloudId = "100";
    const std::string value = "test";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->HasTHMSetXattr(name, key, cloudId, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: HasTHMSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::HasTHMSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, HasTHMSetXattrTest2, TestSize.Level1)
{
    const std::string name = "test";
    const std::string key = "100";
    const std::string cloudId = "100";
    const std::string value = "2";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->HasTHMSetXattr(name, key, cloudId, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: HasTHMSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::HasTHMSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, HasTHMSetXattrTest3, TestSize.Level1)
{
    const std::string name = "test";
    const std::string key = CLOUD_HAS_LCD;
    const std::string cloudId = "100";
    const std::string value = "0";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->HasTHMSetXattr(name, key, cloudId, value);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: HasTHMSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::HasTHMSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, HasTHMSetXattrTest4, TestSize.Level1)
{
    const std::string name = "test";
    const std::string key = CLOUD_HAS_THM;
    const std::string cloudId = "100";
    const std::string value = "0";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->HasTHMSetXattr(name, key, cloudId, value);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: HasTHMSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::HasTHMSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, HasTHMSetXattrTest5, TestSize.Level1)
{
    const std::string name = "test";
    const std::string key = CLOUD_HAS_LCD;
    const std::string cloudId = "100";
    const std::string value = "1";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->HasTHMSetXattr(name, key, cloudId, value);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: HasTHMSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::HasTHMSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, HasTHMSetXattrTest6, TestSize.Level1)
{
    const std::string name = "test";
    const std::string key = CLOUD_HAS_THM;
    const std::string cloudId = "100";
    const std::string value = "1";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->HasTHMSetXattr(name, key, cloudId, value);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: HasTHMSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::HasTHMSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, HasTHMSetXattrTest7, TestSize.Level1)
{
    const std::string name = "test";
    const std::string key = CLOUD_HAS_THM;
    const std::string cloudId = "100";
    const std::string value = "1";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));

    int32_t ret = clouddiskrdbStore_->HasTHMSetXattr(name, key, cloudId, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetRowId
 * @tc.desc: Verify the CloudDiskRdbStore::GetRowId function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetRowIdTest1, TestSize.Level1)
{
    const std::string cloudId = "100";
    int64_t rowId = 100;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->GetRowId(cloudId, rowId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetRowId
 * @tc.desc: Verify the CloudDiskRdbStore::GetRowId function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetRowIdTest2, TestSize.Level1)
{
    const std::string cloudId = "100";
    int64_t rowId = 100;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->GetRowId(cloudId, rowId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetRowId
 * @tc.desc: Verify the CloudDiskRdbStore::GetRowId function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetRowIdTest3, TestSize.Level1)
{
    const std::string cloudId = "100";
    int64_t rowId = 100;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->GetRowId(cloudId, rowId);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetParentCloudIdTest
 * @tc.desc: Verify the CloudDiskRdbStore::GetParentCloudIdTest function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetParentCloudIdTestTest1, TestSize.Level1)
{
    const std::string cloudId = "100";
    std::string parentCloudId = "mock";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->GetParentCloudId(cloudId, parentCloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetParentCloudIdTest
 * @tc.desc: Verify the CloudDiskRdbStore::GetParentCloudIdTest function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetParentCloudIdTestTest2, TestSize.Level1)
{
    const std::string cloudId = "100";
    std::string parentCloudId = "mock";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->GetParentCloudId(cloudId, parentCloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetParentCloudIdTest
 * @tc.desc: Verify the CloudDiskRdbStore::GetParentCloudIdTest function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetParentCloudIdTestTest3, TestSize.Level1)
{
    const std::string cloudId = "100";
    std::string parentCloudId = "mock";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->GetParentCloudId(cloudId, parentCloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetParentCloudIdTest
 * @tc.desc: Verify the CloudDiskRdbStore::GetParentCloudIdTest function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetParentCloudIdTestTest4, TestSize.Level1)
{
    const std::string cloudId = "100";
    std::string parentCloudId = "test";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->GetParentCloudId(cloudId, parentCloudId);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetSourcePath
 * @tc.desc: Verify the CloudDiskRdbStore::GetSourcePath function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetSourcePathTest1, TestSize.Level1)
{
    const std::string attr = R"({"srcPath": "/data/service"})";
    const std::string parentCloudId = "100";
    std::string sourcePath = "/data";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->GetSourcePath(attr, parentCloudId, sourcePath);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetSourcePath
 * @tc.desc: Verify the CloudDiskRdbStore::GetSourcePath function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetSourcePathTest2, TestSize.Level1)
{
    const std::string attr = R"({"srcPath": 100})";
    const std::string parentCloudId = "100";
    std::string sourcePath = "/data";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->GetSourcePath(attr, parentCloudId, sourcePath);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetSourcePath
 * @tc.desc: Verify the CloudDiskRdbStore::GetSourcePath function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetSourcePathTest3, TestSize.Level1)
{
    const std::string attr = "";
    const std::string parentCloudId = "100";
    std::string sourcePath = "/data";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->GetSourcePath(attr, parentCloudId, sourcePath);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetSourcePath
 * @tc.desc: Verify the CloudDiskRdbStore::GetSourcePath function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetSourcePathTest4, TestSize.Level1)
{
    const std::string attr = "";
    const std::string parentCloudId = "";
    std::string sourcePath = "/data";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->GetSourcePath(attr, parentCloudId, sourcePath);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetSourcePath
 * @tc.desc: Verify the CloudDiskRdbStore::GetSourcePath function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetSourcePathTest5, TestSize.Level1)
{
    const std::string attr = "";
    const std::string parentCloudId = "100";
    std::string sourcePath = "/data";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->GetSourcePath(attr, parentCloudId, sourcePath);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: SourcePathSetValue
 * @tc.desc: Verify the CloudDiskRdbStore::SourcePathSetValue function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SourcePathSetValueTest1, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string attr = "";
    ValuesBucket setXattr;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->SourcePathSetValue(cloudId, attr, setXattr);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: SourcePathSetValue
 * @tc.desc: Verify the CloudDiskRdbStore::SourcePathSetValue function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SourcePathSetValueTest2, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    const std::string attr = "";
    ValuesBucket setXattr;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->SourcePathSetValue(cloudId, attr, setXattr);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: SourcePathSetValue
 * @tc.desc: Verify the CloudDiskRdbStore::SourcePathSetValue function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SourcePathSetValueTest3, TestSize.Level1)
{
    const std::string cloudId = "test";
    const std::string attr = "";
    ValuesBucket setXattr;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();

    // GetCurNode return E_OK
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(), An<const std::vector<std::string> &>(), preCount))
        .WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetRow(_)).WillRepeatedly(Return(E_OK));

    // GetNotifyUri return E_INVAL_ARG
    int32_t ret = clouddiskrdbStore_->SourcePathSetValue(cloudId, attr, setXattr);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: SourcePathSetValue
 * @tc.desc: Verify the CloudDiskRdbStore::SourcePathSetValue function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SourcePathSetValueTest4, TestSize.Level1)
{
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();

    // GetCurNode return E_OK
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(), An<const std::vector<std::string> &>(), preCount))
        .WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetRow(_)).WillRepeatedly(Return(E_OK));

    // GetNotifyUri return E_OK
    clouddiskrdbStore_->rootId_ = "rootId";

    // jsonObject parse faild + jsonObject dump success
    const std::string cloudId = "test";
    const std::string attr = "";
    ValuesBucket setXattr;

    int32_t ret = clouddiskrdbStore_->SourcePathSetValue(cloudId, attr, setXattr);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: SourcePathSetValue
 * @tc.desc: Verify the CloudDiskRdbStore::SourcePathSetValue function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SourcePathSetValueTest5, TestSize.Level1)
{
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();

    // GetCurNode return E_OK
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(), An<const std::vector<std::string> &>(), preCount))
        .WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetRow(_)).WillRepeatedly(Return(E_OK));

    // GetNotifyUri return E_OK
    clouddiskrdbStore_->rootId_ = "rootId";

    // jsonObject parse success + jsonObject dump success
    const std::string cloudId = "test";
    const std::string attr = "{\"key\":\"value\"}";
    ValuesBucket setXattr;

    int32_t ret = clouddiskrdbStore_->SourcePathSetValue(cloudId, attr, setXattr);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: SourcePathSetValue
 * @tc.desc: Verify the CloudDiskRdbStore::SourcePathSetValue function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SourcePathSetValueTest6, TestSize.Level1)
{
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();

    // GetCurNode return E_OK
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(), An<const std::vector<std::string> &>(), preCount))
        .WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetRow(_)).WillRepeatedly(Return(E_OK));

    // GetNotifyUri return E_OK
    clouddiskrdbStore_->rootId_ = "rootId";

    // jsonObject parse success + jsonObject dump failed
    const std::string cloudId = "test";
    const std::string attr = "{\"srcPath\":\"test\"}";
    ValuesBucket setXattr;
    shared_ptr<CloudDiskNotifyUtilsMock> utilsMock = make_shared<CloudDiskNotifyUtilsMock>();
    ICloudDiskNotifyUtils::ins = utilsMock;
    EXPECT_CALL(*utilsMock, GetUriFromCache())
        .WillOnce(Return("file://com.test.hap/data/storage/el2/cloud/\xD6\xF8\xD0\xA1/text.txt"));

    int32_t ret = clouddiskrdbStore_->SourcePathSetValue(cloudId, attr, setXattr);
    EXPECT_EQ(ret, E_INVAL_ARG);

    ICloudDiskNotifyUtils::ins = nullptr;
}

/**
 * @tc.name: RecycleSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::RecycleSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RecycleSetXattrTest1, TestSize.Level1)
{
    const std::string name = "test";
    const std::string parentCloudId = "100";
    const std::string cloudId = "100";
    const std::string value = "notnum";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->RecycleSetXattr(name, parentCloudId, cloudId, value);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: RecycleSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::RecycleSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RecycleSetXattrTest2, TestSize.Level1)
{
    const std::string name = "test";
    const std::string parentCloudId = "100";
    const std::string cloudId = "100";
    const std::string value = "0";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->RecycleSetXattr(name, parentCloudId, cloudId, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: RecycleSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::RecycleSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RecycleSetXattrTest3, TestSize.Level1)
{
    const std::string name = "test";
    const std::string parentCloudId = "100";
    const std::string cloudId = "100";
    const std::string value = "1";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->RecycleSetXattr(name, parentCloudId, cloudId, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: RecycleSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::RecycleSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RecycleSetXattrTest4, TestSize.Level1)
{
    const std::string name = "test";
    const std::string parentCloudId = "100";
    const std::string cloudId = "100";
    const std::string value = "-1";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    
    int32_t ret = clouddiskrdbStore_->RecycleSetXattr(name, parentCloudId, cloudId, value);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: CheckIsConflict
 * @tc.desc: Verify the CloudDiskRdbStore::CheckIsConflict function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CheckIsConflictTest1, TestSize.Level1)
{
    const std::string name = "mock";
    const std::string parentCloudId = "100";
    std::string newName = "test1";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->CheckIsConflict(name, parentCloudId, newName);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: CheckIsConflict
 * @tc.desc: Verify the CloudDiskRdbStore::CheckIsConflict function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CheckIsConflictTest2, TestSize.Level1)
{
    const std::string name = "test";
    const std::string parentCloudId = "100";
    std::string newName = "test1";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->CheckIsConflict(name, parentCloudId, newName);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: RestoreUpdateRdb
 * @tc.desc: Verify the CloudDiskRdbStore::RestoreUpdateRdb function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RestoreUpdateRdbTest1, TestSize.Level1)
{
    const std::string cloudId = "100";
    const struct RestoreInfo restoreInfo;
    ValuesBucket setXattr;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));

    int32_t ret = clouddiskrdbStore_->RestoreUpdateRdb(cloudId, restoreInfo, setXattr);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: RestoreUpdateRdb
 * @tc.desc: Verify the CloudDiskRdbStore::RestoreUpdateRdb function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RestoreUpdateRdbTest2, TestSize.Level1)
{
    const std::string cloudId = "100";
    const struct RestoreInfo restoreInfo;
    ValuesBucket setXattr;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->RestoreUpdateRdb(cloudId, restoreInfo, setXattr);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: RestoreUpdateRdb
 * @tc.desc: Verify the CloudDiskRdbStore::RestoreUpdateRdb function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RestoreUpdateRdbTest3, TestSize.Level1)
{
    const std::string cloudId = "100";
    const struct RestoreInfo restoreInfo;
    ValuesBucket setXattr;
    restoreInfo.parentCloudId == "mock";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->RestoreUpdateRdb(cloudId, restoreInfo, setXattr);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: HandleRestoreXattr
 * @tc.desc: Verify the CloudDiskRdbStore::HandleRestoreXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, HandleRestoreXattrTest1, TestSize.Level1)
{
    const std::string name = "";
    const std::string parentCloudId = "";
    const std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->HandleRestoreXattr(name, parentCloudId, cloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: HandleRestoreXattr
 * @tc.desc: Verify the CloudDiskRdbStore::HandleRestoreXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, HandleRestoreXattrTest2, TestSize.Level1)
{
    const std::string name = "";
    const std::string parentCloudId = "";
    const std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)))
                                           .WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));

    int32_t ret = clouddiskrdbStore_->HandleRestoreXattr(name, parentCloudId, cloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: HandleRestoreXattr
 * @tc.desc: Verify the CloudDiskRdbStore::HandleRestoreXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, HandleRestoreXattrTest3, TestSize.Level1)
{
    const std::string name = "";
    const std::string parentCloudId = "";
    const std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)))
                                           .WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->HandleRestoreXattr(name, parentCloudId, cloudId);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: HandleRecycleXattr
 * @tc.desc: Verify the CloudDiskRdbStore::HandleRecycleXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, HandleRecycleXattrTest1, TestSize.Level1)
{
    const std::string name = "";
    const std::string parentCloudId = "";
    const std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->HandleRecycleXattr(name, parentCloudId, cloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: HandleRecycleXattr
 * @tc.desc: Verify the CloudDiskRdbStore::HandleRecycleXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, HandleRecycleXattrTest2, TestSize.Level1)
{
    const std::string name = "";
    const std::string parentCloudId = "";
    const std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->rootId_ = "rootId";
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(1), Return(E_OK)));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));

    int32_t ret = clouddiskrdbStore_->HandleRecycleXattr(name, parentCloudId, cloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: HandleRecycleXattr
 * @tc.desc: Verify the CloudDiskRdbStore::HandleRecycleXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, HandleRecycleXattrTest3, TestSize.Level1)
{
    const std::string name = "";
    const std::string parentCloudId = "mock";
    const std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->rootId_ = "rootId";
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(0), Return(E_OK)));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->HandleRecycleXattr(name, parentCloudId, cloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: HandleRecycleXattr
 * @tc.desc: Verify the CloudDiskRdbStore::HandleRecycleXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, HandleRecycleXattrTest4, TestSize.Level1)
{
    const std::string name = "";
    const std::string parentCloudId = "";
    const std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->rootId_ = "rootId";
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(1), Return(E_OK)));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->HandleRecycleXattr(name, parentCloudId, cloudId);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetRecycleInfoTest001
 * @tc.desc: Verify the CloudDiskRdbStore::GetRecycleInfo function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetRecycleInfoTest1, TestSize.Level1)
{
    const std::string cloudId = "100";
    int64_t rowId;
    int32_t position;
    std::string attr;
    bool preCount = true;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->GetRecycleInfo(transaction, cloudId, rowId, position, attr, dirtyType);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetRecycleInfoTest002
 * @tc.desc: Verify the CloudDiskRdbStore::GetRecycleInfo function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetRecycleInfoTest2, TestSize.Level1)
{
    const std::string cloudId = "100";
    int64_t rowId;
    int32_t position;
    std::string attr;
    bool preCount = true;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->GetRecycleInfo(transaction, cloudId, rowId, position, attr, dirtyType);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetRecycleInfoTest003
 * @tc.desc: Verify the CloudDiskRdbStore::GetRecycleInfo function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetRecycleInfoTest3, TestSize.Level1)
{
    const std::string cloudId = "100";
    int64_t rowId;
    int32_t position;
    std::string attr;
    bool preCount = true;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->GetRecycleInfo(transaction, cloudId, rowId, position, attr, dirtyType);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetRecycleInfoTest004
 * @tc.desc: Verify the CloudDiskRdbStore::GetRecycleInfo function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetRecycleInfoTest4, TestSize.Level1)
{
    const std::string cloudId = "100";
    int64_t rowId;
    int32_t position;
    std::string attr;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->GetRecycleInfo(transaction, cloudId, rowId, position, attr, dirtyType);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetRecycleInfoTest005
 * @tc.desc: Verify the CloudDiskRdbStore::GetRecycleInfo function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetRecycleInfoTest5, TestSize.Level1)
{
    const std::string cloudId = "100";
    int64_t rowId;
    int32_t position;
    std::string attr;
    bool preCount = true;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->GetRecycleInfo(transaction, cloudId, rowId, position, attr, dirtyType);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetRecycleInfoTest006
 * @tc.desc: Verify the CloudDiskRdbStore::GetRecycleInfo function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetRecycleInfoTest6, TestSize.Level1)
{
    const std::string cloudId = "100";
    int64_t rowId;
    int32_t position;
    std::string attr;
    bool preCount = true;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->GetRecycleInfo(transaction, cloudId, rowId, position, attr, dirtyType);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetRecycleInfoTest007
 * @tc.desc: Verify the CloudDiskRdbStore::GetRecycleInfo function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetRecycleInfoTest7, TestSize.Level1)
{
    const std::string cloudId = "100";
    int64_t rowId;
    int32_t position;
    std::string attr;
    bool preCount = true;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    auto transaction = make_shared<TransactionMock>();
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*transaction, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_RDB));
 
    int32_t ret = clouddiskrdbStore_->GetRecycleInfo(transaction, cloudId, rowId, position, attr, dirtyType);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: FavoriteSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FavoriteSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FavoriteSetXattrTest1, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    const std::string value = "notnum";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->FavoriteSetXattr(cloudId, value);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: FavoriteSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FavoriteSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FavoriteSetXattrTest2, TestSize.Level1)
{
    const std::string cloudId = "root";
    const std::string value = "2";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->FavoriteSetXattr(cloudId, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: FavoriteSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FavoriteSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FavoriteSetXattrTest3, TestSize.Level1)
{
    const std::string cloudId = "root";
    const std::string value = "0";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, Update(_, _, _, _, An<const std::vector<ValueObject> &>())).WillOnce(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->FavoriteSetXattr(cloudId, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: FavoriteSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FavoriteSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FavoriteSetXattrTest4, TestSize.Level1)
{
    const std::string cloudId = "root";
    const std::string value = "1";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, Update(_, _, _, _, An<const std::vector<ValueObject> &>())).WillOnce(Return(E_OK));
    
    int32_t ret = clouddiskrdbStore_->FavoriteSetXattr(cloudId, value);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: LocationGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::LocationGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, LocationGetXattrTest1, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string key = IS_FAVORITE_XATTR;
    std::string value = "";
    const std::string name = "test";
    const std::string parentCloudId = "rootId";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->LocationGetXattr(name, key, value, parentCloudId);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: LocationGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::LocationGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, LocationGetXattrTest2, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string key = CLOUD_FILE_LOCATION;
    std::string value = "";
    const std::string name = "mock";
    const std::string parentCloudId = "rootId";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->LocationGetXattr(name, key, value, parentCloudId);
    EXPECT_EQ(ret, ENOENT);
}

/**
 * @tc.name: LocationGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::LocationGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, LocationGetXattrTest3, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string key = CLOUD_FILE_LOCATION;
    std::string value = "";
    const std::string name = "test";
    const std::string parentCloudId = "rootId";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->LocationGetXattr(name, key, value, parentCloudId);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: FavoriteGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FavoriteGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FavoriteGetXattrTest1, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string key = IS_FAVORITE_XATTR;
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->FavoriteGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: FavoriteGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FavoriteGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FavoriteGetXattrTest2, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    const std::string key = IS_FAVORITE_XATTR;
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->FavoriteGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: FavoriteGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FavoriteGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FavoriteGetXattrTest3, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = IS_FILE_STATUS_XATTR;
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->FavoriteGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: FavoriteGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FavoriteGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FavoriteGetXattrTest4, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = IS_FAVORITE_XATTR;
    std::string value = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->FavoriteGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: FavoriteGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FavoriteGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FavoriteGetXattrTest5, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = IS_FAVORITE_XATTR;
    std::string value = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->FavoriteGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: FavoriteGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FavoriteGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FavoriteGetXattrTest6, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = IS_FAVORITE_XATTR;
    std::string value = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->FavoriteGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: FileStatusGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FileStatusGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FileStatusGetXattrTest1, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string key = IS_FILE_STATUS_XATTR;
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->FileStatusGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: FileStatusGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FileStatusGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FileStatusGetXattrTest2, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    const std::string key = IS_FILE_STATUS_XATTR;
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->FileStatusGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: FileStatusGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FileStatusGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FileStatusGetXattrTest3, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = IS_FAVORITE_XATTR;
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->FileStatusGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: FileStatusGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FileStatusGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FileStatusGetXattrTest4, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = IS_FILE_STATUS_XATTR;
    std::string value = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->FileStatusGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: FileStatusGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FileStatusGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FileStatusGetXattrTest5, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = IS_FILE_STATUS_XATTR;
    std::string value = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->FileStatusGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: FileStatusGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FileStatusGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FileStatusGetXattrTest6, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = IS_FILE_STATUS_XATTR;
    std::string value = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->FileStatusGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: FileStatusGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::FileStatusGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, FileStatusGetXattrTest7, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = IS_FILE_STATUS_XATTR;
    std::string value = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->FileStatusGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: TimeRecycledGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::TimeRecycledGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, TimeRecycledGetXattrTest1, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string key = CLOUD_TIME_RECYCLED;
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->TimeRecycledGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: TimeRecycledGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::TimeRecycledGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, TimeRecycledGetXattrTest2, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    const std::string key = CLOUD_TIME_RECYCLED;
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->TimeRecycledGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: TimeRecycledGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::TimeRecycledGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, TimeRecycledGetXattrTest3, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = IS_FAVORITE_XATTR;
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->TimeRecycledGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: TimeRecycledGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::TimeRecycledGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, TimeRecycledGetXattrTest4, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = CLOUD_TIME_RECYCLED;
    std::string value = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->TimeRecycledGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: TimeRecycledGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::TimeRecycledGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, TimeRecycledGetXattrTest5, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = CLOUD_TIME_RECYCLED;
    std::string value = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->TimeRecycledGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: TimeRecycledGetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::TimeRecycledGetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, TimeRecycledGetXattrTest6, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = CLOUD_TIME_RECYCLED;
    std::string value = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->TimeRecycledGetXattr(cloudId, key, value);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetExtAttrValue
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrValueTest1, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string key = "100";
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->GetExtAttrValue(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetExtAttrValue
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrValueTest2, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    const std::string key = "100";
    std::string value = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->GetExtAttrValue(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetExtAttrValue
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrValueTest3, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = "";
    std::string value = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->GetExtAttrValue(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetExtAttrValue
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrValueTest4, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = "100";
    std::string value = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->GetExtAttrValue(cloudId, key, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetExtAttrValue
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrValueTest5, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = "100";
    std::string value = "100";
    std::string mockValue = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(mockValue), Return(E_OK)));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->GetExtAttrValue(cloudId, key, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetExtAttrValue
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrValueTest6, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = "100";
    std::string value = "100";
    std::string mockValue = "invalid";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(mockValue), Return(E_OK)));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->GetExtAttrValue(cloudId, key, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetExtAttrValue
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrValueTest7, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = "test";
    std::string value = "100";
    std::string mockValue = R"({"key": "/data/service"})";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(mockValue), Return(E_OK)));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->GetExtAttrValue(cloudId, key, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetExtAttrValue
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrValueTest8, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = "key";
    std::string value = "100";
    std::string mockValue = R"({"key": 123})";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(mockValue), Return(E_OK)));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->GetExtAttrValue(cloudId, key, value);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetExtAttrValue
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrValueTest9, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = "key";
    std::string value = "100";
    std::string mockValue = R"({"key": "/data/service"})";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(mockValue), Return(E_OK)));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->GetExtAttrValue(cloudId, key, value);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetExtAttr
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrTest1, TestSize.Level1)
{
    const std::string cloudId = "";
    std::string value = "";
    int32_t pos = 0;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->GetExtAttr(cloudId, value, pos, dirtyType);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetExtAttr
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrTest2, TestSize.Level1)
{
    const std::string cloudId = "rootId";
    std::string value = "";
    int32_t pos = 0;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->GetExtAttr(cloudId, value, pos, dirtyType);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetExtAttr
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrTest3, TestSize.Level1)
{
    const std::string cloudId = "100";
    std::string value = "mock";
    int32_t pos = 0;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->GetExtAttr(cloudId, value, pos, dirtyType);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetExtAttr
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrTest4, TestSize.Level1)
{
    const std::string cloudId = "100";
    std::string value = "";
    int32_t pos = 0;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->GetExtAttr(cloudId, value, pos, dirtyType);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetExtAttr
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrTest5, TestSize.Level1)
{
    const std::string cloudId = "100";
    std::string value = "mock";
    int32_t pos = 0;
    bool preCount = true;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->GetExtAttr(cloudId, value, pos, dirtyType);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetExtAttr
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrTest6, TestSize.Level1)
{
    const std::string cloudId = "100";
    std::string value = "";
    int32_t pos = -1;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->GetExtAttr(cloudId, value, pos, dirtyType);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetExtAttr
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrTest7, TestSize.Level1)
{
    const std::string cloudId = "100";
    std::string value = "";
    int32_t pos = -1;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->GetExtAttr(cloudId, value, pos, dirtyType);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetExtAttr
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrTest8, TestSize.Level1)
{
    const std::string cloudId = "100";
    std::string value = "";
    int32_t pos = 1;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->GetExtAttr(cloudId, value, pos, dirtyType);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetExtAttr
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetExtAttrTest9, TestSize.Level1)
{
    const std::string cloudId = "100";
    std::string value = "";
    int32_t pos = 1;
    int32_t dirtyType = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillOnce(Return(E_OK)).WillOnce(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->GetExtAttr(cloudId, value, pos, dirtyType);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetXAttr
 * @tc.desc: Verify the CloudDiskRdbStore::GetXAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetXAttrTest1, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string key = CLOUD_FILE_LOCATION;
    std::string value = "";
    CacheNode node;
    node.fileName = "test";
    node.parentCloudId = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->GetXAttr(cloudId, key, value);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetXAttr
 * @tc.desc: Verify the CloudDiskRdbStore::GetXAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetXAttrTest2, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string key = IS_FAVORITE_XATTR;
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->GetXAttr(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetXAttr
 * @tc.desc: Verify the CloudDiskRdbStore::GetXAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetXAttrTest3, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string key = IS_FILE_STATUS_XATTR;
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->GetXAttr(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetXAttr
 * @tc.desc: Verify the CloudDiskRdbStore::GetXAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetXAttrTest4, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string key = CLOUD_EXT_ATTR;
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->GetXAttr(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetXAttr
 * @tc.desc: Verify the CloudDiskRdbStore::GetXAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetXAttrTest5, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string key = CLOUD_TIME_RECYCLED;
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->GetXAttr(cloudId, key, value);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetXAttr
 * @tc.desc: Verify the CloudDiskRdbStore::GetXAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetXAttrTest6, TestSize.Level1)
{
    const std::string cloudId = "";
    const std::string key = CLOUD_HAS_THM;
    std::string value = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->GetXAttr(cloudId, key, value);
    EXPECT_EQ(ret, ENOSYS);
}

/**
 * @tc.name: ExtAttributeSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, ExtAttributeSetXattrTest1, TestSize.Level1)
{
    const std::string cloudId = "cloudId";
    const std::string key = "key1";
    const std::string value = "value1";
    std::string xattrList = "";
    int32_t pos = 0;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(xattrList), Return(E_OK)));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(pos), Return(E_OK)));

    int32_t ret = clouddiskrdbStore_->ExtAttributeSetXattr(cloudId, value, key);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: ExtAttributeSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, ExtAttributeSetXattrTest2, TestSize.Level1)
{
    const std::string cloudId = "cloudId";
    const std::string key = "key1";
    const std::string value = "value1";
    std::string xattrList = "{\"key2\":\"value2\"}";;
    int32_t pos = 0;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(xattrList), Return(E_OK)));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(pos), Return(E_OK)));

    int32_t ret = clouddiskrdbStore_->ExtAttributeSetXattr(cloudId, value, key);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: ExtAttributeSetXattr
 * @tc.desc: Verify the CloudDiskRdbStore::ExtAttributeSetXattr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, ExtAttributeSetXattrTest3, TestSize.Level1)
{
    const std::string cloudId = "cloudId";
    const std::string key = "key1";
    const std::string value = "value1";
    std::string xattrList = "{\"key1\":\"value1\"}";;
    int32_t pos = 1;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    auto transaction = make_shared<TransactionMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(xattrList), Return(E_OK)));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(pos), Return(E_OK)));

    int32_t ret = clouddiskrdbStore_->ExtAttributeSetXattr(cloudId, value, key);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: SetXAttr
 * @tc.desc: Verify the CloudDiskRdbStore::SetXAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SetXAttrTest1, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = CLOUD_FILE_LOCATION;
    const std::string value = "notnum";
    const std::string name = "test";
    const std::string parentCloudId = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->SetXAttr(cloudId, key, value, name, parentCloudId);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: SetXAttr
 * @tc.desc: Verify the CloudDiskRdbStore::SetXAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SetXAttrTest2, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = CLOUD_CLOUD_RECYCLE_XATTR;
    const std::string value = "notnum";
    const std::string name = "test";
    const std::string parentCloudId = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->SetXAttr(cloudId, key, value, name, parentCloudId);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: SetXAttr
 * @tc.desc: Verify the CloudDiskRdbStore::SetXAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SetXAttrTest3, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = IS_FAVORITE_XATTR;
    const std::string value = "notnum";
    const std::string name = "test";
    const std::string parentCloudId = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->SetXAttr(cloudId, key, value, name, parentCloudId);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: SetXAttr
 * @tc.desc: Verify the CloudDiskRdbStore::SetXAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SetXAttrTest4, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = CLOUD_EXT_ATTR;
    const std::string value = "notnum";
    const std::string name = "test";
    const std::string parentCloudId = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->SetXAttr(cloudId, key, value, name, parentCloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: SetXAttr
 * @tc.desc: Verify the CloudDiskRdbStore::SetXAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SetXAttrTest5, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = CLOUD_HAS_THM;
    const std::string value = "notnum";
    const std::string name = "test";
    const std::string parentCloudId = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->SetXAttr(cloudId, key, value, name, parentCloudId);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: SetXAttr
 * @tc.desc: Verify the CloudDiskRdbStore::SetXAttr function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, SetXAttrTest6, TestSize.Level1)
{
    const std::string cloudId = "100";
    const std::string key = CLOUD_CLOUD_ID_XATTR;
    const std::string value = "notnum";
    const std::string name = "test";
    const std::string parentCloudId = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->SetXAttr(cloudId, key, value, name, parentCloudId);
    EXPECT_EQ(ret, ENOSYS);
}

/**
 * @tc.name: Rename
 * @tc.desc: Verify the CloudDiskRdbStore::Rename function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RenameTest1, TestSize.Level1)
{
    const std::string oldParentCloudId = "100";
    const std::string oldFileName = "test";
    const std::string newParentCloudId = "100";
    const std::string newFileName = "";
    const bool newFileNoNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
 
    int32_t ret = clouddiskrdbStore_->Rename(oldParentCloudId, oldFileName, newParentCloudId, newFileName,
                                             newFileNoNeedUpload);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: Rename
 * @tc.desc: Verify the CloudDiskRdbStore::Rename function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RenameTest2, TestSize.Level1)
{
    const std::string oldParentCloudId = "";
    const std::string oldFileName = "test";
    const std::string newParentCloudId = "100";
    const std::string newFileName = "newFileName";
    const bool newFileNoNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
 
    int32_t ret = clouddiskrdbStore_->Rename(oldParentCloudId, oldFileName, newParentCloudId, newFileName,
                                             newFileNoNeedUpload);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: Rename
 * @tc.desc: Verify the CloudDiskRdbStore::Rename function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RenameTest3, TestSize.Level1)
{
    const std::string oldParentCloudId = "100";
    const std::string oldFileName = "";
    const std::string newParentCloudId = "100";
    const std::string newFileName = "newFileName";
    const bool newFileNoNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
 
    int32_t ret = clouddiskrdbStore_->Rename(oldParentCloudId, oldFileName, newParentCloudId, newFileName,
                                             newFileNoNeedUpload);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: Rename
 * @tc.desc: Verify the CloudDiskRdbStore::Rename function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RenameTest4, TestSize.Level1)
{
    const std::string oldParentCloudId = "100";
    const std::string oldFileName = "test";
    const std::string newParentCloudId = "";
    const std::string newFileName = "newFileName";
    const bool newFileNoNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
 
    int32_t ret = clouddiskrdbStore_->Rename(oldParentCloudId, oldFileName, newParentCloudId, newFileName,
                                             newFileNoNeedUpload);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: Rename
 * @tc.desc: Verify the CloudDiskRdbStore::Rename function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RenameTest5, TestSize.Level1)
{
    const std::string oldParentCloudId = "10";
    const std::string oldFileName = "mock";
    const std::string newParentCloudId = "100";
    const std::string newFileName = "newFileName";
    const bool newFileNoNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
 
    int32_t ret = clouddiskrdbStore_->Rename(oldParentCloudId, oldFileName, newParentCloudId, newFileName,
                                             newFileNoNeedUpload);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: Rename
 * @tc.desc: Verify the CloudDiskRdbStore::Rename function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RenameTest6, TestSize.Level1)
{
    const std::string oldParentCloudId = "10";
    const std::string oldFileName = "oldFileName";
    const std::string newParentCloudId = "100";
    const std::string newFileName = "..";
    const bool newFileNoNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
 
    int32_t ret = clouddiskrdbStore_->Rename(oldParentCloudId, oldFileName, newParentCloudId, newFileName,
                                             newFileNoNeedUpload);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: Rename
 * @tc.desc: Verify the CloudDiskRdbStore::Rename function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RenameTest7, TestSize.Level1)
{
    const std::string oldParentCloudId = "100";
    const std::string oldFileName = "oldFileName";
    const std::string newParentCloudId = "100";
    const std::string newFileName = "mock";
    const bool newFileNoNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
 
    int32_t ret = clouddiskrdbStore_->Rename(oldParentCloudId, oldFileName, newParentCloudId, newFileName,
                                             newFileNoNeedUpload);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: Rename
 * @tc.desc: Verify the CloudDiskRdbStore::Rename function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, RenameTest8, TestSize.Level1)
{
    const std::string oldParentCloudId = "10";
    const std::string oldFileName = "oldFileName";
    const std::string newParentCloudId = "100";
    const std::string newFileName = "mock";
    const bool newFileNoNeedUpload = false;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
 
    int32_t ret = clouddiskrdbStore_->Rename(oldParentCloudId, oldFileName, newParentCloudId, newFileName,
                                             newFileNoNeedUpload);
    EXPECT_EQ(ret, EINVAL);
}

/**
 * @tc.name: GetHasChild
 * @tc.desc: Verify the CloudDiskRdbStore::GetHasChild function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetHasChildTest1, TestSize.Level1)
{
    const std::string cloudId = "100";
    bool hasChild = true;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));
    int32_t ret = clouddiskrdbStore_->GetHasChild(cloudId, hasChild);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetHasChild
 * @tc.desc: Verify the CloudDiskRdbStore::GetHasChild function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetHasChildTest2, TestSize.Level1)
{
    const std::string cloudId = "100";
    bool hasChild = true;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));
    int32_t ret = clouddiskrdbStore_->GetHasChild(cloudId, hasChild);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetHasChild
 * @tc.desc: Verify the CloudDiskRdbStore::GetHasChild function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetHasChildTest3, TestSize.Level1)
{
    const std::string cloudId = "100";
    bool hasChild = true;
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    int32_t ret = clouddiskrdbStore_->GetHasChild(cloudId, hasChild);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: UnlinkSynced
 * @tc.desc: Verify the CloudDiskRdbStore::UnlinkSynced function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, UnlinkSyncedTest1, TestSize.Level1)
{
    const std::string cloudId = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, Update(_, _, _, _, An<const std::vector<std::string> &>())).WillOnce(Return(E_RDB));
    
    int32_t ret = clouddiskrdbStore_->UnlinkSynced(cloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: UnlinkSynced
 * @tc.desc: Verify the CloudDiskRdbStore::UnlinkSynced function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, UnlinkSyncedTest2, TestSize.Level1)
{
    const std::string cloudId = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, Update(_, _, _, _, An<const std::vector<std::string> &>())).WillOnce(Return(E_OK));
    
    int32_t ret = clouddiskrdbStore_->UnlinkSynced(cloudId);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: UnlinkLocal
 * @tc.desc: Verify the CloudDiskRdbStore::UnlinkLocal function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, UnlinkLocalTest1, TestSize.Level1)
{
    const std::string cloudId = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, Delete(_, _, _, An<const std::vector<std::string> &>())).WillOnce(Return(E_RDB));
    
    int32_t ret = clouddiskrdbStore_->UnlinkLocal(cloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: UnlinkLocal
 * @tc.desc: Verify the CloudDiskRdbStore::UnlinkLocal function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, UnlinkLocalTest2, TestSize.Level1)
{
    const std::string cloudId = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, Delete(_, _, _, An<const std::vector<std::string> &>())).WillOnce(Return(E_OK));
    
    int32_t ret = clouddiskrdbStore_->UnlinkLocal(cloudId);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: Unlink
 * @tc.desc: Verify the CloudDiskRdbStore::Unlink function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, UnlinkTest1, TestSize.Level1)
{
    std::string cloudId = "";
    const int32_t noUpload = 0;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;

    int32_t ret = clouddiskrdbStore_->Unlink(cloudId, noUpload);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: Unlink
 * @tc.desc: Verify the CloudDiskRdbStore::Unlink function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, UnlinkTest2, TestSize.Level1)
{
    const int32_t noUpload = 1;
    std::string cloudId = "rootId";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    
    int32_t ret = clouddiskrdbStore_->Unlink(cloudId, noUpload);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: Unlink
 * @tc.desc: Verify the CloudDiskRdbStore::Unlink function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, UnlinkTest3, TestSize.Level1)
{
    const int32_t noUpload = 1;
    std::string cloudId = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, Delete(_, _, _, An<const std::vector<std::string> &>())).WillOnce(Return(E_OK));
    
    int32_t ret = clouddiskrdbStore_->Unlink(cloudId, noUpload);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: Unlink
 * @tc.desc: Verify the CloudDiskRdbStore::Unlink function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, UnlinkTest4, TestSize.Level1)
{
    const int32_t noUpload = 0;
    std::string cloudId = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, Update(_, _, _, _, An<const std::vector<std::string> &>())).WillOnce(Return(E_OK));
    
    int32_t ret = clouddiskrdbStore_->Unlink(cloudId, noUpload);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetDirtyType
 * @tc.desc: Verify the CloudDiskRdbStore::GetDirtyType function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetDirtyTypeTest1, TestSize.Level1)
{
    int32_t dirtyType = 0;
    std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));
    
    int32_t ret = clouddiskrdbStore_->GetDirtyType(cloudId, dirtyType);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetDirtyType
 * @tc.desc: Verify the CloudDiskRdbStore::GetDirtyType function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetDirtyTypeTest2, TestSize.Level1)
{
    int32_t dirtyType = 0;
    std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->GetDirtyType(cloudId, dirtyType);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetDirtyType
 * @tc.desc: Verify the CloudDiskRdbStore::GetDirtyType function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetDirtyTypeTest3, TestSize.Level1)
{
    int32_t dirtyType = -1;
    std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->GetDirtyType(cloudId, dirtyType);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetDirtyType
 * @tc.desc: Verify the CloudDiskRdbStore::GetDirtyType function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetDirtyTypeTest4, TestSize.Level1)
{
    int32_t dirtyType = 1;
    std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));

    int32_t ret = clouddiskrdbStore_->GetDirtyType(cloudId, dirtyType);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetCurNode
 * @tc.desc: Verify the CloudDiskRdbStore::GetCurNode function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetCurNodeTest1, TestSize.Level1)
{
    CacheNode curNode;
    std::string cloudId = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    int32_t ret = clouddiskrdbStore_->GetCurNode(cloudId, curNode);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetCurNode
 * @tc.desc: Verify the CloudDiskRdbStore::GetCurNode function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetCurNodeTest2, TestSize.Level1)
{
    CacheNode curNode;
    std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));
    
    int32_t ret = clouddiskrdbStore_->GetCurNode(cloudId, curNode);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetCurNode
 * @tc.desc: Verify the CloudDiskRdbStore::GetCurNode function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetCurNodeTest3, TestSize.Level1)
{
    CacheNode curNode;
    std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));
    
    int32_t ret = clouddiskrdbStore_->GetCurNode(cloudId, curNode);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetCurNode
 * @tc.desc: Verify the CloudDiskRdbStore::GetCurNode function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetCurNodeTest4, TestSize.Level1)
{
    CacheNode curNode;
    std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetRow(_)).WillRepeatedly(Return(E_RDB));
    
    int32_t ret = clouddiskrdbStore_->GetCurNode(cloudId, curNode);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetCurNode
 * @tc.desc: Verify the CloudDiskRdbStore::GetCurNode function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetCurNodeTest5, TestSize.Level1)
{
    CacheNode curNode;
    std::string cloudId = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetRow(_)).WillRepeatedly(Return(E_OK));
    
    int32_t ret = clouddiskrdbStore_->GetCurNode(cloudId, curNode);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetParentNode
 * @tc.desc: Verify the CloudDiskRdbStore::GetParentNode function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetParentNodeTest1, TestSize.Level1)
{
    const std::string parentCloudId = "";
    std::string nextCloudId = "100";
    std::string fileName = "test";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    
    int32_t ret = clouddiskrdbStore_->GetParentNode(parentCloudId, nextCloudId, fileName);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetParentNode
 * @tc.desc: Verify the CloudDiskRdbStore::GetParentNode function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetParentNodeTest2, TestSize.Level1)
{
    const std::string parentCloudId = "100";
    std::string nextCloudId = "100";
    std::string fileName = "test";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));
    
    int32_t ret = clouddiskrdbStore_->GetParentNode(parentCloudId, nextCloudId, fileName);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetParentNode
 * @tc.desc: Verify the CloudDiskRdbStore::GetParentNode function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetParentNodeTest3, TestSize.Level1)
{
    const std::string parentCloudId = "100";
    std::string nextCloudId = "100";
    std::string fileName = "test";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));
    
    int32_t ret = clouddiskrdbStore_->GetParentNode(parentCloudId, nextCloudId, fileName);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetParentNode
 * @tc.desc: Verify the CloudDiskRdbStore::GetParentNode function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetParentNodeTest4, TestSize.Level1)
{
    const std::string parentCloudId = "100";
    std::string nextCloudId = "100";
    std::string fileName = "test";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetRow(_)).WillRepeatedly(Return(E_RDB));
    
    int32_t ret = clouddiskrdbStore_->GetParentNode(parentCloudId, nextCloudId, fileName);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetParentNode
 * @tc.desc: Verify the CloudDiskRdbStore::GetParentNode function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetParentNodeTest5, TestSize.Level1)
{
    const std::string parentCloudId = "100";
    std::string nextCloudId = "100";
    std::string fileName = "test";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetRow(_)).WillRepeatedly(Return(E_OK));
    
    int32_t ret = clouddiskrdbStore_->GetParentNode(parentCloudId, nextCloudId, fileName);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetUriFromDB
 * @tc.desc: Verify the CloudDiskRdbStore::GetUriFromDB function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetUriFromDBTest1, TestSize.Level1)
{
    const std::string parentCloudId = "";
    std::string uri = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    
    int32_t ret = clouddiskrdbStore_->GetUriFromDB(parentCloudId, uri);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetUriFromDB
 * @tc.desc: Verify the CloudDiskRdbStore::GetUriFromDB function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetUriFromDBTest2, TestSize.Level1)
{
    const std::string parentCloudId = "rootId";
    std::string uri = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    
    int32_t ret = clouddiskrdbStore_->GetUriFromDB(parentCloudId, uri);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetUriFromDB
 * @tc.desc: Verify the CloudDiskRdbStore::GetUriFromDB function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetUriFromDBTest3, TestSize.Level1)
{
    const std::string parentCloudId = "100";
    std::string uri = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));
    
    int32_t ret = clouddiskrdbStore_->GetUriFromDB(parentCloudId, uri);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetUriFromDB
 * @tc.desc: Verify the CloudDiskRdbStore::GetUriFromDB function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetUriFromDBTest4, TestSize.Level1)
{
    const std::string parentCloudId = "100";
    std::string nextCloudId = "cloudid";
    std::string uri = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillOnce(Return(E_OK)).WillRepeatedly(Return(E_RDB));
    EXPECT_CALL(*rset, GetRow(_)).WillRepeatedly(Return(E_OK));
    
    int32_t ret = clouddiskrdbStore_->GetUriFromDB(parentCloudId, uri);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetUriFromDB
 * @tc.desc: Verify the CloudDiskRdbStore::GetUriFromDB function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetUriFromDBTest5, TestSize.Level1)
{
    const std::string parentCloudId = "100";
    std::string nextCloudId = "cloudid";
    std::string uri = "100";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetRow(_)).WillRepeatedly(Return(E_OK));
    
    int32_t ret = clouddiskrdbStore_->GetUriFromDB(parentCloudId, uri);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetNotifyUri
 * @tc.desc: Verify the CloudDiskRdbStore::GetNotifyUri function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetNotifyUriTest1, TestSize.Level1)
{
    CacheNode cacheNode;
    std::string uri = "100";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->rootId_ = "";
    int32_t ret = clouddiskrdbStore_->GetNotifyUri(cacheNode, uri);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetNotifyUri
 * @tc.desc: Verify the CloudDiskRdbStore::GetNotifyUri function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetNotifyUriTest2, TestSize.Level1)
{
    CacheNode cacheNode;
    std::string uri = "test";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->rootId_ = "rootId";
    int32_t ret = clouddiskrdbStore_->GetNotifyUri(cacheNode, uri);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetNotifyUri
 * @tc.desc: Verify the CloudDiskRdbStore::GetNotifyUri function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetNotifyUriTest3, TestSize.Level1)
{
    CacheNode cacheNode;
    std::string uri = "mock";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->rootId_ = "rootId";
    int32_t ret = clouddiskrdbStore_->GetNotifyUri(cacheNode, uri);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: GetNotifyUri
 * @tc.desc: Verify the CloudDiskRdbStore::GetNotifyUri function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetNotifyUriTest4, TestSize.Level1)
{
    CacheNode cacheNode;
    cacheNode.parentCloudId = "root";
    std::string uri = "mock";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->rootId_ = "rootId";
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->GetNotifyUri(cacheNode, uri);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: GetNotifyData
 * @tc.desc: Verify the CloudDiskRdbStore::GetNotifyData function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetNotifyDataTest1, TestSize.Level1)
{
    CacheNode cacheNode;
    NotifyData notifyData;
    notifyData.uri = "mock";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->rootId_ = "";

    int32_t ret = clouddiskrdbStore_->GetNotifyData(cacheNode, notifyData);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: GetNotifyData
 * @tc.desc: Verify the CloudDiskRdbStore::GetNotifyData function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, GetNotifyDataTest2, TestSize.Level1)
{
    CacheNode cacheNode;
    NotifyData notifyData;
    notifyData.uri = "mock";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->rootId_ = "rootId";

    int32_t ret = clouddiskrdbStore_->GetNotifyData(cacheNode, notifyData);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: CheckRootIdValid
 * @tc.desc: Verify the CloudDiskRdbStore::CheckRootIdValid function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CheckRootIdValidTest1, TestSize.Level1)
{
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->rootId_ = "rootId";

    int32_t ret = clouddiskrdbStore_->CheckRootIdValid();
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: CheckRootIdValid
 * @tc.desc: Verify the CloudDiskDataCallBack::CheckRootIdValid function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CheckRootIdValidTest2, TestSize.Level1)
{
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    clouddiskrdbStore_->rootId_ = "";

    int32_t ret = clouddiskrdbStore_->CheckRootIdValid();
    EXPECT_EQ(ret, E_INVAL_ARG);
}

/**
 * @tc.name: OnCreate
 * @tc.desc: Verify the CloudDiskDataCallBack::OnCreate function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, OnCreateTest1, TestSize.Level1)
{
    RdbStoreMock store;
    RdbStoreMock* rdb = &store;
    CloudDiskDataCallBack CloudDiskDataCallBack;
    EXPECT_CALL(*rdb, ExecuteSql(_, _)).WillRepeatedly(Return(E_RDB));

    int32_t ret = CloudDiskDataCallBack.OnCreate(store);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
}

/**
 * @tc.name: OnCreate
 * @tc.desc: Verify the CloudDiskDataCallBack::OnCreate function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, OnCreateTest2, TestSize.Level1)
{
    RdbStoreMock store;
    RdbStoreMock* rdb = &store;
    CloudDiskDataCallBack CloudDiskDataCallBack;
    EXPECT_CALL(*rdb, ExecuteSql(_, _)).WillRepeatedly(Return(E_OK));

    int32_t ret = CloudDiskDataCallBack.OnCreate(store);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: OnUpgrade
 * @tc.desc: Verify the CloudDiskDataCallBack::OnUpgrade function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, OnUpgradeTest1, TestSize.Level1)
{
    RdbStoreMock store;
    int32_t oldVersion = 13;
    int32_t newVersion = 14;
    CloudDiskDataCallBack CloudDiskDataCallBack;
    int32_t ret = CloudDiskDataCallBack.OnUpgrade(store, oldVersion, newVersion);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: OnUpgrade
 * @tc.desc: Verify the CloudDiskDataCallBack::OnUpgrade function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, OnUpgradeTest2, TestSize.Level1)
{
    RdbStoreMock store;
    int32_t oldVersion = 8;
    int32_t newVersion = 14;
    CloudDiskDataCallBack CloudDiskDataCallBack;
    int32_t ret = CloudDiskDataCallBack.OnUpgrade(store, oldVersion, newVersion);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: CreateDentryFile
 * @tc.desc: Verify the CloudDiskRdbStore::CreateDentryFile function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CreateDentryFileTest1, TestSize.Level1)
{
    MetaBase metaBase;
    metaBase.name = "mock";
    std::string destParentCloudId = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    
    int32_t ret = clouddiskrdbStore_->CreateDentryFile(metaBase, destParentCloudId);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: CreateDentryFile
 * @tc.desc: Verify the CloudDiskRdbStore::CreateDentryFile function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CreateDentryFileTest2, TestSize.Level1)
{
    MetaBase metaBase;
    std::string destParentCloudId = "";
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    
    int32_t ret = clouddiskrdbStore_->CreateDentryFile(metaBase, destParentCloudId);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: InsertCopyData
 * @tc.desc: Verify the CloudDiskRdbStore::InsertCopyData function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, InsertCopyDataTest1, TestSize.Level1)
{
    std::string srcCloudId = "";
    std::string destCloudId = "";
    std::string destParentCloudId = "";
    auto rdb = make_shared<RdbStoreMock>();
    auto transaction = make_shared<TransactionMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*rset, GetColumnIndex(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*transaction, Insert(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));
    
    int32_t ret = clouddiskrdbStore_->InsertCopyData(srcCloudId, destCloudId, destParentCloudId, rset);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: InsertCopyData
 * @tc.desc: Verify the CloudDiskRdbStore::InsertCopyData function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, InsertCopyDataTest2, TestSize.Level1)
{
    std::string srcCloudId = "";
    std::string destCloudId = "";
    std::string destParentCloudId = "";
    auto rdb = make_shared<RdbStoreMock>();
    auto transaction = make_shared<TransactionMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rset, GetColumnIndex(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>("mock"), Return(E_OK)));

    int32_t ret = clouddiskrdbStore_->InsertCopyData(srcCloudId, destCloudId, destParentCloudId, rset);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: InsertCopyData
 * @tc.desc: Verify the CloudDiskRdbStore::InsertCopyData function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, InsertCopyDataTest3, TestSize.Level1)
{
    std::string srcCloudId = "";
    std::string destCloudId = "";
    std::string destParentCloudId = "";
    auto rdb = make_shared<RdbStoreMock>();
    auto transaction = make_shared<TransactionMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*rset, GetColumnIndex(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*transaction, Insert(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));
    
    int32_t ret = clouddiskrdbStore_->InsertCopyData(srcCloudId, destCloudId, destParentCloudId, rset);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: CopyFile
 * @tc.desc: Verify the CloudDiskRdbStore::CopyFile function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CopyFileTest1, TestSize.Level1)
{
    std::string srcCloudId = "";
    std::string destCloudId = "";
    std::string bundleName = "com.ohos.photos";
    const int userId = 100;
    std::string destPath = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(nullptr)));

    int32_t ret = clouddiskrdbStore_->CopyFile(srcCloudId, destCloudId,
                                               bundleName, userId, destPath);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: CopyFile
 * @tc.desc: Verify the CloudDiskRdbStore::CopyFile function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CopyFileTest2, TestSize.Level1)
{
    std::string srcCloudId = "";
    std::string destCloudId = "";
    std::string bundleName = "com.ohos.photos";
    const int userId = 100;
    std::string destPath = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));

    int32_t ret = clouddiskrdbStore_->CopyFile(srcCloudId, destCloudId,
                                               bundleName, userId, destPath);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: CopyFile
 * @tc.desc: Verify the CloudDiskRdbStore::CopyFile function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CopyFileTest3, TestSize.Level1)
{
    std::string srcCloudId = "";
    std::string destCloudId = "";
    std::string bundleName = "com.ohos.photos";
    const int userId = 100;
    std::string destPath = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    auto transaction = make_shared<TransactionMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetColumnIndex(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*transaction, Insert(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));

    int32_t ret = clouddiskrdbStore_->CopyFile(srcCloudId, destCloudId,
                                               bundleName, userId, destPath);
    EXPECT_EQ(ret, E_RDB);
}

/**
 * @tc.name: CopyFile
 * @tc.desc: Verify the CloudDiskRdbStore::CopyFile function
 * @tc.type: FUNC
 */
HWTEST_F(CloudDiskRdbStoreTest, CopyFileTest4, TestSize.Level1)
{
    std::string srcCloudId = "";
    std::string destCloudId = "";
    std::string bundleName = "com.ohos.photos";
    const int userId = 100;
    std::string destPath = "";
    bool preCount = true;
    auto rdb = make_shared<RdbStoreMock>();
    auto transaction = make_shared<TransactionMock>();
    clouddiskrdbStore_->rdbStore_ = rdb;
    std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
    EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(),
    An<const std::vector<std::string> &>(), preCount)).WillOnce(Return(ByMove(rset)));
    EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
    EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetColumnIndex(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetLong(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetInt(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*transaction, Insert(_, _, _)).WillOnce(Return(std::make_pair(E_OK, 0)));

    int32_t ret = clouddiskrdbStore_->CopyFile(srcCloudId, destCloudId,
                                               bundleName, userId, destPath);
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: OnUpgradeExtend
 * @tc.desc: Verify the CloudDiskRdbStore::OnUpgradeExtend function
 * @tc.type: FUNC
 * @tc.require: ICGORT
 */
HWTEST_F(CloudDiskRdbStoreTest, OnUpgradeExtendTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnUpgradeExtendTest001 start";
    try {
        RdbStoreMock store;
        int32_t oldVersion = 15;
        int32_t newVersion = 14;
        CloudDiskDataCallBack CloudDiskDataCallBack;
        int32_t ret = CloudDiskDataCallBack.OnUpgradeExtend(store, oldVersion, newVersion);
        EXPECT_EQ(ret, E_OK);

        oldVersion = 16;
        ret = CloudDiskDataCallBack.OnUpgradeExtend(store, oldVersion, newVersion);
        EXPECT_EQ(ret, E_OK);

        oldVersion = 17;
        ret = CloudDiskDataCallBack.OnUpgradeExtend(store, oldVersion, newVersion);
        EXPECT_EQ(ret, E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnUpgradeExtendTest001 failed";
    }
    GTEST_LOG_(INFO) << "OnUpgradeExtendTest001 end";
}

/**
 * @tc.name: UpdateTHMStatus
 * @tc.desc: Verify the CloudDiskRdbStore::UpdateTHMStatus function
 * @tc.type: FUNC
 * @tc.require: ICPOAK
 */
HWTEST_F(CloudDiskRdbStoreTest, UpdateTHMStatusTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UpdateTHMStatusTest001 start";
    try {
        shared_ptr<CloudDiskMetaFile> metaFile = make_shared<CloudDiskMetaFile>(100, "", "");
        MetaBase metaBase;
        metaBase.fileType == FILE_TYPE_THUMBNAIL;
        int32_t status = 0;
        bool preCount = true;
        auto rdb = make_shared<RdbStoreMock>();
        clouddiskrdbStore_->rdbStore_ = rdb;
        auto transaction = make_shared<TransactionMock>();
        EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(), An<const std::vector<std::string> &>(), preCount))
        .WillOnce(Return(ByMove(nullptr)));
        int32_t ret = clouddiskrdbStore_->UpdateTHMStatus(metaFile, metaBase, status);
        EXPECT_EQ(ret, E_RDB);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UpdateTHMStatusTest001 failed";
    }
    GTEST_LOG_(INFO) << "UpdateTHMStatusTest001 end";
}

/**
 * @tc.name: UpdateTHMStatus
 * @tc.desc: Verify the CloudDiskRdbStore::UpdateTHMStatus function
 * @tc.type: FUNC
 * @tc.require: ICPOAK
 */
HWTEST_F(CloudDiskRdbStoreTest, UpdateTHMStatusTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UpdateTHMStatusTest002 start";
    try {
        shared_ptr<CloudDiskMetaFile> metaFile = make_shared<CloudDiskMetaFile>(100, "", "");
        MetaBase metaBase;
        metaBase.fileType == FILE_TYPE_CONTENT;
        int32_t status = 0;
        bool preCount = true;
        auto rdb = make_shared<RdbStoreMock>();
        clouddiskrdbStore_->rdbStore_ = rdb;
        auto transaction = make_shared<TransactionMock>();
        std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
        EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(), An<const std::vector<std::string> &>(), preCount))
        .WillOnce(Return(ByMove(rset)));
        EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
        EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
        EXPECT_CALL(*rset, GetColumnIndex(_, _)).WillRepeatedly(Return(E_OK));
        EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(50, transaction)));
        int32_t ret = clouddiskrdbStore_->UpdateTHMStatus(metaFile, metaBase, status);
        EXPECT_EQ(ret, 50);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UpdateTHMStatusTest002 failed";
    }
    GTEST_LOG_(INFO) << "UpdateTHMStatusTest002 end";
}

/**
 * @tc.name: UpdateTHMStatus
 * @tc.desc: Verify the CloudDiskRdbStore::UpdateTHMStatus function
 * @tc.type: FUNC
 * @tc.require: ICPOAK
 */
HWTEST_F(CloudDiskRdbStoreTest, UpdateTHMStatusTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UpdateTHMStatusTest003 start";
    try {
        shared_ptr<CloudDiskMetaFile> metaFile = make_shared<CloudDiskMetaFile>(100, "", "");
        MetaBase metaBase;
        metaBase.fileType == FILE_TYPE_CONTENT;
        int32_t status = 0;
        bool preCount = true;
        auto rdb = make_shared<RdbStoreMock>();
        clouddiskrdbStore_->rdbStore_ = rdb;
        auto transaction = make_shared<TransactionMock>();
        std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
        EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(), An<const std::vector<std::string> &>(), preCount))
        .WillOnce(Return(ByMove(rset)));
        EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
        EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
        EXPECT_CALL(*rset, GetColumnIndex(_, _)).WillRepeatedly(Return(E_OK));
        EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
        EXPECT_CALL(*transaction, Update(_, _, _)).WillOnce(Return(std::make_pair(E_RDB, 0)));
        int32_t ret = clouddiskrdbStore_->UpdateTHMStatus(metaFile, metaBase, status);
        EXPECT_EQ(ret, E_RDB);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UpdateTHMStatusTest003 failed";
    }
    GTEST_LOG_(INFO) << "UpdateTHMStatusTest003 end";
}

/**
 * @tc.name: UpdateTHMStatus
 * @tc.desc: Verify the CloudDiskRdbStore::UpdateTHMStatus function
 * @tc.type: FUNC
 * @tc.require: ICPOAK
 */
HWTEST_F(CloudDiskRdbStoreTest, UpdateTHMStatusTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UpdateTHMStatusTest004 start";
    try {
        shared_ptr<CloudDiskMetaFile> metaFile = make_shared<CloudDiskMetaFile>(100, "", "");
        MetaBase metaBase;
        metaBase.fileType == FILE_TYPE_CONTENT;
        int32_t status = 0;
        bool preCount = true;
        auto rdb = make_shared<RdbStoreMock>();
        clouddiskrdbStore_->rdbStore_ = rdb;
        auto transaction = make_shared<TransactionMock>();
        std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
        EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(), An<const std::vector<std::string> &>(), preCount))
        .WillOnce(Return(ByMove(rset)));
        EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
        EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
        EXPECT_CALL(*rset, GetColumnIndex(_, _)).WillRepeatedly(Return(E_OK));
        EXPECT_CALL(*rdb, CreateTransaction(_)).WillOnce(Return(std::make_pair(E_OK, transaction)));
        EXPECT_CALL(*transaction, Update(_, _, _)).WillRepeatedly(Return(std::make_pair(E_OK, 0)));
        int32_t ret = clouddiskrdbStore_->UpdateTHMStatus(metaFile, metaBase, status);
        EXPECT_EQ(ret, E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UpdateTHMStatusTest004 failed";
    }
    GTEST_LOG_(INFO) << "UpdateTHMStatusTest004 end";
}

/**
 * @tc.name: GetSrcCloudId
 * @tc.desc: Verify the CloudDiskRdbStore::GetSrcCloudId function
 * @tc.type: FUNC
 * @tc.require: ICPOAK
 */
HWTEST_F(CloudDiskRdbStoreTest, GetSrcCloudIdTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSrcCloudIdTest001 start";
    try {
        std::string cloudId = "";
        std::string srcCloudId = "";
        bool preCount = true;
        auto rdb = make_shared<RdbStoreMock>();
        clouddiskrdbStore_->rdbStore_ = rdb;
        EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(), An<const std::vector<std::string> &>(), preCount))
        .WillOnce(Return(ByMove(nullptr)));
        int32_t ret = clouddiskrdbStore_->GetSrcCloudId(cloudId, srcCloudId);
        EXPECT_EQ(ret, E_RDB);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSrcCloudIdTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetSrcCloudIdTest001 end";
}

/**
 * @tc.name: GetSrcCloudId
 * @tc.desc: Verify the CloudDiskRdbStore::GetSrcCloudId function
 * @tc.type: FUNC
 * @tc.require: ICPOAK
 */
HWTEST_F(CloudDiskRdbStoreTest, GetSrcCloudIdTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSrcCloudIdTest002 start";
    try {
        std::string cloudId = "";
        std::string srcCloudId = "";
        bool preCount = true;
        auto rdb = make_shared<RdbStoreMock>();
        clouddiskrdbStore_->rdbStore_ = rdb;
        auto transaction = make_shared<TransactionMock>();
        std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
        EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(), An<const std::vector<std::string> &>(), preCount))
        .WillOnce(Return(ByMove(rset)));
        EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_RDB));
        int32_t ret = clouddiskrdbStore_->GetSrcCloudId(cloudId, srcCloudId);
        EXPECT_EQ(ret, E_RDB);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSrcCloudIdTest002 failed";
    }
    GTEST_LOG_(INFO) << "GetSrcCloudIdTest002 end";
}

/**
 * @tc.name: GetSrcCloudId
 * @tc.desc: Verify the CloudDiskRdbStore::GetSrcCloudId function
 * @tc.type: FUNC
 * @tc.require: ICPOAK
 */
HWTEST_F(CloudDiskRdbStoreTest, GetSrcCloudIdTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSrcCloudIdTest003 start";
    try {
        std::string cloudId = "";
        std::string srcCloudId = "";
        bool preCount = true;
        auto rdb = make_shared<RdbStoreMock>();
        clouddiskrdbStore_->rdbStore_ = rdb;
        auto transaction = make_shared<TransactionMock>();
        std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
        EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(), An<const std::vector<std::string> &>(), preCount))
        .WillOnce(Return(ByMove(rset)));
        EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
        EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_RDB));
        EXPECT_CALL(*rset, GetColumnIndex(_, _)).WillRepeatedly(Return(E_OK));
        int32_t ret = clouddiskrdbStore_->GetSrcCloudId(cloudId, srcCloudId);
        EXPECT_EQ(ret, E_RDB);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSrcCloudIdTest003 failed";
    }
    GTEST_LOG_(INFO) << "GetSrcCloudIdTest003 end";
}

/**
 * @tc.name: GetSrcCloudId
 * @tc.desc: Verify the CloudDiskRdbStore::GetSrcCloudId function
 * @tc.type: FUNC
 * @tc.require: ICPOAK
 */
HWTEST_F(CloudDiskRdbStoreTest, GetSrcCloudIdTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSrcCloudIdTest004 start";
    try {
        std::string cloudId = "";
        std::string srcCloudId = "";
        bool preCount = true;
        auto rdb = make_shared<RdbStoreMock>();
        clouddiskrdbStore_->rdbStore_ = rdb;
        auto transaction = make_shared<TransactionMock>();
        std::shared_ptr<ResultSetMock> rset = std::make_shared<ResultSetMock>();
        EXPECT_CALL(*rdb, QueryByStep(An<const AbsRdbPredicates &>(), An<const std::vector<std::string> &>(), preCount))
        .WillOnce(Return(ByMove(rset)));
        EXPECT_CALL(*rset, GoToNextRow()).WillRepeatedly(Return(E_OK));
        EXPECT_CALL(*rset, GetString(_, _)).WillRepeatedly(Return(E_OK));
        EXPECT_CALL(*rset, GetColumnIndex(_, _)).WillRepeatedly(Return(E_OK));
        int32_t ret = clouddiskrdbStore_->GetSrcCloudId(cloudId, srcCloudId);
        EXPECT_EQ(ret, E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSrcCloudIdTest004 failed";
    }
    GTEST_LOG_(INFO) << "GetSrcCloudIdTest004 end";
}

/**
 * @tc.name: TriggerSyncForWrite
 * @tc.desc: Verify the CloudDiskRdbStore::TriggerSyncForWrite function
 * @tc.type: FUNC
 * @tc.require: ICGORT
 */
HWTEST_F(CloudDiskRdbStoreTest, TriggerSyncForWriteTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TriggerSyncForWriteTest001 start";
    try {
        auto rdb = make_shared<RdbStoreMock>();
        const std::string fileName = "Test";
        const std::string parentCloudId = "100";
        clouddiskrdbStore_->rdbStore_ = rdb;
        clouddiskrdbStore_->TriggerSyncForWrite(fileName, parentCloudId);
        EXPECT_EQ(fileName, "Test");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TriggerSyncForWriteTest001 failed";
    }
    GTEST_LOG_(INFO) << "TriggerSyncForWriteTest001 end";
}
 
/**
 * @tc.name: TriggerSyncForWrite
 * @tc.desc: Verify the CloudDiskRdbStore::TriggerSyncForWrite function
 * @tc.type: FUNC
 * @tc.require: ICGORT
 */
HWTEST_F(CloudDiskRdbStoreTest, TriggerSyncForWriteTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TriggerSyncForWriteTest002 start";
    try {
        auto rdb = make_shared<RdbStoreMock>();
        const std::string fileName = "mock";
        const std::string parentCloudId = "100";
        clouddiskrdbStore_->rdbStore_ = rdb;
        clouddiskrdbStore_->TriggerSyncForWrite(fileName, parentCloudId);
        EXPECT_EQ(fileName, "mock");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TriggerSyncForWriteTest002 failed";
    }
    GTEST_LOG_(INFO) << "TriggerSyncForWriteTest002 end";
}
} // namespace OHOS::FileManagement::CloudDisk::Test