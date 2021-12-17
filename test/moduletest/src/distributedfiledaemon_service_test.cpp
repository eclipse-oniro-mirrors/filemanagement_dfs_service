/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <sys/mount.h>
#include <system_error>

#include "device/device_manager_agent.h"
#include "mountpoint/mount_point.h"
#include "utils_directory.h"
#include "utils_log.h"
namespace OHOS {
namespace Storage {
namespace DistributedFile {
namespace Test {
using namespace testing::ext;
using namespace std;

class DistributedFileDaemonServiceTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedFileDaemonServiceTest::SetUpTestCase(void)
{
    // input testsuit setup step，setup invoked before all testcases
}

void DistributedFileDaemonServiceTest::TearDownTestCase(void)
{
    // input testsuit teardown step，teardown invoked after all testcases
}

void DistributedFileDaemonServiceTest::SetUp(void)
{
    // input testcase setup step，setup invoked before each testcases
}

void DistributedFileDaemonServiceTest::TearDown(void)
{
    // input testcase teardown step，teardown invoked after each testcases
}

/**
 * @tc.name: mount_test_001
 * @tc.desc: Verify the mount/umount function.
 * @tc.type: FUNC
 * @tc.require: Issue Number
 */
HWTEST_F(DistributedFileDaemonServiceTest, mount_umount_test_001, TestSize.Level1)
{
    auto mp = make_unique<OHOS::Storage::DistributedFile::MountPoint>(
        OHOS::Storage::DistributedFile::Utils::MountArgumentDescriptors::Alpha(9527));

    shared_ptr<OHOS::Storage::DistributedFile::MountPoint> smp = move(mp);

    try {
        smp->Mount();
        smp->Umount();
        LOGE("testcase run OK");
    } catch (const exception &e) {
        LOGE("%{public}s", e.what());
        EXPECT_EQ(0, 1);
    }
    EXPECT_EQ(0, 0);
}
} // namespace Test
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS