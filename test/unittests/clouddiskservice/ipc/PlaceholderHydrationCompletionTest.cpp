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

#include <cstdlib>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

#include "assistant.h"
#include "cloud_disk_service_error.h"
#include "placeholder_callback_manager.h"
#include "placeholder_helper.h"
#include "placeholder_progress_manager.h"
#include "placeholder_task_manager.h"

namespace OHOS::FileManagement::CloudDiskService::Test {
using namespace testing;
using namespace testing::ext;

namespace {
const std::string TEST_TEMP_ROOT = "/data/test";
const std::string TEST_SYNC_FOLDER = "/storage/Users/currentUser/sync";
const std::string TEST_BUNDLE_NAME = "com.example.cloud.disk";
constexpr uint32_t TEST_SYNC_FOLDER_INDEX = 100;

UniqueFd OpenTaskFd()
{
    char pathTemplate[] = "/data/test/cloud_disk_task_XXXXXX";
    UniqueFd fd(mkstemp(pathTemplate));
    if (fd < 0) {
        return UniqueFd(-1);
    }
    if (unlink(pathTemplate) != 0) {
        return UniqueFd(-1);
    }
    return fd;
}

CallbackExecuteRequest MakeRequest(const std::vector<uint8_t> &reqKey,
                                   uint64_t offset = 0,
                                   const std::vector<uint8_t> &data = {},
                                   uint64_t totalSize = 0,
                                   bool isComplete = true)
{
    CallbackExecuteRequest request;
    request.reqKey = reqKey;
    request.syncFolder = TEST_SYNC_FOLDER;
    request.filePath = "file.txt";
    request.offset = offset;
    request.size = data.size();
    request.data = data;
    request.totalSize = totalSize;
    request.isComplete = isComplete;
    return request;
}

} // namespace

class PlaceholderHydrationCompletionTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        struct stat st {};
        if (stat(TEST_TEMP_ROOT.c_str(), &st) != 0) {
            mkdir(TEST_TEMP_ROOT.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
        }
    }

    void SetUp() override
    {
        PlaceholderTaskManager::GetInstance().StopScheduler();
        mock_ = std::make_shared<AssistantMock>();
        Assistant::ins = mock_;
        Assistant::mockErrno = 0;
        Assistant::mockFdApi = false;
        Assistant::mockPwriteApi = false;
        Assistant::mockFsyncApi = false;
    }

    void TearDown() override
    {
        PlaceholderTaskManager::GetInstance().StopScheduler();
        PlaceholderProgressManager::GetInstance().Drain();
        PlaceholderProgressManager::GetInstance().Clear();
        PlaceholderCallbackManager::GetInstance().ClearBySyncFolder(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX);
        Assistant::ins = nullptr;
        Assistant::mockErrno = 0;
        Assistant::mockFdApi = false;
        Assistant::mockPwriteApi = false;
        Assistant::mockFsyncApi = false;
        mock_ = nullptr;
    }

    static int32_t ActivateTask(PlaceholderTaskManager &manager,
                                const PlaceholderTaskManager::RequestKey &reqKey,
                                UniqueFd outputFd = OpenTaskFd())
    {
        auto task = manager.taskMap_.at(reqKey);
        int32_t taskFd = outputFd.Get();
        task->outputFd = std::move(outputFd);
        task->fetchDispatched = true;
        task->state = PlaceholderTaskState::IN_PROGRESS;
        manager.NotifyProgressLocked(task);
        return taskFd;
    }

    std::shared_ptr<AssistantMock> mock_;
};

/**
 * @tc.name: HydrationTimestamps_RestoredPerWrite_001
 * @tc.desc: Timestamps are restored after every written chunk, not only on completion.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderHydrationCompletionTest, HydrationTimestamps_RestoredPerWrite_001, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto task = manager.taskMap_.at(key);
    int32_t taskFd = ActivateTask(manager, key);
    ASSERT_GE(taskFd, 0);
    task->totalSize = 4;
    task->timestampsSaved = true;
    task->savedAtime = {111, 222};
    task->savedMtime = {333, 444};
    Assistant::mockPwriteApi = true;
    EXPECT_CALL(*mock_, Pwrite(taskFd, _, 4, 0)).WillOnce(Return(4));
    EXPECT_CALL(*mock_, futimens(taskFd, _))
        .WillOnce(Invoke([](int, const struct timespec *times) {
            EXPECT_EQ(times[0].tv_sec, 111);
            EXPECT_EQ(times[0].tv_nsec, 222);
            EXPECT_EQ(times[1].tv_sec, 333);
            EXPECT_EQ(times[1].tv_nsec, 444);
            return 0;
        }));
    EXPECT_CALL(*mock_, fgetxattr(taskFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
        .Times(2)
        .WillRepeatedly(Invoke([](int, const char *, void *value, size_t) {
            *static_cast<uint8_t *>(value) = MakeFileSyncState(PLACEHOLDER_STATE_UNHYDRATED, 0);
            return static_cast<ssize_t>(sizeof(uint8_t));
        }));
    EXPECT_CALL(*mock_, fsetxattr(taskFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t), 0))
        .WillOnce(Return(0));

    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2, 3, 4}, 4, false)),
              E_OK);
    EXPECT_EQ(task->state.load(), PlaceholderTaskState::IN_PROGRESS);
}

} // namespace OHOS::FileManagement::CloudDiskService::Test
