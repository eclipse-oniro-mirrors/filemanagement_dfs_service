/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "database_manager.h"

namespace OHOS::FileManagement::CloudDisk::Test {
using namespace testing;
using namespace testing::ext;
using namespace std;

class DatabaseManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static inline shared_ptr<DatabaseManager> databaseManager_ = nullptr;
};

void DatabaseManagerTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
    databaseManager_ = make_shared<DatabaseManager>();
}

void DatabaseManagerTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
    databaseManager_ = nullptr;
}

void DatabaseManagerTest::SetUp(void)
{
    GTEST_LOG_(INFO) << "SetUp";
}

void DatabaseManagerTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "TearDown";
}

/**
 * @tc.name: GetRdbStoreTest001
 * @tc.desc: Verify the GetRdbStore function
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(DatabaseManagerTest, GetRdbStoreTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetRdbStoreTest001 start";
    try {
        const std::string bundleName = "GetRdbStoreTest";
        const int32_t userId = 123456789;
        databaseManager_->GetRdbStore(bundleName, userId);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetRdbStoreTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetRdbStoreTest001 end";
}

/**
 * @tc.name: GetInstanceTest001
 * @tc.desc: Verify the GetInstance function
 * @tc.type: FUNC
 * @tc.require: issuesIB4SSZ
 */
HWTEST_F(DatabaseManagerTest, GetInstanceTest001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetInstanceTest001 start";
    try{
        DatabaseManager &instance = databaseManager_->GetInstance();
        EXPECT_NE(&instance, nullptr);
    }
    catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetInstanceTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetInstanceTest001 end";
}

/**
 * @tc.name: ClearRdbStoreTest001
 * @tc.desc: Verify the ClearRdbStore function
 * @tc.type: FUNC
 * @tc.require: issuesIB4SSZ
 */
HWTEST_F(DatabaseManagerTest, ClearRdbStoreTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ClearRdbStore001 start";
    try{
        databaseManager_->ClearRdbStore();
        EXPECT_TRUE(true);
    }
    catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ClearRdbStoreTest001 failed";
    }
    GTEST_LOG_(INFO) << "ClearRdbStoreTest001 end";
}
} // namespace OHOS::FileManagement::CloudDisk::Test
