/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "system_mock.h"
#include "task_state_manager.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

#include "refbase.h"

namespace OHOS::FileManagement::CloudSync::Test {
using namespace testing;
using namespace testing::ext;
using namespace std;

class MockTaskStateManager : public TaskStateManager {
public:
    MOCK_METHOD0(GetInstance, TaskStateManager&());
    MOCK_METHOD1(DelayUnloadTask, void(bool needSetCritical));
    MOCK_METHOD0(CancelUnloadTask, void());
    MOCK_METHOD0(StartTask, void());
    MOCK_METHOD2(StartTask, void(string bundleName, TaskType task));
    MOCK_METHOD2(CompleteTask, void(string bundleName, TaskType task));
    MOCK_METHOD2(HasTask, bool(string bundleName, TaskType task));
};

class TaskStateManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static inline std::shared_ptr<system::SystemMock> systemPtr{nullptr};
};

void TaskStateManagerTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
    systemPtr = make_shared<system::SystemMock>();
    system::ISystem::system_ = systemPtr;
}

void TaskStateManagerTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
}

void TaskStateManagerTest::SetUp(void)
{
    GTEST_LOG_(INFO) << "SetUp";
}

void TaskStateManagerTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "TearDown";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_001 Start";
    try {
        TaskStateManager::GetInstance();
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_001 Start ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_001 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_002 Start";
    try {
        TaskStateManager::GetInstance().HasTask("testBundleName", TaskType::UPLOAD_ASSET_TASK);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_002 Start ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_002 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_003 Start";
    try {
        TaskStateManager::GetInstance().HasTask("testBundleName", TaskType::DOWNLOAD_ASSET_TASK);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_003 Start ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_003 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_004 Start";
    try {
        MockTaskStateManager mockTaskStateManager;
        EXPECT_CALL(mockTaskStateManager, GetInstance())
            .WillOnce(ReturnRef(mockTaskStateManager));
        
        mockTaskStateManager.GetInstance();
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_004 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_004 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_005 Start";
    try {
        MockTaskStateManager mockTaskStateManager;
        EXPECT_CALL(mockTaskStateManager, DelayUnloadTask(true));
        mockTaskStateManager.DelayUnloadTask(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_005 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_005 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_006 Start";
    try {
        MockTaskStateManager mockTaskStateManager;
        EXPECT_CALL(mockTaskStateManager, CancelUnloadTask());
        mockTaskStateManager.CancelUnloadTask();
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_006 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_006 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_007, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_007 Start";
    try {
        MockTaskStateManager mockTaskStateManager;
        EXPECT_CALL(mockTaskStateManager, StartTask());
        mockTaskStateManager.StartTask();
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_007 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_007 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_008, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_008 Start";
    try {
        MockTaskStateManager mockTaskStateManager;
        string bundleName = "testbundleName";
        EXPECT_CALL(mockTaskStateManager, StartTask(bundleName, TaskType::SYNC_TASK));
        EXPECT_CALL(mockTaskStateManager, StartTask(bundleName, TaskType::DOWNLOAD_TASK));
        EXPECT_CALL(mockTaskStateManager, StartTask(bundleName, TaskType::UPLOAD_ASSET_TASK));
        EXPECT_CALL(mockTaskStateManager, StartTask(bundleName, TaskType::DOWNLOAD_ASSET_TASK));
        EXPECT_CALL(mockTaskStateManager, StartTask(bundleName, TaskType::DOWNLOAD_REMOTE_ASSET_TASK));
        EXPECT_CALL(mockTaskStateManager, StartTask(bundleName, TaskType::DOWNLOAD_THUMB_TASK));
        EXPECT_CALL(mockTaskStateManager, StartTask(bundleName, TaskType::DISABLE_CLOUD_TASK));

        mockTaskStateManager.StartTask(bundleName, TaskType::SYNC_TASK);
        mockTaskStateManager.StartTask(bundleName, TaskType::DOWNLOAD_TASK);
        mockTaskStateManager.StartTask(bundleName, TaskType::UPLOAD_ASSET_TASK);
        mockTaskStateManager.StartTask(bundleName, TaskType::DOWNLOAD_ASSET_TASK);
        mockTaskStateManager.StartTask(bundleName, TaskType::DOWNLOAD_REMOTE_ASSET_TASK);
        mockTaskStateManager.StartTask(bundleName, TaskType::DOWNLOAD_THUMB_TASK);
        mockTaskStateManager.StartTask(bundleName, TaskType::DISABLE_CLOUD_TASK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_008 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_008 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_009, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_009 Start";
    try {
        MockTaskStateManager mockTaskStateManager;
        string bundleName = "testbundleName";
        EXPECT_CALL(mockTaskStateManager, CompleteTask(bundleName, TaskType::SYNC_TASK));
        EXPECT_CALL(mockTaskStateManager, CompleteTask(bundleName, TaskType::DOWNLOAD_TASK));
        EXPECT_CALL(mockTaskStateManager, CompleteTask(bundleName, TaskType::UPLOAD_ASSET_TASK));
        EXPECT_CALL(mockTaskStateManager, CompleteTask(bundleName, TaskType::DOWNLOAD_ASSET_TASK));
        EXPECT_CALL(mockTaskStateManager, CompleteTask(bundleName, TaskType::DOWNLOAD_REMOTE_ASSET_TASK));
        EXPECT_CALL(mockTaskStateManager, CompleteTask(bundleName, TaskType::DOWNLOAD_THUMB_TASK));
        EXPECT_CALL(mockTaskStateManager, CompleteTask(bundleName, TaskType::DISABLE_CLOUD_TASK));

        mockTaskStateManager.CompleteTask(bundleName, TaskType::SYNC_TASK);
        mockTaskStateManager.CompleteTask(bundleName, TaskType::DOWNLOAD_TASK);
        mockTaskStateManager.CompleteTask(bundleName, TaskType::UPLOAD_ASSET_TASK);
        mockTaskStateManager.CompleteTask(bundleName, TaskType::DOWNLOAD_ASSET_TASK);
        mockTaskStateManager.CompleteTask(bundleName, TaskType::DOWNLOAD_REMOTE_ASSET_TASK);
        mockTaskStateManager.CompleteTask(bundleName, TaskType::DOWNLOAD_THUMB_TASK);
        mockTaskStateManager.CompleteTask(bundleName, TaskType::DISABLE_CLOUD_TASK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_009 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_009 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_010, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_010 Start";
    try {
        MockTaskStateManager mockTaskStateManager;
        string bundleName = "testbundleName";
        EXPECT_CALL(mockTaskStateManager, HasTask(bundleName, TaskType::SYNC_TASK));
        EXPECT_CALL(mockTaskStateManager, HasTask(bundleName, TaskType::DOWNLOAD_TASK));
        EXPECT_CALL(mockTaskStateManager, HasTask(bundleName, TaskType::UPLOAD_ASSET_TASK));
        EXPECT_CALL(mockTaskStateManager, HasTask(bundleName, TaskType::DOWNLOAD_ASSET_TASK));
        EXPECT_CALL(mockTaskStateManager, HasTask(bundleName, TaskType::DOWNLOAD_REMOTE_ASSET_TASK));
        EXPECT_CALL(mockTaskStateManager, HasTask(bundleName, TaskType::DOWNLOAD_THUMB_TASK));
        EXPECT_CALL(mockTaskStateManager, HasTask(bundleName, TaskType::DISABLE_CLOUD_TASK));

        mockTaskStateManager.HasTask(bundleName, TaskType::SYNC_TASK);
        mockTaskStateManager.HasTask(bundleName, TaskType::DOWNLOAD_TASK);
        mockTaskStateManager.HasTask(bundleName, TaskType::UPLOAD_ASSET_TASK);
        mockTaskStateManager.HasTask(bundleName, TaskType::DOWNLOAD_ASSET_TASK);
        mockTaskStateManager.HasTask(bundleName, TaskType::DOWNLOAD_REMOTE_ASSET_TASK);
        mockTaskStateManager.HasTask(bundleName, TaskType::DOWNLOAD_THUMB_TASK);
        mockTaskStateManager.HasTask(bundleName, TaskType::DISABLE_CLOUD_TASK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_010 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_010 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_011, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_011 Start";
    try {
        MockTaskStateManager mockTaskStateManager;
        string bundleName = "testbundleName";
        EXPECT_CALL(mockTaskStateManager, HasTask(bundleName, TaskType::SYNC_TASK))
            .WillOnce(Return(true))
            .WillOnce(Return(false));

        EXPECT_TRUE(mockTaskStateManager.HasTask(bundleName, TaskType::SYNC_TASK));
        EXPECT_FALSE(mockTaskStateManager.HasTask(bundleName, TaskType::SYNC_TASK));
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_011 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_011 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_012, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_012 Start";
    try {
        TaskStateManager taskStateManager;
        string bundleName = "testBundleName";
        taskStateManager.StartTask();
        taskStateManager.StartTask(bundleName, TaskType::SYNC_TASK);
        taskStateManager.StartTask();
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_012 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_012 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_013, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_013 Start";
    try {
        TaskStateManager taskStateManager;
        string bundleName = "testBundleName";
        taskStateManager.StartTask(bundleName, TaskType::SYNC_TASK);
        taskStateManager.CompleteTask(bundleName, TaskType::SYNC_TASK);
        taskStateManager.StartTask();
        taskStateManager.GetInstance();
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_013 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_013 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_014, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_014 Start";
    try {
        TaskStateManager taskStateManager;
        string bundleName = "testBundleName";
        taskStateManager.StartTask(bundleName, TaskType::SYNC_TASK);
        taskStateManager.CompleteTask(bundleName, TaskType::SYNC_TASK);
        taskStateManager.StartTask();
        taskStateManager.GetInstance();
        EXPECT_FALSE(taskStateManager.HasTask(bundleName, TaskType::SYNC_TASK));
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_014 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_014 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_015, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_015 Start";
    try {
        TaskStateManager taskStateManager;
        string bundleName = "testBundleName";
        taskStateManager.StartTask(bundleName, TaskType::SYNC_TASK);
        EXPECT_TRUE(taskStateManager.HasTask(bundleName, TaskType::SYNC_TASK));
        taskStateManager.StartTask(bundleName, TaskType::DOWNLOAD_TASK);
        EXPECT_TRUE(taskStateManager.HasTask(bundleName, TaskType::DOWNLOAD_TASK));

        taskStateManager.StartTask(bundleName, TaskType::CLEAN_TASK);
        EXPECT_TRUE(taskStateManager.HasTask(bundleName, TaskType::CLEAN_TASK));
        taskStateManager.StartTask(bundleName, TaskType::UPLOAD_ASSET_TASK);
        EXPECT_TRUE(taskStateManager.HasTask(bundleName, TaskType::UPLOAD_ASSET_TASK));

        taskStateManager.StartTask(bundleName, TaskType::DOWNLOAD_ASSET_TASK);
        EXPECT_TRUE(taskStateManager.HasTask(bundleName, TaskType::DOWNLOAD_ASSET_TASK));
        taskStateManager.StartTask(bundleName, TaskType::DOWNLOAD_REMOTE_ASSET_TASK);
        EXPECT_TRUE(taskStateManager.HasTask(bundleName, TaskType::DOWNLOAD_REMOTE_ASSET_TASK));

        taskStateManager.StartTask(bundleName, TaskType::DOWNLOAD_THUMB_TASK);
        EXPECT_TRUE(taskStateManager.HasTask(bundleName, TaskType::DOWNLOAD_THUMB_TASK));
        taskStateManager.StartTask(bundleName, TaskType::DISABLE_CLOUD_TASK);
        EXPECT_TRUE(taskStateManager.HasTask(bundleName, TaskType::DISABLE_CLOUD_TASK));
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_015 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_015 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_016, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_016 Start";
    try {
        TaskStateManager taskStateManager;
        string bundleName = "testBundleName";
        
        taskStateManager.StartTask();
        EXPECT_FALSE(taskStateManager.HasTask(bundleName, TaskType::SYNC_TASK));

        taskStateManager.StartTask(bundleName, TaskType::SYNC_TASK);
        taskStateManager.CompleteTask(bundleName, TaskType::SYNC_TASK);
        EXPECT_FALSE(taskStateManager.HasTask(bundleName, TaskType::SYNC_TASK));

        taskStateManager.StartTask();
        taskStateManager.GetInstance();
        EXPECT_FALSE(taskStateManager.HasTask(bundleName, TaskType::SYNC_TASK));
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_016 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_016 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_017, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_017 Start";
    try {
        TaskStateManager taskStateManager;
        string bundleName = "testBundleName";
        TaskType task = TaskType::SYNC_TASK;
        
        taskStateManager.CompleteTask(bundleName, task);
        auto ret = taskStateManager.taskMaps_.find(bundleName);
        EXPECT_EQ(ret, taskStateManager.taskMaps_.end());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_017 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_017 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_018, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_018 Start";
    try {
        auto taskStateManager_ = make_shared<TaskStateManager>();
        taskStateManager_->criticalStatus_ = true;
        
        EXPECT_CALL(*systemPtr, GetParameter(_, _)).Times(2).WillOnce(Return("true")).WillOnce(Return("true"));
        taskStateManager_->DelayUnloadTask(true);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_018 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_018 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_019, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_019 Start";
    try {
        TaskStateManager taskStateManager;
        string bundleName = "testBundleName";
        TaskType task = TaskType::SYNC_TASK;
        taskStateManager.taskMaps_.clear();
        
        EXPECT_CALL(*systemPtr, GetParameter(_, _)).Times(2).WillOnce(Return("true")).WillOnce(Return("true"));
        taskStateManager.CompleteTask(bundleName, task);
        EXPECT_TRUE(taskStateManager.taskMaps_.empty());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_019 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_019 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_020, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_020 Start";
    try {
        TaskStateManager taskStateManager;
        taskStateManager.taskMaps_.clear();

        EXPECT_CALL(*systemPtr, GetParameter(_, _)).Times(2).WillOnce(Return("true")).WillOnce(Return("true"));
        taskStateManager.StartTask();
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_020 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_020 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_021, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_021 Start";
    try {
        TaskStateManager taskStateManager;
        string bundleName = "testBundleName";
        TaskType task = TaskType::SYNC_TASK;
        taskStateManager.criticalStatus_ = false;

        EXPECT_CALL(*systemPtr, GetParameter(_, _)).Times(2).WillOnce(Return("true")).WillOnce(Return("true"));
        taskStateManager.StartTask(bundleName, task);
        EXPECT_TRUE(taskStateManager.criticalStatus_);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_021 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_021 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_022, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_022 Start";
    try {
        TaskStateManager taskStateManager;
        string bundleName = "testBundleName";
        TaskType task = TaskType::SYNC_TASK;
        taskStateManager.criticalStatus_ = true;

        EXPECT_CALL(*systemPtr, GetParameter(_, _)).Times(2).WillOnce(Return("true")).WillOnce(Return("true"));
        taskStateManager.StartTask(bundleName, task);
        EXPECT_TRUE(taskStateManager.criticalStatus_);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_022 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_022 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_023, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_023 Start";
    try {
        auto taskStateManager_ = make_shared<TaskStateManager>();
        taskStateManager_->criticalStatus_ = true;
        
        EXPECT_CALL(*systemPtr, GetParameter(_, _)).Times(2).WillOnce(Return("true")).WillOnce(Return("true"));
        taskStateManager_->DelayUnloadTask(false);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_023 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_023 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_024, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_024 Start";
    try {
        auto taskStateManager_ = make_shared<TaskStateManager>();
        taskStateManager_->criticalStatus_ = false;
        
        EXPECT_CALL(*systemPtr, GetParameter(_, _)).Times(2).WillOnce(Return("true")).WillOnce(Return("true"));
        taskStateManager_->DelayUnloadTask(true);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_024 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_024 End";
}

HWTEST_F(TaskStateManagerTest, TaskStateManagerTest_025, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "TaskStateManagerTest_025 Start";
    try {
        auto taskStateManager_ = make_shared<TaskStateManager>();
        taskStateManager_->criticalStatus_ = false;
        
        EXPECT_CALL(*systemPtr, GetParameter(_, _)).Times(2).WillOnce(Return("true")).WillOnce(Return("true"));
        taskStateManager_->DelayUnloadTask(false);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "TaskStateManagerTest_025 ERROR";
    }
    GTEST_LOG_(INFO) << "TaskStateManagerTest_025 End";
}
}