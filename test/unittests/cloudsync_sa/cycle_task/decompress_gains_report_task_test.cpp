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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "cloud_pref_impl_mock.h"
#include "cycle_task.h"
#include "decompress_gains_report_task.h"
#include "decompress_gains_reporter.h"
#include "dfs_error.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {
using namespace testing;
using namespace testing::ext;

class DecompressGainsReportTaskTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static inline std::shared_ptr<DecompressGainsReportTask> task_ = nullptr;
    static inline std::shared_ptr<CloudPrefImplMock> cloudPrefImplMock_ = nullptr;
};

void DecompressGainsReportTaskTest::SetUpTestCase(void)
{
    cloudPrefImplMock_ = std::make_shared<CloudPrefImplMock>();
    CloudPrefImplMock::proxy_ = cloudPrefImplMock_;
    task_ = std::make_shared<DecompressGainsReportTask>(nullptr);
}

void DecompressGainsReportTaskTest::TearDownTestCase(void)
{
    task_ = nullptr;
    cloudPrefImplMock_ = nullptr;
    CloudPrefImplMock::proxy_ = nullptr;
}

void DecompressGainsReportTaskTest::SetUp(void) {}
void DecompressGainsReportTaskTest::TearDown(void) {}

/**
 * @tc.name: TaskName_001
 * @tc.desc: Verify task name is "decompress_gains_report_task"
 * @tc.type: FUNC
 */
HWTEST_F(DecompressGainsReportTaskTest, TaskName_001, TestSize.Level1)
{
    EXPECT_EQ(task_->GetTaskName(), "decompress_gains_report_task");
}

/**
 * @tc.name: RunTaskForSwitchOff_001
 * @tc.desc: Verify runTaskForSwitchOff_ is true (task runs even when cloud sync is off)
 * @tc.type: FUNC
 */
HWTEST_F(DecompressGainsReportTaskTest, RunTaskForSwitchOff_001, TestSize.Level1)
{
    EXPECT_TRUE(task_->runTaskForSwitchOff_);
}

/**
 * @tc.name: IntervalTime_001
 * @tc.desc: Verify interval is ONE_DAY (86400 seconds)
 * @tc.type: FUNC
 */
HWTEST_F(DecompressGainsReportTaskTest, IntervalTime_001, TestSize.Level1)
{
    constexpr int32_t ONE_DAY_SEC = 24 * 60 * 60;
    EXPECT_EQ(task_->intervalTime_, ONE_DAY_SEC);
}

/**
 * @tc.name: RunTaskForBundle_001
 * @tc.desc: Verify RunTaskForBundle calls DecompressGainsReporter::Report and returns E_OK
 * @tc.type: FUNC
 */
HWTEST_F(DecompressGainsReportTaskTest, RunTaskForBundle_001, TestSize.Level1)
{
    int32_t ret = task_->RunTaskForBundle(100, "decompress");
    EXPECT_EQ(ret, E_OK);
}

/**
 * @tc.name: Destructor_001
 * @tc.desc: Verify destructor does not crash
 * @tc.type: FUNC
 */
HWTEST_F(DecompressGainsReportTaskTest, Destructor_001, TestSize.Level1)
{
    auto *task = new DecompressGainsReportTask(nullptr);
    EXPECT_NO_FATAL_FAILURE({ delete task; });
}
} // namespace OHOS::FileManagement::CloudSync
