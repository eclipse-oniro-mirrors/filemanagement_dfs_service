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

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <dlfcn.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <mutex>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#include "assistant.h"
#include "cloud_disk_service_error.h"
#include "iremote_stub.h"
#include "placeholder_callback_manager.h"
#include "placeholder_helper.h"
#include "placeholder_progress_manager.h"
#include "placeholder_task_manager.h"

namespace OHOS::FileManagement::CloudDiskService::Test {
using namespace testing;
using namespace testing::ext;

namespace {
const std::string TEST_TEMP_ROOT = "/data/test";
// Mirrors the production hydration temp layout; the build defines override the base to
// /data/test so real-activation tests never touch the service el2 directories.
const std::string TEST_HYDRATION_TEMP_DIR =
    std::string(CLOUD_DISK_HYDRATION_TEMP_BASE) + "/100/clouddiskservice/hydrate_tmp";
const std::string TEST_SYNC_FOLDER = "/storage/Users/currentUser/sync";
const std::string TEST_BUNDLE_NAME = "com.example.cloud.disk";
constexpr uint32_t TEST_SYNC_FOLDER_INDEX = 100;
constexpr uint64_t TEST_DEVICE_ID = 1;
constexpr uint64_t TEST_INODE_ID = 100;
constexpr uint64_t REPLACED_TEST_INODE_ID = 101;
constexpr int32_t NON_EXECUTE_CALLBACK_TYPE = static_cast<int32_t>(CloudDiskCallbackType::DEHYDRATE);
constexpr int32_t INVALID_CALLBACK_TYPE = 3;
constexpr int32_t ADMISSION_AND_ACTIVATION_METADATA_QUERY_COUNT = 2;
static_assert(NON_EXECUTE_CALLBACK_TYPE == 2);

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

class HydrationTestFile {
public:
    HydrationTestFile()
    {
        char pathTemplate[] = "/data/test/cloud_disk_hydration_XXXXXX";
        fd_ = mkstemp(pathTemplate);
        path_ = pathTemplate;
        if (fd_ < 0) {
            Reset();
        }
    }

    ~HydrationTestFile()
    {
        Reset();
    }

    bool IsValid() const
    {
        return fd_ >= 0;
    }

    UniqueFd DuplicateFd() const
    {
        return UniqueFd(dup(fd_));
    }

    const std::string &GetPath() const
    {
        return path_;
    }

    bool Remove()
    {
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
        return !path_.empty() && unlink(path_.c_str()) == 0;
    }

    bool Replace()
    {
        if (fd_ < 0 || path_.empty()) {
            return false;
        }
        int32_t oldFd = fd_;
        fd_ = -1;
        if (unlink(path_.c_str()) != 0) {
            fd_ = oldFd;
            return false;
        }
        fd_ = open(path_.c_str(), O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR);
        close(oldFd);
        return fd_ >= 0;
    }

private:
    void Reset()
    {
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
        if (!path_.empty()) {
            unlink(path_.c_str());
            path_.clear();
        }
    }

    int32_t fd_ = -1;
    std::string path_;
};

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

auto ReturnPlaceholderState(uint8_t state)
{
    return [state](int, const char *, void *value, size_t size) {
        *static_cast<uint8_t *>(value) = MakeFileSyncState(state, 0);
        return static_cast<ssize_t>(size);
    };
}

class TaskProgressRecorder final : public IRemoteStub<ICloudDiskProgressCallback> {
public:
    void OnProgress(const HydrateProgress &progress) override
    {
        events.push_back(progress);
    }
    int32_t OnRemoteRequest(uint32_t, MessageParcel &, MessageParcel &, MessageOption &) override
    {
        return E_OK;
    }
    std::vector<HydrateProgress> events;
};

class ExecuteWriteBarrier {
public:
    void BlockFirstWrite()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        firstEntered_ = true;
        cv_.notify_all();
        cv_.wait(lock, [this] { return releaseFirst_; });
    }

    bool WaitForFirstWrite()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, WAIT_TIMEOUT, [this] { return firstEntered_; });
    }

    void CompleteSecondWrite()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        secondDone_ = true;
        cv_.notify_all();
    }

    bool ReleaseFirstAfterSecondWrite()
    {
        bool independent = false;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            independent = cv_.wait_for(lock, WAIT_TIMEOUT, [this] { return secondDone_; });
            releaseFirst_ = true;
        }
        cv_.notify_all();
        return independent;
    }

private:
    static constexpr auto WAIT_TIMEOUT = std::chrono::seconds(2);
    std::mutex mutex_;
    std::condition_variable cv_;
    bool firstEntered_ = false;
    bool releaseFirst_ = false;
    bool secondDone_ = false;
};

class HydrationCallbackStub final : public IRemoteStub<ICloudDiskServiceCallbackTable> {
public:
    void OnCallback(const CloudDiskCallbackReqHead &reqHead, CloudDiskCallbackContext &reqContext) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        callbackTypes.push_back(reqHead.callbackType);
        bool isFetch = reqHead.callbackType == CloudDiskCallbackType::FETCH_DATA && reqContext.fetchData != nullptr;
        called = called || isFetch;
        if (isFetch) {
            priority = reqContext.fetchData->priority;
        }
        cv.notify_all();
    }

    int32_t OnRemoteRequest(uint32_t, MessageParcel &, MessageParcel &, MessageOption &) override
    {
        return E_OK;
    }

    std::mutex mutex;
    std::condition_variable cv;
    bool called = false;
    CloudDiskHydratePriority priority = CLOUD_DISK_HYDRATE_PRIORITY_LOW;
    std::vector<CloudDiskCallbackType> callbackTypes;
};
} // namespace

class PlaceholderTaskManagerTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        struct stat st {};
        if (stat(TEST_TEMP_ROOT.c_str(), &st) != 0) {
            mkdir(TEST_TEMP_ROOT.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
        }
        // Real-activation tests run the production ActivateAndDispatch temp layout through the
        // utils_directory stub (mock/utils_directory_stub.cpp), whose access() probe is answered
        // by the mock as "exists", so the directory itself has to be created here beforehand
        // (mkdir is not mocked).
        size_t tempDirOffset = 0;
        while (tempDirOffset != std::string::npos) {
            tempDirOffset = TEST_HYDRATION_TEMP_DIR.find('/', tempDirOffset + 1);
            std::string subDir = tempDirOffset == std::string::npos
                                     ? TEST_HYDRATION_TEMP_DIR
                                     : TEST_HYDRATION_TEMP_DIR.substr(0, tempDirOffset);
            if (stat(subDir.c_str(), &st) != 0) {
                mkdir(subDir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
            }
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
        Assistant::mockSendfileApi = false;
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
        Assistant::mockSendfileApi = false;
        mock_ = nullptr;
    }

    void ExpectStateTransitions(int32_t taskFd, uint8_t &stateByte)
    {
        // The partial flip happens at activation; the completion path only flips to fully hydrated.
        constexpr int32_t stateUpdateCount = 1;                    // Fully hydrated.
        constexpr int32_t stateReadCount = stateUpdateCount * 2;   // Get + Set per update.
        EXPECT_CALL(*mock_, fgetxattr(taskFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
            .Times(stateReadCount)
            .WillRepeatedly(Invoke([&stateByte](int, const char *, void *value, size_t) {
                *static_cast<uint8_t *>(value) = stateByte;
                return static_cast<ssize_t>(sizeof(uint8_t));
            }));
        EXPECT_CALL(*mock_, fsetxattr(taskFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t), 0))
            .Times(stateUpdateCount)
            .WillRepeatedly(Invoke([&stateByte](int, const char *, const void *value, size_t, int) {
                stateByte = *static_cast<const uint8_t *>(value);
                return 0;
            }));
    }

    static int32_t ActivateTask(PlaceholderTaskManager &manager,
                                const PlaceholderTaskManager::RequestKey &reqKey,
                                UniqueFd outputFd = OpenTaskFd())
    {
        auto task = manager.taskMap_.at(reqKey);
        int32_t taskFd = outputFd.Get();
        task->outputFd = std::move(outputFd);
        // Mirror the production activation: a service-private temp file receives all
        // hydration bytes first. Keep the path on disk; EraseTaskLocked unlinks it.
        char tempPathTemplate[] = "/data/test/cloud_disk_hydration_tmp_XXXXXX";
        UniqueFd tempFd(mkstemp(tempPathTemplate));
        if (tempFd >= 0) {
            task->tempFd = std::move(tempFd);
            task->tempPath = tempPathTemplate;
            task->tempSize = 0;
        }
        task->fetchDispatched = true;
        task->state = PlaceholderTaskState::IN_PROGRESS;
        manager.MarkTaskActiveLocked(task);
        manager.NotifyProgressLocked(task);
        return taskFd;
    }

    static void ExpectActiveCount(PlaceholderTaskManager &manager, const std::string &bundleName, int32_t userId,
                                  size_t expected)
    {
        std::lock_guard<std::mutex> lock(manager.mapMutex_);
        auto key = PlaceholderTaskManager::MakeAppUserKey(bundleName, userId);
        auto item = manager.activeByKey_.find(key);
        if (expected == 0) {
            EXPECT_TRUE(item == manager.activeByKey_.end());
            return;
        }
        ASSERT_TRUE(item != manager.activeByKey_.end());
        EXPECT_EQ(item->second, expected);
    }

    void ExpectAdmissionMetadata(uint64_t inodeId, uint64_t logicalSize = 0)
    {
        EXPECT_CALL(*mock_, fstat(_, _)).WillOnce(Invoke([inodeId, logicalSize](int, struct stat *metadata) {
            metadata->st_dev = static_cast<dev_t>(TEST_DEVICE_ID);
            metadata->st_ino = static_cast<ino_t>(inodeId);
            metadata->st_mode = S_IFREG | S_IRUSR | S_IWUSR;
            metadata->st_size = static_cast<off_t>(logicalSize);
            return 0;
        }));
    }

    void ExpectAdmissionAndActivationMetadata(uint64_t admissionInode,
                                              uint64_t activationInode,
                                              uint64_t admissionSize = 0,
                                              uint64_t activationSize = 0)
    {
        EXPECT_CALL(*mock_, fstat(_, _))
            .Times(ADMISSION_AND_ACTIVATION_METADATA_QUERY_COUNT)
            .WillOnce(Invoke([admissionInode, admissionSize](int, struct stat *metadata) {
                metadata->st_dev = static_cast<dev_t>(TEST_DEVICE_ID);
                metadata->st_ino = static_cast<ino_t>(admissionInode);
                metadata->st_mode = S_IFREG | S_IRUSR | S_IWUSR;
                metadata->st_size = static_cast<off_t>(admissionSize);
                return 0;
            }))
            .WillOnce(Invoke([activationInode, activationSize](int, struct stat *metadata) {
                metadata->st_dev = static_cast<dev_t>(TEST_DEVICE_ID);
                metadata->st_ino = static_cast<ino_t>(activationInode);
                metadata->st_mode = S_IFREG | S_IRUSR | S_IWUSR;
                metadata->st_size = static_cast<off_t>(activationSize);
                return 0;
            }));
        // Validation reads the state once. When activation proceeds, the partial-state
        // flip at activation adds one more read plus the old-state read inside the set.
        constexpr int32_t validationOnlyReadCount = 1;
        constexpr int32_t activationProceedReadCount = 3; // Validation + partial flip + old-state read.
        const bool activationProceeds = admissionInode == activationInode;
        EXPECT_CALL(*mock_, fgetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
            .Times(activationProceeds ? activationProceedReadCount : validationOnlyReadCount)
            .WillRepeatedly(Invoke([](int, const char *, void *value, size_t) {
                *static_cast<uint8_t *>(value) = MakeFileSyncState(PLACEHOLDER_STATE_UNHYDRATED, 0);
                return static_cast<ssize_t>(sizeof(uint8_t));
            }));
        if (activationProceeds) {
            EXPECT_CALL(*mock_, fsetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t), 0))
                .WillOnce(Return(0));
        }
    }

    static void CheckCompletedProgress(const sptr<TaskProgressRecorder> &observer)
    {
        ASSERT_GE(observer->events.size(), 3u);
        EXPECT_EQ(observer->events.front().state, static_cast<int32_t>(HydrateProgressState::PENDING));
        EXPECT_EQ(observer->events[1].state, static_cast<int32_t>(HydrateProgressState::IN_PROGRESS));
        const auto &completed = observer->events.back();
        EXPECT_EQ(completed.state, static_cast<int32_t>(HydrateProgressState::COMPLETED));
        EXPECT_EQ(completed.filePath, TEST_SYNC_FOLDER + "/file.txt");
        EXPECT_EQ(completed.processedSize, 6u);
        EXPECT_EQ(completed.totalSize, 6u);
    }

    static int32_t CreateOwnedTask(PlaceholderTaskManager &manager, uint64_t accessorId,
        PlaceholderTaskManager::RequestKey &key, const std::string &filePath = "file.txt")
    {
        PlaceholderProgressContext context{100, TEST_SYNC_FOLDER + "/" + filePath};
        context.owner = {1, 10, accessorId};
        return manager.CreateHydrateTask(TEST_SYNC_FOLDER, filePath, TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
            CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key, context);
    }

    // The interceptors forward fstat/opendir/readdir/closedir to the mock unconditionally,
    // so tests that must observe the real filesystem delegate these calls back to the
    // real implementations resolved with dlsym(RTLD_NEXT).
    void DelegateRealFstat()
    {
        EXPECT_CALL(*mock_, fstat(_, _)).WillRepeatedly(Invoke([](int fd, struct stat *metadata) {
            auto realFstat = reinterpret_cast<int (*)(int, struct stat *)>(dlsym(RTLD_NEXT, "fstat"));
            return realFstat(fd, metadata);
        }));
    }

    void DelegateRealDirApis()
    {
        EXPECT_CALL(*mock_, opendir(_)).WillOnce(Invoke([](const char *path) {
            auto realOpendir = reinterpret_cast<DIR *(*)(const char *)>(dlsym(RTLD_NEXT, "opendir"));
            return realOpendir(path);
        }));
        EXPECT_CALL(*mock_, readdir(_)).WillRepeatedly(Invoke([](DIR *dir) {
            auto realReaddir = reinterpret_cast<struct dirent *(*)(DIR *)>(dlsym(RTLD_NEXT, "readdir"));
            return realReaddir(dir);
        }));
        EXPECT_CALL(*mock_, CloseDir(_)).WillOnce(Invoke([](DIR *dir) {
            auto realClosedir = reinterpret_cast<int (*)(DIR *)>(dlsym(RTLD_NEXT, "closedir"));
            return realClosedir(dir);
        }));
    }

    std::shared_ptr<AssistantMock> mock_;
};

/**
 * @tc.name: OpenValidatedHydrationFileErrorMapping_001
 * @tc.desc: Map a missing target and malformed persisted placeholder state to their dedicated errors.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, OpenValidatedHydrationFileErrorMapping_001, TestSize.Level1)
{
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    const std::string path = file.GetPath();
    ASSERT_TRUE(file.Remove());
    UniqueFd outputFd;
    HydrationFileMetadata metadata;
    EXPECT_EQ(OpenValidatedHydrationFile(TEST_TEMP_ROOT, path, outputFd, metadata), E_FILE_NOT_EXIST);

    HydrationTestFile malformedFile;
    ASSERT_TRUE(malformedFile.IsValid());
    ExpectAdmissionMetadata(TEST_INODE_ID);
    EXPECT_CALL(*mock_, fgetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t))).WillOnce(Return(0));
    EXPECT_EQ(OpenValidatedHydrationFile(TEST_TEMP_ROOT, malformedFile.GetPath(), outputFd, metadata),
              E_INVALID_PLACEHOLDER_STATE);

    Mock::VerifyAndClearExpectations(mock_.get());
    ExpectAdmissionMetadata(TEST_INODE_ID);
    EXPECT_CALL(*mock_, fgetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
        .WillOnce(Invoke([](int, const char *, void *, size_t) {
            errno = ERANGE;
            return -1;
        }));
    EXPECT_EQ(OpenValidatedHydrationFile(TEST_TEMP_ROOT, malformedFile.GetPath(), outputFd, metadata),
              E_INVALID_PLACEHOLDER_STATE);
}

/**
 * @tc.name: CreateHydrateTask_001
 * @tc.desc: Verify priority ordering, same-priority FIFO, and the per-file active-task guard.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, CreateHydrateTask_001, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey lowKey;
    PlaceholderTaskManager::RequestKey highKey;
    PlaceholderTaskManager::RequestKey secondHighKey;
    PlaceholderTaskManager::RequestKey duplicateKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "low.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_LOW, OpenTaskFd(), lowKey),
              E_OK);
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "high.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_HIGH, OpenTaskFd(), highKey),
              E_OK);
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "second-high.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_HIGH, OpenTaskFd(), secondHighKey),
              E_OK);
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "low.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), duplicateKey),
              E_HYDRATE_IN_PROGRESS);
    EXPECT_LT(manager.taskMap_.at(lowKey)->outputFd.Get(), 0);
    auto selected = manager.SelectNextTaskLocked();
    ASSERT_NE(selected, nullptr);
    EXPECT_EQ(selected->reqKey, highKey);
    selected->state = PlaceholderTaskState::IN_PROGRESS;
    manager.MarkTaskActiveLocked(selected);
    selected = manager.SelectNextTaskLocked();
    ASSERT_NE(selected, nullptr);
    EXPECT_EQ(selected->reqKey, secondHighKey);
    EXPECT_TRUE(manager.HasOutstandingTask(TEST_SYNC_FOLDER, "low.txt", TEST_SYNC_FOLDER_INDEX));
    EXPECT_FALSE(manager.HasOutstandingTask(TEST_SYNC_FOLDER, "low.txt", TEST_SYNC_FOLDER_INDEX + 1));
}

/**
 * @tc.name: CreateHydrateTask_002
 * @tc.desc: Reject each invalid task field and new work while the scheduler is stopping.
 * @tc.type: SECU
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, CreateHydrateTask_002, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key{1};
    EXPECT_EQ(manager.CreateHydrateTask("", "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_INVALID_ARG);
    EXPECT_TRUE(key.empty());
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_INVALID_ARG);
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", "", TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_INVALID_ARG);
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        static_cast<CloudDiskHydratePriority>(CLOUD_DISK_HYDRATE_PRIORITY_HIGH + 1),
                                        OpenTaskFd(), key),
              E_INVALID_ARG);
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, UniqueFd(-1), key),
              E_INVALID_ARG);

    EXPECT_CALL(*mock_, fstat(_, _)).WillOnce(Invoke([](int, struct stat *metadata) {
        metadata->st_size = 0;
        return 0;
    }));
    manager.stopping_ = true;
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_TRY_AGAIN);
    manager.stopping_ = false;
    EXPECT_TRUE(manager.taskMap_.empty());
}

/**
 * @tc.name: CreateHydrateTask_003
 * @tc.desc: Verify request-key recovery and create-sequence wrap protection with queued work.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, CreateHydrateTask_003, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    manager.nextReqKeyValue_ = 0;
    manager.nextCreateSeq_ = std::numeric_limits<uint64_t>::max();
    PlaceholderTaskManager::RequestKey firstKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "first.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), firstKey),
              E_OK);
    ASSERT_EQ(firstKey.size(), sizeof(uint64_t));
    EXPECT_EQ(firstKey.front(), 1U);
    EXPECT_EQ(manager.taskMap_.at(firstKey)->createSeq, 1U);

    manager.nextCreateSeq_ = std::numeric_limits<uint64_t>::max();
    PlaceholderTaskManager::RequestKey secondKey;
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "second.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), secondKey),
              E_TRY_AGAIN);
    EXPECT_TRUE(secondKey.empty());
    EXPECT_EQ(manager.taskMap_.size(), 1U);
}

/**
 * @tc.name: CreateHydrateTask_004
 * @tc.desc: Accept pending hydration tasks without per-app or global pending caps.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, CreateHydrateTask_004, TestSize.Level1)
{
    constexpr size_t taskCount = 100;
    constexpr size_t otherAppTaskCount = 20;
    auto &manager = PlaceholderTaskManager::GetInstance();
    for (size_t index = 0; index < taskCount; ++index) {
        PlaceholderTaskManager::RequestKey key;
        ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "bulk-" + std::to_string(index), TEST_BUNDLE_NAME,
                                            TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(),
                                            key),
                  E_OK);
    }
    EXPECT_EQ(manager.taskMap_.size(), taskCount);
    EXPECT_EQ(manager.taskKeyIndex_.size(), taskCount);
    EXPECT_TRUE(manager.activeByKey_.empty());

    PlaceholderTaskManager::RequestKey duplicateKey;
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "bulk-0", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_HIGH, OpenTaskFd(), duplicateKey),
              E_HYDRATE_IN_PROGRESS);

    for (size_t index = 0; index < otherAppTaskCount; ++index) {
        PlaceholderTaskManager::RequestKey key;
        ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "bulk-other-" + std::to_string(index),
                                            "com.example.other", TEST_SYNC_FOLDER_INDEX,
                                            CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
                  E_OK);
    }
    EXPECT_EQ(manager.taskMap_.size(), taskCount + otherAppTaskCount);
    EXPECT_EQ(manager.taskKeyIndex_.size(), taskCount + otherAppTaskCount);
}

/**
 * @tc.name: HydrationQueueIndex_001
 * @tc.desc: Remove the task identity index and active count after cancellation and allow a new task.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationQueueIndex_001, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "indexed.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    EXPECT_TRUE(manager.HasOutstandingTask(TEST_SYNC_FOLDER, "indexed.txt", TEST_SYNC_FOLDER_INDEX));
    EXPECT_EQ(manager.taskKeyIndex_.size(), 1U);
    ASSERT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "indexed.txt", TEST_SYNC_FOLDER_INDEX), E_OK);
    EXPECT_FALSE(manager.HasOutstandingTask(TEST_SYNC_FOLDER, "indexed.txt", TEST_SYNC_FOLDER_INDEX));
    EXPECT_TRUE(manager.taskMap_.empty());
    EXPECT_TRUE(manager.taskKeyIndex_.empty());
    EXPECT_TRUE(manager.activeByKey_.empty());

    PlaceholderTaskManager::RequestKey recreatedKey;
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "indexed.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), recreatedKey),
              E_OK);
    EXPECT_EQ(manager.taskKeyIndex_.size(), 1U);
}

/**
 * @tc.name: HydrationQueueIndex_StaleMissing_001
 * @tc.desc: Self-heal a stale index whose task record is missing.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationQueueIndex_StaleMissing_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "stale-has.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    {
        std::lock_guard<std::mutex> lock(manager.mapMutex_);
        manager.taskMap_.erase(key);
    }
    EXPECT_FALSE(manager.HasOutstandingTask(TEST_SYNC_FOLDER, "stale-has.txt", TEST_SYNC_FOLDER_INDEX));
    EXPECT_TRUE(manager.taskKeyIndex_.empty());

    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "stale-cancel.txt", TEST_BUNDLE_NAME,
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    {
        std::lock_guard<std::mutex> lock(manager.mapMutex_);
        manager.taskMap_.erase(key);
    }
    EXPECT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "stale-cancel.txt", TEST_SYNC_FOLDER_INDEX),
              E_NO_HYDRATION_IN_PROGRESS);
    EXPECT_TRUE(manager.taskKeyIndex_.empty());

    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "stale-prepare.txt", TEST_BUNDLE_NAME,
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    {
        std::lock_guard<std::mutex> lock(manager.mapMutex_);
        manager.taskMap_.erase(key);
    }
    PlaceholderTaskManager::RequestKey recreatedKey;
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "stale-prepare.txt", TEST_BUNDLE_NAME,
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(),
                                        recreatedKey),
              E_OK);
    EXPECT_EQ(manager.taskKeyIndex_.size(), 1U);
    EXPECT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "stale-prepare.txt", TEST_SYNC_FOLDER_INDEX), E_OK);
    EXPECT_TRUE(manager.taskMap_.empty());
    EXPECT_TRUE(manager.taskKeyIndex_.empty());
}

/**
 * @tc.name: HydrationQueueIndex_StaleTerminal_001
 * @tc.desc: Handle a terminal task that still owns the identity index.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationQueueIndex_StaleTerminal_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "stale-terminal-has.txt", TEST_BUNDLE_NAME,
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto terminalTask = manager.taskMap_.at(key);
    terminalTask->state = PlaceholderTaskState::COMPLETED;
    EXPECT_FALSE(manager.HasOutstandingTask(TEST_SYNC_FOLDER, "stale-terminal-has.txt", TEST_SYNC_FOLDER_INDEX));
    EXPECT_TRUE(manager.taskKeyIndex_.empty());
    manager.EraseTaskLocked(terminalTask);
    EXPECT_TRUE(manager.taskMap_.empty());

    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "stale-terminal-cancel.txt", TEST_BUNDLE_NAME,
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    terminalTask = manager.taskMap_.at(key);
    terminalTask->state = PlaceholderTaskState::COMPLETED;
    EXPECT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "stale-terminal-cancel.txt", TEST_SYNC_FOLDER_INDEX),
              E_NO_HYDRATION_IN_PROGRESS);
    EXPECT_EQ(manager.taskKeyIndex_.size(), 1U);

    PlaceholderTaskManager::RequestKey recreatedKey;
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "stale-terminal-cancel.txt", TEST_BUNDLE_NAME,
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(),
                                        recreatedKey),
              E_OK);
    EXPECT_EQ(manager.taskMap_.size(), 2U);
    EXPECT_EQ(manager.taskKeyIndex_.size(), 1U);
    manager.EraseTaskLocked(terminalTask);
    EXPECT_EQ(manager.taskMap_.count(key), 0U);
    EXPECT_EQ(manager.taskMap_.count(recreatedKey), 1U);
    EXPECT_EQ(manager.taskKeyIndex_.size(), 1U);
    EXPECT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "stale-terminal-cancel.txt", TEST_SYNC_FOLDER_INDEX), E_OK);
    EXPECT_TRUE(manager.taskMap_.empty());
    EXPECT_TRUE(manager.taskKeyIndex_.empty());
}

/**
 * @tc.name: HydrationQueueIndex_CleanupPending_001
 * @tc.desc: A terminal task stays outstanding until EraseTaskLocked removes its identity index.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationQueueIndex_CleanupPending_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "cleanup-pending.txt", TEST_BUNDLE_NAME,
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto task = manager.taskMap_.at(key);
    task->cleanupPending = true;
    task->state = PlaceholderTaskState::COMPLETED;

    EXPECT_TRUE(manager.HasOutstandingTask(TEST_SYNC_FOLDER, "cleanup-pending.txt", TEST_SYNC_FOLDER_INDEX));
    EXPECT_EQ(manager.taskKeyIndex_.size(), 1U);
    PlaceholderTaskManager::RequestKey duplicateKey;
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "cleanup-pending.txt", TEST_BUNDLE_NAME,
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(),
                                        duplicateKey),
              E_HYDRATE_IN_PROGRESS);
    EXPECT_EQ(manager.taskKeyIndex_.size(), 1U);

    manager.EraseTaskLocked(task);
    EXPECT_FALSE(manager.HasOutstandingTask(TEST_SYNC_FOLDER, "cleanup-pending.txt", TEST_SYNC_FOLDER_INDEX));
    EXPECT_TRUE(manager.taskMap_.empty());
    EXPECT_TRUE(manager.taskKeyIndex_.empty());
}

/**
 * @tc.name: ActiveSlotBookkeeping_Mark_001
 * @tc.desc: Cover duplicate and missing-task active-slot marking paths.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, ActiveSlotBookkeeping_Mark_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "active-mark.txt", TEST_BUNDLE_NAME,
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    ActivateTask(manager, key);
    auto task = manager.taskMap_.at(key);
    manager.MarkTaskActiveLocked(task);
    ExpectActiveCount(manager, TEST_BUNDLE_NAME, -1, 1U);

    auto orphanTask = std::make_shared<PlaceholderTaskRecord>();
    orphanTask->reqKey = {0xEE};
    orphanTask->bundleName = TEST_BUNDLE_NAME;
    orphanTask->userId = -1;
    manager.MarkTaskActiveLocked(orphanTask);
    ExpectActiveCount(manager, TEST_BUNDLE_NAME, -1, 1U);

    EXPECT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "active-mark.txt", TEST_SYNC_FOLDER_INDEX), E_OK);
    ExpectActiveCount(manager, TEST_BUNDLE_NAME, -1, 0);
}

/**
 * @tc.name: ActiveSlotBookkeeping_Release_001
 * @tc.desc: Cover missing and zero active-slot count release paths.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, ActiveSlotBookkeeping_Release_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskRecord ghostTask;
    ghostTask.activeSlotHeld = true;
    ghostTask.bundleName = "com.example.ghost";
    ghostTask.userId = 123;
    {
        std::lock_guard<std::mutex> lock(manager.mapMutex_);
        manager.ReleaseActiveSlotLocked(ghostTask);
    }
    EXPECT_FALSE(ghostTask.activeSlotHeld);

    auto zeroKey = PlaceholderTaskManager::MakeAppUserKey("com.example.ghost-zero", 456);
    PlaceholderTaskRecord zeroTask;
    zeroTask.activeSlotHeld = true;
    zeroTask.bundleName = "com.example.ghost-zero";
    zeroTask.userId = 456;
    {
        std::lock_guard<std::mutex> lock(manager.mapMutex_);
        manager.activeByKey_[zeroKey] = 0;
        manager.ReleaseActiveSlotLocked(zeroTask);
        manager.activeByKey_.erase(zeroKey);
    }
    EXPECT_FALSE(zeroTask.activeSlotHeld);
    EXPECT_TRUE(manager.activeByKey_.empty());
}

/**
 * @tc.name: QueueSummaryLog_NullReason_001
 * @tc.desc: Cover the null reason branch of the mass-cancellation summary log.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, QueueSummaryLog_NullReason_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "queue-summary.txt", TEST_BUNDLE_NAME,
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    {
        std::lock_guard<std::mutex> lock(manager.mapMutex_);
        manager.LogQueueSummaryLocked(nullptr);
    }
    EXPECT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "queue-summary.txt", TEST_SYNC_FOLDER_INDEX), E_OK);
    EXPECT_TRUE(manager.taskMap_.empty());
    EXPECT_TRUE(manager.taskKeyIndex_.empty());
}

/**
 * @tc.name: SchedulerCapacity_001
 * @tc.desc: Enforce five active tasks per app-user key without a global active cap.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, SchedulerCapacity_001, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    const std::string eligibleBundle = "com.example.eligible";
    for (size_t index = 0; index < CLOUD_DISK_MAX_ACTIVE_TASKS_PER_APP; ++index) {
        PlaceholderTaskManager::RequestKey key;
        ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "active-app-" + std::to_string(index), TEST_BUNDLE_NAME,
                                            TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(),
                                            key),
                  E_OK);
        ActivateTask(manager, key);
    }

    PlaceholderTaskManager::RequestKey blockedAppKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "blocked-app.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_HIGH, OpenTaskFd(), blockedAppKey),
              E_OK);
    PlaceholderTaskManager::RequestKey eligibleAppKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "eligible-app.txt", eligibleBundle,
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_LOW, OpenTaskFd(),
                                        eligibleAppKey),
              E_OK);
    auto selected = manager.SelectNextTaskLocked();
    ASSERT_NE(selected, nullptr);
    EXPECT_EQ(selected->reqKey, eligibleAppKey);
    ActivateTask(manager, eligibleAppKey);

    for (size_t index = 1; index < CLOUD_DISK_MAX_ACTIVE_TASKS_PER_APP; ++index) {
        PlaceholderTaskManager::RequestKey key;
        ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "active-eligible-" + std::to_string(index),
                                            eligibleBundle, TEST_SYNC_FOLDER_INDEX,
                                            CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
                  E_OK);
        ActivateTask(manager, key);
    }
    {
        std::lock_guard<std::mutex> lock(manager.mapMutex_);
        EXPECT_EQ(manager.GetActiveCountLocked(eligibleBundle, -1), CLOUD_DISK_MAX_ACTIVE_TASKS_PER_APP);
    }

    PlaceholderTaskManager::RequestKey thirdAppKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "third-app.txt", "com.example.third",
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_LOW, OpenTaskFd(),
                                        thirdAppKey),
              E_OK);
    selected = manager.SelectNextTaskLocked();
    ASSERT_NE(selected, nullptr);
    EXPECT_EQ(selected->reqKey, thirdAppKey);

    PlaceholderTaskState blockedState = PlaceholderTaskState::CANCELLED;
    ASSERT_TRUE(manager.GetTaskState(blockedAppKey, blockedState));
    EXPECT_EQ(blockedState, PlaceholderTaskState::PENDING);
}

/**
 * @tc.name: SchedulerCapacity_MultiUser_001
 * @tc.desc: Isolate the five active slots by bundleName and userId.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, SchedulerCapacity_MultiUser_001, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderProgressContext firstUserContext;
    firstUserContext.userId = 100;
    for (size_t index = 0; index < CLOUD_DISK_MAX_ACTIVE_TASKS_PER_APP; ++index) {
        PlaceholderTaskManager::RequestKey key;
        ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "user-100-" + std::to_string(index), TEST_BUNDLE_NAME,
                                            TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(),
                                            key, firstUserContext),
                  E_OK);
        ActivateTask(manager, key);
    }

    PlaceholderTaskManager::RequestKey blockedKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "user-100-blocked.txt", TEST_BUNDLE_NAME,
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_HIGH, OpenTaskFd(),
                                        blockedKey, firstUserContext),
              E_OK);
    PlaceholderProgressContext secondUserContext;
    secondUserContext.userId = 200;
    PlaceholderTaskManager::RequestKey secondUserKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "user-200.txt", TEST_BUNDLE_NAME,
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_LOW, OpenTaskFd(),
                                        secondUserKey, secondUserContext),
              E_OK);

    auto selected = manager.SelectNextTaskLocked();
    ASSERT_NE(selected, nullptr);
    EXPECT_EQ(selected->reqKey, secondUserKey);
    auto firstUserKey = PlaceholderTaskManager::MakeAppUserKey(TEST_BUNDLE_NAME, 100);
    auto secondUserActiveKey = PlaceholderTaskManager::MakeAppUserKey(TEST_BUNDLE_NAME, 200);
    {
        std::lock_guard<std::mutex> lock(manager.mapMutex_);
        EXPECT_EQ(manager.activeByKey_.at(firstUserKey), CLOUD_DISK_MAX_ACTIVE_TASKS_PER_APP);
        EXPECT_EQ(manager.activeByKey_.count(secondUserActiveKey), 0U);
    }
}

/**
 * @tc.name: PendingCancelNotification_001
 * @tc.desc: A queued task may deliver CANCEL as the first callback before FETCH_DATA is dispatched.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, PendingCancelNotification_001, TestSize.Level1)
{
    auto callback = sptr(new HydrationCallbackStub());
    ASSERT_EQ(PlaceholderCallbackManager::GetInstance().RegisterCallbackTable(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                                                              callback),
              E_OK);
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    ASSERT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "file.txt", TEST_SYNC_FOLDER_INDEX), E_OK);
    std::unique_lock<std::mutex> lock(callback->mutex);
    ASSERT_TRUE(
        callback->cv.wait_for(lock, std::chrono::seconds(2), [&callback] { return !callback->callbackTypes.empty(); }));
    ASSERT_EQ(callback->callbackTypes.size(), 1U);
    EXPECT_EQ(callback->callbackTypes.front(), CloudDiskCallbackType::CANCEL_FETCH_DATA);
}

/**
 * @tc.name: PendingFileRemoved_001
 * @tc.desc: Cancel a queued task when its original file is deleted before activation.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, PendingFileRemoved_001, TestSize.Level1)
{
    auto callback = sptr(new HydrationCallbackStub());
    ASSERT_EQ(PlaceholderCallbackManager::GetInstance().RegisterCallbackTable(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                                                              callback),
              E_OK);
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    ExpectAdmissionMetadata(TEST_INODE_ID);
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key,
                                        {-1, "", file.GetPath(), TEST_TEMP_ROOT}),
              E_OK);
    ASSERT_TRUE(file.Remove());
    manager.StartScheduler();
    std::unique_lock<std::mutex> lock(callback->mutex);
    ASSERT_TRUE(
        callback->cv.wait_for(lock, std::chrono::seconds(2), [&callback] { return !callback->callbackTypes.empty(); }));
    ASSERT_EQ(callback->callbackTypes.size(), 1U);
    EXPECT_EQ(callback->callbackTypes.front(), CloudDiskCallbackType::CANCEL_FETCH_DATA);
    lock.unlock();
    PlaceholderTaskState state;
    EXPECT_FALSE(manager.GetTaskState(key, state));
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key)), E_CANCELLED);
}

/**
 * @tc.name: PendingFileReplaced_001
 * @tc.desc: Cancel a queued task when the path resolves to a different inode before activation.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, PendingFileReplaced_001, TestSize.Level1)
{
    auto callback = sptr(new HydrationCallbackStub());
    ASSERT_EQ(PlaceholderCallbackManager::GetInstance().RegisterCallbackTable(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                                                              callback),
              E_OK);
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    ExpectAdmissionAndActivationMetadata(TEST_INODE_ID, REPLACED_TEST_INODE_ID);
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key,
                                        {-1, "", file.GetPath(), TEST_TEMP_ROOT}),
              E_OK);
    ASSERT_TRUE(file.Replace());
    manager.StartScheduler();
    std::unique_lock<std::mutex> lock(callback->mutex);
    ASSERT_TRUE(
        callback->cv.wait_for(lock, std::chrono::seconds(2), [&callback] { return !callback->callbackTypes.empty(); }));
    ASSERT_EQ(callback->callbackTypes.size(), 1U);
    EXPECT_EQ(callback->callbackTypes.front(), CloudDiskCallbackType::CANCEL_FETCH_DATA);
}

/**
 * @tc.name: PendingFileResize_001
 * @tc.desc: Preserve file identity and use the latest logical size when a queued file changes size.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, PendingFileResize_001, TestSize.Level1)
{
    constexpr uint64_t LATEST_SIZE = 4096;
    auto callback = sptr(new HydrationCallbackStub());
    ASSERT_EQ(PlaceholderCallbackManager::GetInstance().RegisterCallbackTable(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                                                              callback),
              E_OK);
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    ExpectAdmissionAndActivationMetadata(TEST_INODE_ID, TEST_INODE_ID, 0, LATEST_SIZE);
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key,
                                        {100, "", file.GetPath(), TEST_TEMP_ROOT}),
              E_OK);
    manager.StartScheduler();
    {
        std::unique_lock<std::mutex> lock(callback->mutex);
        ASSERT_TRUE(callback->cv.wait_for(lock, std::chrono::seconds(2), [&callback] { return callback->called; }));
    }
    auto task = manager.FindTask(key);
    ASSERT_NE(task, nullptr);
    std::lock_guard<std::mutex> lock(task->mutex);
    EXPECT_EQ(task->state, PlaceholderTaskState::IN_PROGRESS);
    EXPECT_EQ(task->totalSize, LATEST_SIZE);
}

/**
 * @tc.name: CancelCancellationRecord_001
 * @tc.desc: Retain only the latest sixteen cancellation records per application and expire stale entries.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, CancelCancellationRecord_001, TestSize.Level1)
{
    constexpr size_t CANCELLATION_RECORD_COUNT = 17;
    auto &manager = PlaceholderTaskManager::GetInstance();
    std::vector<PlaceholderTaskManager::RequestKey> keys;
    for (size_t index = 0; index < CANCELLATION_RECORD_COUNT; ++index) {
        PlaceholderTaskManager::RequestKey key;
        std::string filePath = "file-" + std::to_string(index);
        ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, filePath, TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                            CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
                  E_OK);
        ASSERT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, filePath, TEST_SYNC_FOLDER_INDEX), E_OK);
        keys.push_back(key);
    }
    EXPECT_EQ(manager.cancellationRecordMap_.size(), 16U);
    auto firstRequest = MakeRequest(keys.front());
    firstRequest.filePath = "file-0";
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, firstRequest), E_NO_HYDRATION_IN_PROGRESS);
    auto lastRequest = MakeRequest(keys.back());
    lastRequest.filePath = "file-16";
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, lastRequest), E_CANCELLED);
    manager.cancellationRecordMap_.at(keys.back()).expiresAt = std::chrono::steady_clock::now();
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, lastRequest), E_NO_HYDRATION_IN_PROGRESS);
}

/**
 * @tc.name: CancelTask_001
 * @tc.desc: Verify cancellation removes the active task and Execute observes the cancellation record.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, CancelTask_001, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey reqKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), reqKey),
              E_OK);
    EXPECT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "file.txt", TEST_SYNC_FOLDER_INDEX), E_OK);
    PlaceholderTaskState state = PlaceholderTaskState::PENDING;
    EXPECT_FALSE(manager.GetTaskState(reqKey, state));
    EXPECT_FALSE(manager.HasOutstandingTask(TEST_SYNC_FOLDER, "file.txt", TEST_SYNC_FOLDER_INDEX));
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(reqKey)), E_CANCELLED);
    EXPECT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "missing.txt", TEST_SYNC_FOLDER_INDEX), E_NO_HYDRATION_IN_PROGRESS);
}

/**
 * @tc.name: Execute_001
 * @tc.desc: Verify missing, pending, and provider-identity Execute branches.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, Execute_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey missingKey{1, 2, 3};
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(missingKey)),
              E_NO_HYDRATION_IN_PROGRESS);

    PlaceholderTaskManager::RequestKey reqKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), reqKey),
              E_OK);
    EXPECT_EQ(manager.Execute("other.bundle", TEST_SYNC_FOLDER_INDEX, MakeRequest(reqKey)), E_CALLBACK_NOT_REGISTERED);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX + 1, MakeRequest(reqKey)),
              E_CALLBACK_NOT_REGISTERED);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(reqKey)), E_TRY_AGAIN);
}

/**
 * @tc.name: Execute_002
 * @tc.desc: Verify block writes advance 1 to 2 to 3, preserve sync bits, and close the task fd on completion.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, Execute_002, TestSize.Level1)
{
    auto observer = sptr(new TaskProgressRecorder());
    auto &progressManager = PlaceholderProgressManager::GetInstance();
    ASSERT_EQ(progressManager.Register({1, 10, 11}, 100, observer), E_OK);
    UniqueFd backingFd = OpenTaskFd();
    ASSERT_GE(backingFd.Get(), 0);
    UniqueFd outputFd(dup(backingFd.Get()));
    ASSERT_GE(outputFd.Get(), 0);
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey reqKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, std::move(outputFd), reqKey,
                                        {100, "", "", "", {1, 10, 11}}),
              E_OK);
    int32_t taskFd = ActivateTask(manager, reqKey, UniqueFd(dup(backingFd.Get())));
    ASSERT_GE(taskFd, 0);
    manager.taskMap_.at(reqKey)->totalSize = 6;

    uint8_t stateByte = MakeFileSyncState(PLACEHOLDER_STATE_UNHYDRATED, 1);
    ExpectStateTransitions(taskFd, stateByte);
    std::vector<uint8_t> firstBlock{1, 2, 3};
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(reqKey, 0, firstBlock, 6, false)),
              E_OK);
    // The partial flip belongs to activation; the synthetic activation above skipped it,
    // so a received chunk leaves the state untouched.
    EXPECT_EQ(GetPlaceholderStateFromFileSyncState(stateByte), PLACEHOLDER_STATE_UNHYDRATED);
    EXPECT_EQ(GetSyncStateFromFileSyncState(stateByte), 1);
    std::vector<uint8_t> finalBlock{4, 5, 6};
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(reqKey, 3, finalBlock, 6, true)),
              E_OK);
    EXPECT_EQ(GetPlaceholderStateFromFileSyncState(stateByte), PLACEHOLDER_STATE_FULLY_HYDRATED);
    EXPECT_EQ(GetSyncStateFromFileSyncState(stateByte), 1);
    PlaceholderTaskState taskState;
    EXPECT_FALSE(manager.GetTaskState(reqKey, taskState));
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(reqKey, 3, finalBlock, 6, true)),
              E_NO_HYDRATION_IN_PROGRESS);
    EXPECT_EQ(fcntl(taskFd, F_GETFD), -1);
    std::vector<uint8_t> actual(6);
    ASSERT_EQ(pread(backingFd.Get(), actual.data(), actual.size(), 0), static_cast<ssize_t>(actual.size()));
    EXPECT_EQ(actual, (std::vector<uint8_t>{1, 2, 3, 4, 5, 6}));
    progressManager.Drain();
    CheckCompletedProgress(observer);
}

/**
 * @tc.name: Execute_EmptyFile_001
 * @tc.desc: Complete an empty placeholder with the only valid zero-sized Execute shape.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, Execute_EmptyFile_001, TestSize.Level1)
{
    UniqueFd backingFd = OpenTaskFd();
    ASSERT_GE(backingFd.Get(), 0);
    UniqueFd outputFd(dup(backingFd.Get()));
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, std::move(outputFd), key),
              E_OK);
    int32_t taskFd = ActivateTask(manager, key, UniqueFd(dup(backingFd.Get())));
    ASSERT_GE(taskFd, 0);
    uint8_t stateByte = MakeFileSyncState(PLACEHOLDER_STATE_UNHYDRATED, 1);
    EXPECT_CALL(*mock_, fgetxattr(taskFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
        .Times(2)
        .WillRepeatedly(Invoke([&stateByte](int, const char *, void *value, size_t) {
            *static_cast<uint8_t *>(value) = stateByte;
            return static_cast<ssize_t>(sizeof(uint8_t));
        }));
    EXPECT_CALL(*mock_, fsetxattr(taskFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t), 0))
        .WillOnce(Invoke([&stateByte](int, const char *, const void *value, size_t, int) {
            stateByte = *static_cast<const uint8_t *>(value);
            return 0;
        }));
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key)), E_OK);
    EXPECT_EQ(GetPlaceholderStateFromFileSyncState(stateByte), PLACEHOLDER_STATE_FULLY_HYDRATED);
    PlaceholderTaskState state;
    EXPECT_FALSE(manager.GetTaskState(key, state));
}

/**
 * @tc.name: HydrationComplete_ShortDeliveryKeepsPartialState_001
 * @tc.desc: A short final delivery returns E_OK, completes the task, and keeps the partial state.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationComplete_ShortDeliveryKeepsPartialState_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key),
              E_OK);
    int32_t targetFd = ActivateTask(manager, key, file.DuplicateFd());
    ASSERT_GE(targetFd, 0);
    ASSERT_EQ(truncate(file.GetPath().c_str(), 8), 0);
    auto task = manager.taskMap_.at(key);
    task->totalSize = 8;
    task->logicalSize = 8;

    EXPECT_CALL(*mock_, fgetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t))).Times(0);
    EXPECT_CALL(*mock_, fsetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t), 0)).Times(0);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2, 3, 4}, 8, true)),
              E_OK);
    EXPECT_EQ(task->tempSize, 4u);
    PlaceholderTaskState taskState;
    EXPECT_FALSE(manager.GetTaskState(key, taskState));
}

/**
 * @tc.name: HydrationComplete_UnderReportedSizeKeepsPartialState_001
 * @tc.desc: A totalSize smaller than the target logical size returns E_OK and keeps the partial state.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationComplete_UnderReportedSizeKeepsPartialState_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key),
              E_OK);
    int32_t targetFd = ActivateTask(manager, key, file.DuplicateFd());
    ASSERT_GE(targetFd, 0);
    ASSERT_EQ(truncate(file.GetPath().c_str(), 12), 0);
    auto task = manager.taskMap_.at(key);
    task->totalSize = 8;
    task->logicalSize = 12;

    EXPECT_CALL(*mock_, fgetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t))).Times(0);
    EXPECT_CALL(*mock_, fsetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t), 0)).Times(0);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2, 3, 4, 5, 6, 7, 8}, 8, true)),
              E_OK);
    PlaceholderTaskState taskState;
    EXPECT_FALSE(manager.GetTaskState(key, taskState));
}

/**
 * @tc.name: HydrationComplete_MiddleHoleKeepsPartialState_001
 * @tc.desc: A final delivery with a middle hole returns E_OK and keeps the partial state.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationComplete_MiddleHoleKeepsPartialState_001, TestSize.Level2)
{
    constexpr uint64_t totalSize = 12;
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key),
              E_OK);
    int32_t targetFd = ActivateTask(manager, key, file.DuplicateFd());
    ASSERT_GE(targetFd, 0);
    ASSERT_EQ(truncate(file.GetPath().c_str(), totalSize), 0);
    auto task = manager.taskMap_.at(key);
    task->totalSize = totalSize;
    task->logicalSize = totalSize;

    EXPECT_CALL(*mock_, fgetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t))).Times(0);
    EXPECT_CALL(*mock_, fsetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t), 0)).Times(0);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2, 3, 4}, totalSize, false)),
              E_OK);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 8, {9, 10, 11, 12}, totalSize, true)),
              E_OK);
    EXPECT_EQ(task->tempSize, totalSize);
    PlaceholderTaskState taskState;
    EXPECT_FALSE(manager.GetTaskState(key, taskState));
}

/**
 * @tc.name: HydrationComplete_OutOfOrderOverlapMarksHydrated_001
 * @tc.desc: Out-of-order and overlapping chunks merge into one full range and publish the hydrated state.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationComplete_OutOfOrderOverlapMarksHydrated_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key),
              E_OK);
    int32_t targetFd = ActivateTask(manager, key, file.DuplicateFd());
    ASSERT_GE(targetFd, 0);
    ASSERT_EQ(truncate(file.GetPath().c_str(), 12), 0);
    auto task = manager.taskMap_.at(key);
    task->totalSize = 12;
    task->logicalSize = 12;

    uint8_t stateByte = MakeFileSyncState(PLACEHOLDER_STATE_UNHYDRATED, 0);
    ExpectStateTransitions(targetFd, stateByte);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 4, {5, 6, 7, 8}, 12, false)),
              E_OK);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2, 3, 4, 5, 6}, 12, false)),
              E_OK);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 6, {7, 8, 9, 10, 11, 12}, 12, true)),
              E_OK);
    EXPECT_EQ(GetPlaceholderStateFromFileSyncState(stateByte), PLACEHOLDER_STATE_FULLY_HYDRATED);
    PlaceholderTaskState taskState;
    EXPECT_FALSE(manager.GetTaskState(key, taskState));
}

/**
 * @tc.name: HydrationComplete_ContiguousDeliveryMarksHydrated_001
 * @tc.desc: A final delivery covering the whole logical size still publishes the fully hydrated state.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationComplete_ContiguousDeliveryMarksHydrated_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key),
              E_OK);
    int32_t targetFd = ActivateTask(manager, key, file.DuplicateFd());
    ASSERT_GE(targetFd, 0);
    ASSERT_EQ(truncate(file.GetPath().c_str(), 8), 0);
    auto task = manager.taskMap_.at(key);
    task->totalSize = 8;
    task->logicalSize = 8;

    uint8_t stateByte = MakeFileSyncState(PLACEHOLDER_STATE_UNHYDRATED, 0);
    ExpectStateTransitions(targetFd, stateByte);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2, 3, 4}, 8, false)),
              E_OK);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 4, {5, 6, 7, 8}, 8, true)),
              E_OK);
    EXPECT_EQ(GetPlaceholderStateFromFileSyncState(stateByte), PLACEHOLDER_STATE_FULLY_HYDRATED);
    PlaceholderTaskState taskState;
    EXPECT_FALSE(manager.GetTaskState(key, taskState));
}

/**
 * @tc.name: StartScheduler_001
 * @tc.desc: The scheduler dispatches FETCH_DATA asynchronously and retains the task after callback return.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, StartScheduler_001, TestSize.Level1)
{
    auto callback = sptr(new HydrationCallbackStub());
    ASSERT_EQ(PlaceholderCallbackManager::GetInstance().RegisterCallbackTable(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                                                              callback),
              E_OK);
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    ExpectAdmissionAndActivationMetadata(TEST_INODE_ID, TEST_INODE_ID);
    PlaceholderTaskManager::RequestKey reqKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_HIGH, file.DuplicateFd(), reqKey,
                                        {100, "", file.GetPath(), TEST_TEMP_ROOT}),
              E_OK);
    manager.StartScheduler();
    bool called = false;
    {
        std::unique_lock<std::mutex> lock(callback->mutex);
        called = callback->cv.wait_for(lock, std::chrono::seconds(2), [&callback] { return callback->called; });
    }
    EXPECT_TRUE(called);
    EXPECT_EQ(callback->priority, CLOUD_DISK_HYDRATE_PRIORITY_HIGH);
    auto task = manager.FindTask(reqKey);
    ASSERT_NE(task, nullptr);
    int32_t taskFd = -1;
    {
        std::lock_guard<std::mutex> lock(task->mutex);
        EXPECT_EQ(task->state, PlaceholderTaskState::IN_PROGRESS);
        taskFd = task->outputFd.Get();
    }
    PlaceholderTaskState state;
    ASSERT_TRUE(manager.GetTaskState(reqKey, state));
    EXPECT_EQ(state, PlaceholderTaskState::IN_PROGRESS);
    EXPECT_GE(fcntl(taskFd, F_GETFD), 0);
    manager.StopScheduler();
    EXPECT_FALSE(manager.GetTaskState(reqKey, state));
    EXPECT_EQ(fcntl(taskFd, F_GETFD), -1);
    EXPECT_TRUE(manager.taskMap_.empty());
    EXPECT_TRUE(manager.taskKeyIndex_.empty());
    EXPECT_TRUE(manager.activeByKey_.empty());
}

/**
 * @tc.name: IdleTimeout_001
 * @tc.desc: Expire an idle dispatched task and send exactly one CANCEL after FETCH.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, IdleTimeout_001, TestSize.Level1)
{
    auto callback = sptr(new HydrationCallbackStub());
    ASSERT_EQ(PlaceholderCallbackManager::GetInstance().RegisterCallbackTable(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                                                              callback),
              E_OK);
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    ExpectAdmissionAndActivationMetadata(TEST_INODE_ID, TEST_INODE_ID);
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key,
                                        {100, "", file.GetPath(), TEST_TEMP_ROOT}),
              E_OK);
    manager.StartScheduler();
    {
        std::unique_lock<std::mutex> lock(callback->mutex);
        ASSERT_TRUE(callback->cv.wait_for(lock, std::chrono::seconds(2),
                                          [&callback] { return !callback->callbackTypes.empty(); }));
    }
    auto task = manager.FindTask(key);
    ASSERT_NE(task, nullptr);
    {
        std::lock_guard<std::mutex> lock(task->mutex);
        ASSERT_EQ(task->state, PlaceholderTaskState::IN_PROGRESS);
    }
    {
        std::lock_guard<std::mutex> lock(manager.mapMutex_);
        manager.deadlineMap_[key] = std::chrono::steady_clock::now();
    }
    manager.deadlineCv_.notify_one();
    {
        std::unique_lock<std::mutex> lock(callback->mutex);
        ASSERT_TRUE(callback->cv.wait_for(lock, std::chrono::seconds(2),
                                          [&callback] { return callback->callbackTypes.size() >= 2; }));
        ASSERT_EQ(callback->callbackTypes.size(), 2U);
        EXPECT_EQ(callback->callbackTypes[0], CloudDiskCallbackType::FETCH_DATA);
        EXPECT_EQ(callback->callbackTypes[1], CloudDiskCallbackType::CANCEL_FETCH_DATA);
    }
    PlaceholderTaskState state;
    bool erased = false;
    for (int i = 0; i < 100; ++i) {
        if (!manager.GetTaskState(key, state)) {
            erased = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(erased);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key)), E_CANCELLED);
}

/**
 * @tc.name: Execute_003
 * @tc.desc: Reject oversized, overflowing, inconsistent and cross-file writes before touching the fd.
 * @tc.type: SECU
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, Execute_003, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto task = manager.taskMap_.at(key);
    ActivateTask(manager, key);
    Assistant::mockPwriteApi = true;
    EXPECT_CALL(*mock_, Pwrite(_, _, _, _)).Times(0);

    auto request = MakeRequest(key, 0, {1}, 1, false);
    request.size = 2;
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    request = MakeRequest(key, 0, std::vector<uint8_t>(MAX_EXECUTE_DATA_SIZE + 1), MAX_EXECUTE_DATA_SIZE + 1, false);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    request = MakeRequest(key, std::numeric_limits<uint64_t>::max(), {1}, std::numeric_limits<uint64_t>::max(), false);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    request = MakeRequest(key, 1, {1}, 1, false);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    request = MakeRequest(key, 2, {}, 1, true);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    constexpr uint64_t MAX_OFFSET = static_cast<uint64_t>(std::numeric_limits<off_t>::max());
    request = MakeRequest(key, MAX_OFFSET + 1, {}, MAX_OFFSET + 1, true);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    request = MakeRequest(key, MAX_OFFSET, {1}, MAX_OFFSET + 1, true);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    request = MakeRequest(key);
    request.filePath = "other.txt";
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    request = MakeRequest(key);
    request.syncFolder += "-other";
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    request = MakeRequest(key);
    request.callbackType = NON_EXECUTE_CALLBACK_TYPE;
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    request.callbackType = INVALID_CALLBACK_TYPE;
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
}

/**
 * @tc.name: Execute_013
 * @tc.desc: Reject inconsistent completion metadata and unavailable output descriptors.
 * @tc.type: SECU
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, Execute_013, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto task = manager.taskMap_.at(key);
    ActivateTask(manager, key);
    Assistant::mockPwriteApi = true;
    EXPECT_CALL(*mock_, Pwrite(_, _, _, _)).Times(0);

    auto request = MakeRequest(key, 0, {}, 1, false);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    request = MakeRequest(key, 0, {}, 1, true);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    request = MakeRequest(key, 0, {}, 0, false);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    task->totalSize = 1;
    task->totalSizeInitialized = true;
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key)), E_INVALID_ARG);
    task->totalSizeInitialized = false;
    task->cachedSize = std::numeric_limits<uint64_t>::max();
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key, 0, {1}, 1, false)),
              E_INVALID_ARG);
    task->cachedSize = 0;
    task->totalSize = 0;
    task->outputFd.Reset(-1);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key)), E_TRY_AGAIN);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key)), E_CANCELLED);
}

/**
 * @tc.name: Execute_128KiB_001
 * @tc.desc: Accept an Execute block exactly at the 128 KiB limit.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, Execute_128KiB_001, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto task = manager.taskMap_.at(key);
    ActivateTask(manager, key);
    task->totalSize = MAX_EXECUTE_DATA_SIZE + 1;
    Assistant::mockPwriteApi = true;
    std::vector<uint8_t> data(MAX_EXECUTE_DATA_SIZE, 1);
    EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, MAX_EXECUTE_DATA_SIZE, 0))
        .WillOnce(Return(static_cast<ssize_t>(MAX_EXECUTE_DATA_SIZE)));
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, data, MAX_EXECUTE_DATA_SIZE + 1, false)),
              E_OK);
    EXPECT_EQ(task->cachedSize, MAX_EXECUTE_DATA_SIZE);
}

/**
 * @tc.name: Execute_004
 * @tc.desc: A CANCEL Execute closes the task fd and removes the task without changing file data or state.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, Execute_004, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    UniqueFd fd = OpenTaskFd();
    ASSERT_GE(fd.Get(), 0);
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, std::move(fd), key),
              E_OK);
    int32_t taskFd = ActivateTask(manager, key);
    auto request = MakeRequest(key);
    request.callbackType = static_cast<int32_t>(CloudDiskCallbackType::CANCEL_FETCH_DATA);
    EXPECT_CALL(*mock_, fsetxattr(_, _, _, _, _)).Times(0);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_OK);
    EXPECT_EQ(fcntl(taskFd, F_GETFD), -1);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_CANCELLED);
}

/**
 * @tc.name: Execute_005
 * @tc.desc: Map write failures without erasing the task and retry EINTR and short writes at the correct offset.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, Execute_005, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto task = manager.taskMap_.at(key);
    ActivateTask(manager, key);
    task->totalSize = 3;
    Assistant::mockPwriteApi = true;
    auto request = MakeRequest(key, 0, {1, 2, 3}, 3, false);
    const std::vector<std::pair<int, int32_t>> errors = {
        {ENOSPC, E_NO_SPACE_LEFT}, {EDQUOT, E_NO_SPACE_LEFT}, {EFBIG, E_FILE_TOO_LARGE},
        {EACCES, E_ACCES},         {EPERM, E_PERM},           {EIO, E_TRY_AGAIN},
    };
    for (const auto &[error, expected] : errors) {
        EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 3, 0))
            .WillOnce(Invoke([writeError = error](int, const void *, size_t, off_t) {
                errno = writeError;
                return -1;
            }));
        EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), expected);
        EXPECT_EQ(task->state.load(), PlaceholderTaskState::IN_PROGRESS);
        EXPECT_GE(fcntl(task->outputFd.Get(), F_GETFD), 0);
        Mock::VerifyAndClearExpectations(mock_.get());
    }
    EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 3, 0)).WillOnce(Return(0));
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_TRY_AGAIN);
    Mock::VerifyAndClearExpectations(mock_.get());
    {
        InSequence sequence;
        EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 3, 0))
            .WillOnce(Invoke([](int, const void *, size_t, off_t) {
                errno = EINTR;
                return -1;
            }));
        EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 3, 0)).WillOnce(Return(1));
        EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 2, 1)).WillOnce(Return(2));
    }
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_OK);
    EXPECT_EQ(task->state.load(), PlaceholderTaskState::IN_PROGRESS);
}


/**
 * @tc.name: Execute_014
 * @tc.desc: Keep a completed task retryable when final placeholder-state persistence fails.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, Execute_014, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "complete-state.txt", TEST_BUNDLE_NAME,
                                        TEST_SYNC_FOLDER_INDEX, CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto task = manager.taskMap_.at(key);
    ActivateTask(manager, key);
    Assistant::mockPwriteApi = true;
    Assistant::mockSendfileApi = true;
    task->totalSize = 1;
    EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 1, 0)).WillOnce(Return(1));
    EXPECT_CALL(*mock_, Sendfile(task->outputFd.Get(), task->tempFd.Get(), _, 1)).WillOnce(Return(1));
    EXPECT_CALL(*mock_, fgetxattr(task->outputFd.Get(), _, _, _))
        .Times(2)
        .WillRepeatedly(Invoke(ReturnPlaceholderState(PLACEHOLDER_STATE_PARTIALLY_HYDRATED)));
    EXPECT_CALL(*mock_, fsetxattr(task->outputFd.Get(), _, _, _, _))
        .WillOnce(Invoke([](int, const char *, const void *, size_t, int) {
            errno = ERANGE;
            return -1;
        }));
    auto request = MakeRequest(key, 0, {1}, 1, true);
    request.filePath = "complete-state.txt";
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_TRY_AGAIN);
    PlaceholderTaskState state;
    ASSERT_TRUE(manager.GetTaskState(key, state));
    EXPECT_EQ(state, PlaceholderTaskState::IN_PROGRESS);
}


/**
 * @tc.name: Execute_007
 * @tc.desc: A blocked pwrite holds only its own task mutex and does not block another file's Execute.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, Execute_007, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey firstKey;
    PlaceholderTaskManager::RequestKey secondKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), firstKey),
              E_OK);
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "second.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), secondKey),
              E_OK);
    auto firstTask = manager.taskMap_.at(firstKey);
    auto secondTask = manager.taskMap_.at(secondKey);
    ActivateTask(manager, firstKey);
    ActivateTask(manager, secondKey);
    firstTask->totalSize = secondTask->totalSize = 1;
    Assistant::mockPwriteApi = true;
    ExecuteWriteBarrier barrier;
    EXPECT_CALL(*mock_, Pwrite(firstTask->tempFd.Get(), _, 1, 0))
        .WillOnce(Invoke([&barrier](int, const void *, size_t, off_t) {
            barrier.BlockFirstWrite();
            return 1;
        }));
    EXPECT_CALL(*mock_, Pwrite(secondTask->tempFd.Get(), _, 1, 0)).WillOnce(Return(1));
    int32_t firstResult = E_TRY_AGAIN;
    int32_t secondResult = E_TRY_AGAIN;
    auto firstRequest = MakeRequest(firstKey, 0, {1}, 1, false);
    auto secondRequest = MakeRequest(secondKey, 0, {2}, 1, false);
    secondRequest.filePath = "second.txt";
    std::thread first([&manager, &firstResult, &firstRequest] {
        firstResult = manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, firstRequest);
    });
    EXPECT_TRUE(barrier.WaitForFirstWrite());
    {
        std::unique_lock<std::mutex> taskLock(firstTask->mutex, std::try_to_lock);
        EXPECT_FALSE(taskLock.owns_lock());
    }
    std::thread second([&manager, &secondResult, &secondRequest, &barrier] {
        secondResult = manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, secondRequest);
        barrier.CompleteSecondWrite();
    });
    bool independent = barrier.ReleaseFirstAfterSecondWrite();
    first.join();
    second.join();
    EXPECT_TRUE(independent);
    EXPECT_EQ(firstResult, E_OK);
    EXPECT_EQ(secondResult, E_OK);
}
/**
 * @tc.name: ProgressMetadata_001
 * @tc.desc: Capture original path and metadata, then accumulate only successful Execute bytes.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, ProgressMetadata_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    EXPECT_CALL(*mock_, fstat(_, _)).WillOnce(Invoke([](int, struct stat *metadata) {
        metadata->st_size = 20;
        return 0;
    }));
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_HIGH, OpenTaskFd(), key,
                                        {100, "/original/file.txt"}),
              E_OK);
    auto task = manager.taskMap_.at(key);
    EXPECT_EQ(task->absolutePath, "/original/file.txt");
    EXPECT_EQ(task->userId, 100);
    EXPECT_EQ(task->totalSize, 20u);
    EXPECT_EQ(task->cachedSize, 0u);
    ActivateTask(manager, key);
    task->totalSize = 5;
    Assistant::mockPwriteApi = true;
    Assistant::mockSendfileApi = true;
    EXPECT_CALL(*mock_, Pwrite(_, _, 2, 0)).WillOnce(Return(2));
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key, 0, {1, 2}, 5, false)), E_OK);
    EXPECT_EQ(task->totalSize, 5u);
    EXPECT_EQ(task->cachedSize, 2u);
    EXPECT_CALL(*mock_, Pwrite(_, _, 3, 2)).WillOnce(Return(0));
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key, 2, {3, 4, 5}, 5, false)),
              E_TRY_AGAIN);
    EXPECT_EQ(task->cachedSize, 2u);
    EXPECT_CALL(*mock_, Pwrite(_, _, 3, 2)).WillOnce(Return(3));
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key, 2, {3, 4, 5}, 5, false)),
              E_OK);
    EXPECT_EQ(task->cachedSize, 5u);
    // A user cancellation keeps the 5 received bytes: they are copied back to the
    // target file before the task (and its temp file) is torn down.
    EXPECT_CALL(*mock_, Sendfile(task->outputFd.Get(), task->tempFd.Get(), _, 5)).WillOnce(Return(5));
    EXPECT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "file.txt", TEST_SYNC_FOLDER_INDEX), E_OK);
    EXPECT_TRUE(task->terminalProgressSent);
    EXPECT_FALSE(manager.HasOutstandingTask(TEST_SYNC_FOLDER, "file.txt", TEST_SYNC_FOLDER_INDEX));
}
/**
 * @tc.name: ProgressMetadata_002
 * @tc.desc: Reject metadata failures without retaining an fd, record or queued task.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, ProgressMetadata_002, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    EXPECT_CALL(*mock_, fstat(_, _)).WillOnce(Return(-1));
    UniqueFd fd = OpenTaskFd();
    int32_t rawFd = fd.Get();
    PlaceholderTaskManager::RequestKey key;
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, std::move(fd), key),
              E_TRY_AGAIN);
    EXPECT_TRUE(manager.taskMap_.empty());
    EXPECT_EQ(fcntl(rawFd, F_GETFD), -1);
}

/**
 * @tc.name: OpenValidatedHydrationFile_001
 * @tc.desc: Reject empty, missing, escaped, and non-child hydration paths before opening the target.
 * @tc.type: SECU
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, OpenValidatedHydrationFile_001, TestSize.Level2)
{
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    UniqueFd outputFd;
    HydrationFileMetadata metadata;
    EXPECT_EQ(OpenValidatedHydrationFile("", file.GetPath(), outputFd, metadata), E_INVALID_ARG);
    EXPECT_EQ(OpenValidatedHydrationFile(TEST_TEMP_ROOT, "", outputFd, metadata), E_INVALID_ARG);
    EXPECT_EQ(OpenValidatedHydrationFile(file.GetPath() + "-missing", file.GetPath(), outputFd, metadata),
              E_SYNC_FOLDER_PATH_NOT_EXIST);
    EXPECT_EQ(OpenValidatedHydrationFile("/dev", file.GetPath(), outputFd, metadata), E_INVALID_ARG);
    EXPECT_EQ(OpenValidatedHydrationFile(TEST_TEMP_ROOT, TEST_TEMP_ROOT, outputFd, metadata), E_INVALID_ARG);
}

/**
 * @tc.name: OpenValidatedHydrationFile_002
 * @tc.desc: Map root, component, metadata, directory, and invalid-size failures precisely.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, OpenValidatedHydrationFile_002, TestSize.Level2)
{
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    UniqueFd outputFd;
    HydrationFileMetadata metadata;

    UniqueFd rootFd = OpenTaskFd();
    UniqueFd nestedRootFd = OpenTaskFd();
    UniqueFd dataDirFd = OpenTaskFd();
    UniqueFd testDirFd = OpenTaskFd();
    ASSERT_GE(rootFd.Get(), 0);
    ASSERT_GE(nestedRootFd.Get(), 0);
    ASSERT_GE(dataDirFd.Get(), 0);
    ASSERT_GE(testDirFd.Get(), 0);

    Assistant::mockFdApi = true;
    Assistant::mockErrno = EACCES;
    EXPECT_CALL(*mock_, Open(_, _, _)).WillOnce(Return(-1));
    EXPECT_EQ(OpenValidatedHydrationFile(TEST_TEMP_ROOT, file.GetPath(), outputFd, metadata), E_ACCES);
    Mock::VerifyAndClearExpectations(mock_.get());

    int32_t rawRootFd = rootFd.Release();
    Assistant::mockErrno = ELOOP;
    EXPECT_CALL(*mock_, Open(_, _, _)).WillOnce(Return(rawRootFd));
    EXPECT_CALL(*mock_, OpenAt(rawRootFd, _, _, _)).WillOnce(Return(-1));
    EXPECT_EQ(OpenValidatedHydrationFile(TEST_TEMP_ROOT, file.GetPath(), outputFd, metadata), E_INVALID_ARG);
    Mock::VerifyAndClearExpectations(mock_.get());

    int32_t rawNestedRootFd = nestedRootFd.Release();
    int32_t rawDataDirFd = dataDirFd.Release();
    int32_t rawTestDirFd = testDirFd.Release();
    Assistant::mockErrno = EACCES;
    {
        InSequence sequence;
        EXPECT_CALL(*mock_, Open(StrEq("/"), _, _)).WillOnce(Return(rawNestedRootFd));
        EXPECT_CALL(*mock_, OpenAt(rawNestedRootFd, StrEq("data"), _, _)).WillOnce(Return(rawDataDirFd));
        EXPECT_CALL(*mock_, OpenAt(rawDataDirFd, StrEq("test"), _, _)).WillOnce(Return(rawTestDirFd));
        EXPECT_CALL(*mock_, OpenAt(rawTestDirFd, _, _, _)).WillOnce(Return(-1));
    }
    EXPECT_EQ(OpenValidatedHydrationFile("/", file.GetPath(), outputFd, metadata), E_ACCES);
    Mock::VerifyAndClearExpectations(mock_.get());
    Assistant::mockErrno = 0;
    Assistant::mockFdApi = false;
}

/**
 * @tc.name: OpenValidatedHydrationFile_004
 * @tc.desc: Map metadata, directory, and invalid-size failures precisely.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, OpenValidatedHydrationFile_004, TestSize.Level2)
{
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    UniqueFd outputFd;
    HydrationFileMetadata metadata;

    EXPECT_CALL(*mock_, fstat(_, _)).WillOnce(Invoke([](int, struct stat *) {
        errno = EACCES;
        return -1;
    }));
    EXPECT_EQ(OpenValidatedHydrationFile(TEST_TEMP_ROOT, file.GetPath(), outputFd, metadata), E_ACCES);
    Mock::VerifyAndClearExpectations(mock_.get());

    EXPECT_CALL(*mock_, fstat(_, _)).WillOnce(Invoke([](int, struct stat *fileStat) {
        fileStat->st_mode = S_IFDIR;
        fileStat->st_size = 0;
        return 0;
    }));
    EXPECT_EQ(OpenValidatedHydrationFile(TEST_TEMP_ROOT, file.GetPath(), outputFd, metadata), E_INVALID_ARG);
    Mock::VerifyAndClearExpectations(mock_.get());

    EXPECT_CALL(*mock_, fstat(_, _)).WillOnce(Invoke([](int, struct stat *fileStat) {
        fileStat->st_mode = S_IFREG;
        fileStat->st_size = -1;
        return 0;
    }));
    EXPECT_EQ(OpenValidatedHydrationFile(TEST_TEMP_ROOT, file.GetPath(), outputFd, metadata), E_INVALID_ARG);
}

/**
 * @tc.name: OpenValidatedHydrationFile_003
 * @tc.desc: Cover all persisted placeholder-state decisions and return successful metadata.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, OpenValidatedHydrationFile_003, TestSize.Level1)
{
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    UniqueFd outputFd;
    HydrationFileMetadata metadata;
    const auto runState = [this, &file, &outputFd, &metadata](uint8_t state, int32_t expected) {
        ExpectAdmissionMetadata(TEST_INODE_ID, 7);
        EXPECT_CALL(*mock_, fgetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
            .WillOnce(Invoke(ReturnPlaceholderState(state)));
        EXPECT_EQ(OpenValidatedHydrationFile(TEST_TEMP_ROOT, file.GetPath(), outputFd, metadata), expected);
        Mock::VerifyAndClearExpectations(mock_.get());
    };

    runState(PLACEHOLDER_STATE_NONE, E_NOT_A_PLACEHOLDER);
    runState(PLACEHOLDER_STATE_FULLY_HYDRATED, E_ALREADY_HYDRATED);
    runState(PLACEHOLDER_STATE_FULLY_HYDRATED + 1, E_INVALID_PLACEHOLDER_STATE);
    ExpectAdmissionMetadata(TEST_INODE_ID, 7);
    EXPECT_CALL(*mock_, fgetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
        .WillOnce(Invoke([](int, const char *, void *, size_t) {
            errno = ENOENT;
            return -1;
        }));
    EXPECT_EQ(OpenValidatedHydrationFile(TEST_TEMP_ROOT, file.GetPath(), outputFd, metadata), E_FILE_NOT_EXIST);
    Mock::VerifyAndClearExpectations(mock_.get());

    runState(PLACEHOLDER_STATE_PARTIALLY_HYDRATED, E_OK);
    EXPECT_GE(outputFd.Get(), 0);
    EXPECT_EQ(metadata.deviceId, TEST_DEVICE_ID);
    EXPECT_EQ(metadata.inodeId, TEST_INODE_ID);
    EXPECT_EQ(metadata.logicalSize, 7U);
}

/**
 * @tc.name: CreateHydrateTask_005
 * @tc.desc: Reject a priority below the public range and negative file metadata.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, CreateHydrateTask_005, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key{1};
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        static_cast<CloudDiskHydratePriority>(-1), OpenTaskFd(), key),
              E_INVALID_ARG);
    EXPECT_TRUE(key.empty());

    EXPECT_CALL(*mock_, fstat(_, _)).WillOnce(Invoke([](int, struct stat *metadata) {
        metadata->st_size = -1;
        return 0;
    }));
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_TRY_AGAIN);
    EXPECT_TRUE(key.empty());
    EXPECT_TRUE(manager.taskMap_.empty());
}

/**
 * @tc.name: GenerateRequestKeyLocked_001
 * @tc.desc: Wrap the request-key counter and skip keys held by live tasks or cancellation records.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, GenerateRequestKeyLocked_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    manager.nextReqKeyValue_ = std::numeric_limits<uint64_t>::max();
    auto maxKey = manager.GenerateRequestKeyLocked();
    EXPECT_EQ(maxKey.size(), sizeof(uint64_t));
    EXPECT_EQ(manager.nextReqKeyValue_, 1U);

    manager.nextReqKeyValue_ = 20;
    auto taskKey = manager.GenerateRequestKeyLocked();
    auto task = std::make_shared<PlaceholderTaskRecord>();
    task->reqKey = taskKey;
    manager.taskMap_[taskKey] = task;
    manager.nextReqKeyValue_ = 20;
    auto afterTaskKey = manager.GenerateRequestKeyLocked();
    EXPECT_NE(afterTaskKey, taskKey);
    manager.taskMap_.clear();

    manager.nextReqKeyValue_ = 30;
    auto cancellationRecordKey = manager.GenerateRequestKeyLocked();
    PlaceholderTaskManager::CancelledTaskRecord cancellationRecord;
    cancellationRecord.reqKey = cancellationRecordKey;
    cancellationRecord.expiresAt = std::chrono::steady_clock::now() + std::chrono::minutes(1);
    cancellationRecord.createSeq = 1;
    manager.cancellationRecordMap_[cancellationRecordKey] = cancellationRecord;
    manager.nextReqKeyValue_ = 30;
    auto afterCancellationRecordKey = manager.GenerateRequestKeyLocked();
    EXPECT_NE(afterCancellationRecordKey, cancellationRecordKey);
    manager.ClearCancellationRecords();
}

/**
 * @tc.name: StartScheduler_002
 * @tc.desc: Starting an already running scheduler is idempotent and creates no extra monitor task.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, StartScheduler_002, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    manager.StartScheduler();
    ASSERT_TRUE(manager.running_);
    ASSERT_EQ(manager.monitorHandles_.size(), 1U);
    manager.StartScheduler();
    EXPECT_TRUE(manager.running_);
    EXPECT_EQ(manager.monitorHandles_.size(), 1U);
    manager.StopScheduler();
    EXPECT_FALSE(manager.running_);
    EXPECT_FALSE(manager.stopping_);
    EXPECT_TRUE(manager.monitorHandles_.empty());
}

/**
 * @tc.name: ScheduleDispatch_001
 * @tc.desc: Cover stopped, stopping, already-scheduled, and empty dispatch-loop guards.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, ScheduleDispatch_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    manager.running_ = false;
    manager.stopping_ = false;
    manager.dispatchScheduled_ = false;
    manager.ScheduleDispatch();
    EXPECT_FALSE(manager.dispatchScheduled_);

    manager.running_ = true;
    manager.stopping_ = true;
    manager.ScheduleDispatch();
    EXPECT_FALSE(manager.dispatchScheduled_);

    manager.stopping_ = false;
    manager.dispatchScheduled_ = true;
    manager.ScheduleDispatch();
    EXPECT_TRUE(manager.dispatchScheduled_);
    manager.DispatchLoop();
    EXPECT_FALSE(manager.dispatchScheduled_);

    manager.running_ = true;
    manager.stopping_ = true;
    manager.dispatchScheduled_ = true;
    manager.DispatchLoop();
    EXPECT_FALSE(manager.dispatchScheduled_);
    manager.running_ = false;
    manager.stopping_ = false;
}

/**
 * @tc.name: ActivateAndDispatch_001
 * @tc.desc: Ignore terminal records and remove a validated task when no provider callback is registered.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, ActivateAndDispatch_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    auto terminalTask = std::make_shared<PlaceholderTaskRecord>();
    terminalTask->state = PlaceholderTaskState::COMPLETED;
    manager.ActivateAndDispatch(terminalTask);
    EXPECT_EQ(terminalTask->state, PlaceholderTaskState::COMPLETED);

    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    ExpectAdmissionAndActivationMetadata(TEST_INODE_ID, TEST_INODE_ID);
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key,
                                        {100, "", file.GetPath(), TEST_TEMP_ROOT}),
              E_OK);
    manager.ActivateAndDispatch(manager.taskMap_.at(key));
    PlaceholderTaskState state;
    EXPECT_FALSE(manager.GetTaskState(key, state));
    EXPECT_EQ(manager.cancellationRecordMap_.count(key), 0U);
}

/**
 * @tc.name: ActivateAndDispatch_002
 * @tc.desc: Cancel activation when the reopened file has moved to a different device.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, ActivateAndDispatch_002, TestSize.Level2)
{
    constexpr uint64_t REPLACED_DEVICE_ID = TEST_DEVICE_ID + 1;
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    EXPECT_CALL(*mock_, fstat(_, _))
        .Times(2)
        .WillOnce(Invoke([](int, struct stat *metadata) {
            metadata->st_dev = static_cast<dev_t>(TEST_DEVICE_ID);
            metadata->st_ino = static_cast<ino_t>(TEST_INODE_ID);
            metadata->st_mode = S_IFREG;
            metadata->st_size = 0;
            return 0;
        }))
        .WillOnce(Invoke([](int, struct stat *metadata) {
            metadata->st_dev = static_cast<dev_t>(REPLACED_DEVICE_ID);
            metadata->st_ino = static_cast<ino_t>(TEST_INODE_ID);
            metadata->st_mode = S_IFREG;
            metadata->st_size = 0;
            return 0;
        }));
    EXPECT_CALL(*mock_, fgetxattr(_, _, _, _)).WillOnce(Invoke([](int, const char *, void *value, size_t size) {
        *static_cast<uint8_t *>(value) = MakeFileSyncState(PLACEHOLDER_STATE_UNHYDRATED, 0);
        return static_cast<ssize_t>(size);
    }));
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key,
                                        {-1, "", file.GetPath(), TEST_TEMP_ROOT}),
              E_OK);
    manager.ActivateAndDispatch(manager.taskMap_.at(key));
    EXPECT_EQ(manager.taskMap_.count(key), 0U);
    EXPECT_EQ(manager.cancellationRecordMap_.count(key), 1U);
}

/**
 * @tc.name: ActivateAndDispatch_003
 * @tc.desc: Cancel activation when the partial-state transition at activation fails.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, ActivateAndDispatch_003, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    EXPECT_CALL(*mock_, fstat(_, _))
        .Times(2)
        .WillRepeatedly(Invoke([](int, struct stat *metadata) {
            metadata->st_dev = static_cast<dev_t>(TEST_DEVICE_ID);
            metadata->st_ino = static_cast<ino_t>(TEST_INODE_ID);
            metadata->st_mode = S_IFREG | S_IRUSR | S_IWUSR;
            metadata->st_size = 0;
            return 0;
        }));
    EXPECT_CALL(*mock_, fgetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
        .Times(3)
        .WillRepeatedly(Invoke([](int, const char *, void *value, size_t) {
            *static_cast<uint8_t *>(value) = MakeFileSyncState(PLACEHOLDER_STATE_UNHYDRATED, 0);
            return static_cast<ssize_t>(sizeof(uint8_t));
        }));
    EXPECT_CALL(*mock_, fsetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t), 0))
        .WillOnce(Invoke([](int, const char *, const void *, size_t, int) {
            errno = EINVAL;
            return -1;
        }));
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key,
                                        {100, "", file.GetPath(), TEST_TEMP_ROOT}),
              E_OK);
    manager.ActivateAndDispatch(manager.taskMap_.at(key));
    EXPECT_EQ(manager.taskMap_.count(key), 0U);
    EXPECT_EQ(manager.cancellationRecordMap_.count(key), 1U);
}

/**
 * @tc.name: CancelTasksBySyncFolder_001
 * @tc.desc: Cancel only matching queued and active tasks, then cancel all remaining tasks without cancellation records.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, CancelTasksBySyncFolder_001, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey pendingKey;
    PlaceholderTaskManager::RequestKey activeKey;
    PlaceholderTaskManager::RequestKey otherKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "pending.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), pendingKey),
              E_OK);
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "active.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), activeKey),
              E_OK);
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "other.txt", "com.example.other", TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), otherKey),
              E_OK);
    ActivateTask(manager, activeKey);

    manager.CancelTasksBySyncFolder(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, PlaceholderTaskCancelReason::UNREGISTER);
    EXPECT_EQ(manager.taskMap_.count(pendingKey), 0U);
    EXPECT_EQ(manager.taskMap_.count(activeKey), 0U);
    EXPECT_EQ(manager.taskMap_.count(otherKey), 1U);
    EXPECT_TRUE(manager.activeByKey_.empty());
    EXPECT_EQ(manager.taskKeyIndex_.size(), 1U);
    EXPECT_EQ(manager.cancellationRecordMap_.count(pendingKey), 0U);
    EXPECT_EQ(manager.cancellationRecordMap_.count(activeKey), 1U);

    manager.CancelAllTasks(PlaceholderTaskCancelReason::USER_SWITCH);
    EXPECT_TRUE(manager.taskMap_.empty());
    EXPECT_TRUE(manager.taskKeyIndex_.empty());
    EXPECT_TRUE(manager.activeByKey_.empty());
    EXPECT_EQ(manager.cancellationRecordMap_.count(otherKey), 0U);
    manager.ClearCancellationRecords();
    EXPECT_TRUE(manager.cancellationRecordMap_.empty());
    EXPECT_TRUE(manager.cancellationOrder_.empty());
}

/**
 * @tc.name: UnregisterSyncFolderStopsTasks_001
 * @tc.desc: Unregister teardown cancels folder tasks and rejects new hydration until released.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, UnregisterSyncFolderStopsTasks_001, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    PlaceholderTaskManager::RequestKey otherKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "stop.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "keep.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX + 1,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), otherKey),
              E_OK);

    manager.BeginUnregisterSyncFolder(TEST_SYNC_FOLDER_INDEX);
    EXPECT_EQ(manager.taskMap_.count(key), 0U);
    EXPECT_EQ(manager.taskMap_.count(otherKey), 1U);
    PlaceholderTaskManager::RequestKey rejectedKey;
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "rejected.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), rejectedKey),
              E_TRY_AGAIN);
    EXPECT_TRUE(rejectedKey.empty());

    manager.EndUnregisterSyncFolder(TEST_SYNC_FOLDER_INDEX);
    EXPECT_TRUE(manager.unregisteringFolders_.empty());
    PlaceholderTaskManager::RequestKey recreatedKey;
    EXPECT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "recreated.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), recreatedKey),
              E_OK);
    EXPECT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "recreated.txt", TEST_SYNC_FOLDER_INDEX), E_OK);
    EXPECT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "keep.txt", TEST_SYNC_FOLDER_INDEX + 1), E_OK);
}

/**
 * @tc.name: AddCancellationRecordLocked_001
 * @tc.desc: Wrap cancellation record sequence numbers, enforce the global cap, and purge stale order entries.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, AddCancellationRecordLocked_001, TestSize.Level2)
{
    constexpr size_t GLOBAL_CANCELLATION_RECORD_LIMIT = 32;
    auto &manager = PlaceholderTaskManager::GetInstance();
    manager.nextCancellationSeq_ = std::numeric_limits<uint64_t>::max();
    std::vector<PlaceholderTaskManager::RequestKey> keys;
    for (size_t index = 0; index <= GLOBAL_CANCELLATION_RECORD_LIMIT; ++index) {
        auto task = std::make_shared<PlaceholderTaskRecord>();
        task->reqKey = {static_cast<uint8_t>(index)};
        task->syncFolder = TEST_SYNC_FOLDER;
        task->filePath = "file-" + std::to_string(index);
        task->bundleName = "com.example.cancellation_record." + std::to_string(index);
        task->syncFolderIndex = TEST_SYNC_FOLDER_INDEX;
        manager.AddCancellationRecordLocked(task);
        keys.push_back(task->reqKey);
    }
    EXPECT_EQ(manager.cancellationRecordMap_.size(), GLOBAL_CANCELLATION_RECORD_LIMIT);
    EXPECT_EQ(manager.cancellationRecordMap_.count(keys.front()), 0U);
    EXPECT_EQ(manager.cancellationRecordMap_.count(keys.back()), 1U);
    EXPECT_EQ(manager.nextCancellationSeq_, GLOBAL_CANCELLATION_RECORD_LIMIT + 2U);

    const auto &latest = manager.cancellationRecordMap_.at(keys.back());
    manager.cancellationOrder_.emplace_front(keys.back(), latest.createSeq + 1);
    manager.PurgeExpiredCancellationRecordsLocked();
    EXPECT_EQ(manager.cancellationOrder_.size(), GLOBAL_CANCELLATION_RECORD_LIMIT);
    EXPECT_EQ(manager.cancellationRecordMap_.count(keys.back()), 1U);
    manager.ClearCancellationRecords();
}

/**
 * @tc.name: Execute_010
 * @tc.desc: Cover request-key, terminal-state, and cancellation-record validation branches.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, Execute_010, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest({})), E_INVALID_ARG);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(std::vector<uint8_t>(MAX_CALLBACK_REQUEST_KEY_SIZE + 1, 1))),
              E_INVALID_ARG);

    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto task = manager.taskMap_.at(key);
    task->state = PlaceholderTaskState::COMPLETED;
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key)), E_NO_HYDRATION_IN_PROGRESS);
    task->state = PlaceholderTaskState::CANCELLED;
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key)), E_CANCELLED);
    task->state = PlaceholderTaskState::PENDING;
    ASSERT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "file.txt", TEST_SYNC_FOLDER_INDEX), E_OK);

    auto request = MakeRequest(key);
    EXPECT_EQ(manager.Execute("com.example.other", TEST_SYNC_FOLDER_INDEX, request), E_CALLBACK_NOT_REGISTERED);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX + 1, request), E_CALLBACK_NOT_REGISTERED);
    request.syncFolder += "-other";
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    request = MakeRequest(key);
    request.filePath = "other.txt";
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_INVALID_ARG);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key)), E_CANCELLED);
    manager.ClearCancellationRecords();
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key)), E_NO_HYDRATION_IN_PROGRESS);
}

/**
 * @tc.name: EnsurePartialStateLocked_001
 * @tc.desc: Cover cached, persisted, corrupt, missing, stale-old-state, and success branches.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, EnsurePartialStateLocked_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto task = manager.taskMap_.at(key);
    ActivateTask(manager, key);
    EXPECT_CALL(*mock_, fgetxattr(task->outputFd.Get(), _, _, _))
        .WillOnce(Invoke(ReturnPlaceholderState(PLACEHOLDER_STATE_PARTIALLY_HYDRATED)));
    EXPECT_CALL(*mock_, fsetxattr(_, _, _, _, _)).Times(0);
    EXPECT_EQ(manager.EnsurePartialStateLocked(task), E_OK);
    Mock::VerifyAndClearExpectations(mock_.get());

    EXPECT_CALL(*mock_, fgetxattr(task->outputFd.Get(), _, _, _))
        .WillOnce(Invoke(ReturnPlaceholderState(PLACEHOLDER_STATE_FULLY_HYDRATED)));
    EXPECT_EQ(manager.EnsurePartialStateLocked(task), E_TRY_AGAIN);
    Mock::VerifyAndClearExpectations(mock_.get());

    EXPECT_CALL(*mock_, fgetxattr(task->outputFd.Get(), _, _, _))
        .WillOnce(Invoke(ReturnPlaceholderState(PLACEHOLDER_STATE_FULLY_HYDRATED + 1)));
    EXPECT_EQ(manager.EnsurePartialStateLocked(task), E_INVALID_PLACEHOLDER_STATE);
    Mock::VerifyAndClearExpectations(mock_.get());

    EXPECT_CALL(*mock_, fgetxattr(task->outputFd.Get(), _, _, _))
        .WillOnce(Invoke([](int, const char *, void *, size_t) {
            errno = ENOENT;
            return -1;
        }));
    EXPECT_EQ(manager.EnsurePartialStateLocked(task), E_FILE_NOT_EXIST);
    Mock::VerifyAndClearExpectations(mock_.get());
}

/**
 * @tc.name: EnsurePartialStateLocked_002
 * @tc.desc: Cover stale old-state validation and successful partial-state persistence.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, EnsurePartialStateLocked_002, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto task = manager.taskMap_.at(key);
    ActivateTask(manager, key);

    EXPECT_CALL(*mock_, fgetxattr(task->outputFd.Get(), _, _, _))
        .Times(2)
        .WillOnce(Invoke(ReturnPlaceholderState(PLACEHOLDER_STATE_UNHYDRATED)))
        .WillOnce(Invoke(ReturnPlaceholderState(PLACEHOLDER_STATE_FULLY_HYDRATED + 1)));
    EXPECT_CALL(*mock_, fsetxattr(task->outputFd.Get(), _, _, _, _)).WillOnce(Return(0));
    EXPECT_EQ(manager.EnsurePartialStateLocked(task), E_INVALID_PLACEHOLDER_STATE);
    Mock::VerifyAndClearExpectations(mock_.get());

    EXPECT_CALL(*mock_, fgetxattr(task->outputFd.Get(), _, _, _))
        .Times(2)
        .WillOnce(Invoke(ReturnPlaceholderState(PLACEHOLDER_STATE_UNHYDRATED)))
        .WillOnce(Invoke(ReturnPlaceholderState(PLACEHOLDER_STATE_NONE)));
    EXPECT_CALL(*mock_, fsetxattr(task->outputFd.Get(), _, _, _, _)).WillOnce(Return(0));
    EXPECT_EQ(manager.EnsurePartialStateLocked(task), E_TRY_AGAIN);
    Mock::VerifyAndClearExpectations(mock_.get());

    EXPECT_CALL(*mock_, fgetxattr(task->outputFd.Get(), _, _, _))
        .Times(2)
        .WillOnce(Invoke(ReturnPlaceholderState(PLACEHOLDER_STATE_UNHYDRATED)))
        .WillOnce(Invoke(ReturnPlaceholderState(PLACEHOLDER_STATE_PARTIALLY_HYDRATED)));
    EXPECT_CALL(*mock_, fsetxattr(task->outputFd.Get(), _, _, _, _)).WillOnce(Return(0));
    EXPECT_EQ(manager.EnsurePartialStateLocked(task), E_OK);
}

/**
 * @tc.name: SetCompleteStateLocked_001
 * @tc.desc: Reject read failures and invalid current or old states before accepting a valid transition.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, SetCompleteStateLocked_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto task = manager.taskMap_.at(key);
    ActivateTask(manager, key);
    const auto returnState = [](uint8_t state) {
        return [state](int, const char *, void *value, size_t size) {
            *static_cast<uint8_t *>(value) = MakeFileSyncState(state, 0);
            return static_cast<ssize_t>(size);
        };
    };

    EXPECT_CALL(*mock_, fgetxattr(task->outputFd.Get(), _, _, _))
        .WillOnce(Invoke([](int, const char *, void *, size_t) {
            errno = EINVAL;
            return -1;
        }));
    EXPECT_EQ(manager.SetCompleteStateLocked(task), E_INVALID_PLACEHOLDER_STATE);
    Mock::VerifyAndClearExpectations(mock_.get());

    EXPECT_CALL(*mock_, fgetxattr(task->outputFd.Get(), _, _, _))
        .WillOnce(Invoke(returnState(PLACEHOLDER_STATE_FULLY_HYDRATED + 1)));
    EXPECT_EQ(manager.SetCompleteStateLocked(task), E_INVALID_PLACEHOLDER_STATE);
    Mock::VerifyAndClearExpectations(mock_.get());

    EXPECT_CALL(*mock_, fgetxattr(task->outputFd.Get(), _, _, _))
        .WillOnce(Invoke(returnState(PLACEHOLDER_STATE_FULLY_HYDRATED)));
    EXPECT_EQ(manager.SetCompleteStateLocked(task), E_TRY_AGAIN);
    Mock::VerifyAndClearExpectations(mock_.get());

    EXPECT_CALL(*mock_, fgetxattr(task->outputFd.Get(), _, _, _))
        .Times(2)
        .WillOnce(Invoke(returnState(PLACEHOLDER_STATE_UNHYDRATED)))
        .WillOnce(Invoke(returnState(PLACEHOLDER_STATE_FULLY_HYDRATED + 1)));
    EXPECT_CALL(*mock_, fsetxattr(task->outputFd.Get(), _, _, _, _)).WillOnce(Return(0));
    EXPECT_EQ(manager.SetCompleteStateLocked(task), E_INVALID_PLACEHOLDER_STATE);
    Mock::VerifyAndClearExpectations(mock_.get());

    EXPECT_CALL(*mock_, fgetxattr(task->outputFd.Get(), _, _, _))
        .Times(2)
        .WillOnce(Invoke(returnState(PLACEHOLDER_STATE_UNHYDRATED)))
        .WillOnce(Invoke(returnState(PLACEHOLDER_STATE_NONE)));
    EXPECT_CALL(*mock_, fsetxattr(task->outputFd.Get(), _, _, _, _)).WillOnce(Return(0));
    EXPECT_EQ(manager.SetCompleteStateLocked(task), E_TRY_AGAIN);
    Mock::VerifyAndClearExpectations(mock_.get());

    EXPECT_CALL(*mock_, fgetxattr(task->outputFd.Get(), _, _, _))
        .Times(2)
        .WillOnce(Invoke(returnState(PLACEHOLDER_STATE_PARTIALLY_HYDRATED)))
        .WillOnce(Invoke(returnState(PLACEHOLDER_STATE_PARTIALLY_HYDRATED)));
    EXPECT_CALL(*mock_, fsetxattr(task->outputFd.Get(), _, _, _, _)).WillOnce(Return(0));
    EXPECT_EQ(manager.SetCompleteStateLocked(task), E_OK);
}

/**
 * @tc.name: Execute_011
 * @tc.desc: Keep a task retryable when flushing the completed hydration data fails.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, Execute_011, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto task = manager.taskMap_.at(key);
    int32_t taskFd = ActivateTask(manager, key);
    ASSERT_GE(taskFd, 0);
    task->totalSize = 1;
    Assistant::mockPwriteApi = true;
    Assistant::mockFsyncApi = true;
    Assistant::mockSendfileApi = true;
    EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 1, 0)).WillOnce(Return(1));
    EXPECT_CALL(*mock_, Sendfile(taskFd, task->tempFd.Get(), _, 1)).WillOnce(Return(1));
    EXPECT_CALL(*mock_, Fsync(taskFd)).WillOnce(Invoke([](int) {
        errno = EIO;
        return -1;
    }));
    EXPECT_CALL(*mock_, fgetxattr(_, _, _, _)).Times(0);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key, 0, {1}, 1, true)),
              E_TRY_AGAIN);
    EXPECT_EQ(task->state, PlaceholderTaskState::IN_PROGRESS);
}

/**
 * @tc.name: CancelTaskRecordLocked_001
 * @tc.desc: Avoid duplicate cancellation callbacks and ignore records that are already terminal.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, CancelTaskRecordLocked_001, TestSize.Level2)
{
    auto callback = sptr(new HydrationCallbackStub());
    ASSERT_EQ(PlaceholderCallbackManager::GetInstance().RegisterCallbackTable(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                                                              callback),
              E_OK);
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey cancelledKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), cancelledKey),
              E_OK);
    auto cancelledTask = manager.taskMap_.at(cancelledKey);
    cancelledTask->cancelCallbackAttempted = true;
    manager.CancelTaskRecordLocked(cancelledTask, PlaceholderTaskCancelReason::USER_REQUEST);
    EXPECT_EQ(manager.taskMap_.count(cancelledKey), 0U);
    EXPECT_EQ(manager.cancellationRecordMap_.count(cancelledKey), 1U);
    EXPECT_TRUE(callback->callbackTypes.empty());

    PlaceholderTaskManager::RequestKey terminalKey;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "terminal.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), terminalKey),
              E_OK);
    auto terminalTask = manager.taskMap_.at(terminalKey);
    terminalTask->state = PlaceholderTaskState::COMPLETED;
    manager.CancelTaskRecordLocked(terminalTask, PlaceholderTaskCancelReason::USER_REQUEST);
    EXPECT_EQ(manager.taskMap_.count(terminalKey), 1U);
    terminalTask->state = PlaceholderTaskState::PENDING;
    manager.CancelTaskRecordLocked(terminalTask, PlaceholderTaskCancelReason::SERVICE_STOP);
    EXPECT_EQ(manager.taskMap_.count(terminalKey), 0U);
    EXPECT_EQ(manager.cancellationRecordMap_.count(terminalKey), 0U);
    manager.ClearCancellationRecords();
}

/**
 * @tc.name: RefreshDeadlineLocked_001
 * @tc.desc: Refresh only live records and erase deadlines without removing a different record instance.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, RefreshDeadlineLocked_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto task = manager.taskMap_.at(key);
    manager.RefreshDeadlineLocked(task);
    ASSERT_EQ(manager.deadlineMap_.count(key), 1U);

    auto otherRecord = std::make_shared<PlaceholderTaskRecord>();
    otherRecord->reqKey = key;
    otherRecord->outputFd = OpenTaskFd();
    int32_t otherFd = otherRecord->outputFd.Get();
    ASSERT_GE(otherFd, 0);
    manager.EraseTaskLocked(otherRecord);
    EXPECT_EQ(fcntl(otherFd, F_GETFD), -1);
    EXPECT_EQ(manager.taskMap_.count(key), 1U);
    EXPECT_EQ(manager.deadlineMap_.count(key), 0U);

    manager.RefreshDeadlineLocked(task);
    EXPECT_EQ(manager.deadlineMap_.count(key), 1U);
    manager.EraseTaskLocked(task);
    EXPECT_EQ(manager.taskMap_.count(key), 0U);
    EXPECT_EQ(manager.deadlineMap_.count(key), 0U);
    manager.RefreshDeadlineLocked(task);
    EXPECT_EQ(manager.deadlineMap_.count(key), 0U);
}

/**
 * @tc.name: NotifyProgressLocked_001
 * @tc.desc: Publish one terminal progress event and suppress repeated terminal notifications.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, NotifyProgressLocked_001, TestSize.Level1)
{
    auto observer = sptr(new TaskProgressRecorder());
    auto &progressManager = PlaceholderProgressManager::GetInstance();
    ASSERT_EQ(progressManager.Register({2, 20, 22}, 100, observer), E_OK);
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key,
                                        {100, "/original/file.txt", "", "", {2, 20, 22}}),
              E_OK);
    auto task = manager.taskMap_.at(key);
    task->state = PlaceholderTaskState::CANCELLED;
    manager.NotifyProgressLocked(task);
    manager.NotifyProgressLocked(task);
    progressManager.Drain();
    ASSERT_EQ(observer->events.size(), 2U);
    EXPECT_EQ(observer->events.front().state, static_cast<int32_t>(HydrateProgressState::PENDING));
    EXPECT_EQ(observer->events.back().state, static_cast<int32_t>(HydrateProgressState::CANCELLED));
    EXPECT_EQ(observer->events.back().filePath, "/original/file.txt");
    EXPECT_TRUE(task->terminalProgressSent);
    manager.EraseTaskLocked(task);
}

/**
 * @tc.name: DispatchLoop_001
 * @tc.desc: Releasing an application active slot dispatches its next queued task after the cancellation callback.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, DispatchLoop_001, TestSize.Level1)
{
    auto callback = sptr(new HydrationCallbackStub());
    ASSERT_EQ(PlaceholderCallbackManager::GetInstance().RegisterCallbackTable(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                                                              callback),
              E_OK);
    auto &manager = PlaceholderTaskManager::GetInstance();
    std::vector<std::string> activePaths;
    activePaths.reserve(CLOUD_DISK_MAX_ACTIVE_TASKS_PER_APP);
    for (size_t index = 0; index < CLOUD_DISK_MAX_ACTIVE_TASKS_PER_APP; ++index) {
        std::string filePath = "active-" + std::to_string(index) + ".txt";
        PlaceholderTaskManager::RequestKey key;
        ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, filePath, TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                            CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key,
                                            {100, "", "", ""}),
                  E_OK);
        ActivateTask(manager, key);
        activePaths.push_back(std::move(filePath));
    }

    HydrationTestFile pendingFile;
    ASSERT_TRUE(pendingFile.IsValid());
    ExpectAdmissionAndActivationMetadata(TEST_INODE_ID, TEST_INODE_ID);
    PlaceholderTaskManager::RequestKey pendingKey;
    const std::string pendingPath = "pending.txt";
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, pendingPath, TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_HIGH, pendingFile.DuplicateFd(), pendingKey,
                                        {100, "", pendingFile.GetPath(), TEST_TEMP_ROOT}),
              E_OK);
    manager.StartScheduler();
    auto initialBarrier = manager.dispatchQueue_.submit_h([] {});
    manager.dispatchQueue_.wait(initialBarrier);
    PlaceholderTaskState state = PlaceholderTaskState::CANCELLED;
    ASSERT_TRUE(manager.GetTaskState(pendingKey, state));
    ASSERT_EQ(state, PlaceholderTaskState::PENDING);

    ASSERT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, activePaths.front(), TEST_SYNC_FOLDER_INDEX), E_OK);
    auto dispatchBarrier = manager.dispatchQueue_.submit_h([] {});
    manager.dispatchQueue_.wait(dispatchBarrier);
    {
        std::lock_guard<std::mutex> lock(callback->mutex);
        ASSERT_GE(callback->callbackTypes.size(), 2U);
        EXPECT_EQ(callback->callbackTypes[0], CloudDiskCallbackType::CANCEL_FETCH_DATA);
        EXPECT_EQ(callback->callbackTypes[1], CloudDiskCallbackType::FETCH_DATA);
    }
    ASSERT_TRUE(manager.GetTaskState(pendingKey, state));
    EXPECT_EQ(state, PlaceholderTaskState::IN_PROGRESS);
}

/**
 * @tc.name: Execute_012
 * @tc.desc: A successful non-final Execute refreshes an already expired sliding deadline.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, Execute_012, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    auto task = manager.taskMap_.at(key);
    ActivateTask(manager, key);
    task->totalSize = 2;
    const auto expiredDeadline = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    {
        std::lock_guard<std::mutex> lock(manager.mapMutex_);
        manager.deadlineMap_[key] = expiredDeadline;
    }

    Assistant::mockPwriteApi = true;
    EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 1, 0)).WillOnce(Return(1));
    const auto beforeExecute = std::chrono::steady_clock::now();
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(key, 0, {1}, 2, false)), E_OK);

    std::lock_guard<std::mutex> lock(manager.mapMutex_);
    ASSERT_EQ(manager.deadlineMap_.count(key), 1U);
    EXPECT_GT(manager.deadlineMap_.at(key), expiredDeadline);
    EXPECT_GT(manager.deadlineMap_.at(key), beforeExecute);
    EXPECT_EQ(task->state, PlaceholderTaskState::IN_PROGRESS);
}
/**
 * @tc.name: AccessorOwner_001
 * @tc.desc: Explicit cancellation follows its caller without changing task ownership or misrouting a reused path.
 * @tc.type: SECU
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, AccessorOwner_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    auto &progress = PlaceholderProgressManager::GetInstance();
    auto first = sptr(new TaskProgressRecorder());
    auto second = sptr(new TaskProgressRecorder());
    auto nextOwner = sptr(new TaskProgressRecorder());
    ASSERT_EQ(progress.Register({1, 10, 11}, 100, first), E_OK);
    ASSERT_EQ(progress.Register({1, 10, 22}, 100, second), E_OK);
    ASSERT_EQ(progress.Register({1, 10, 33}, 100, nextOwner), E_OK);
    PlaceholderTaskManager::RequestKey firstKey;
    PlaceholderTaskManager::RequestKey secondKey;
    ASSERT_EQ(CreateOwnedTask(manager, 11, firstKey), E_OK);
    auto oldTask = manager.taskMap_.at(firstKey);
    EXPECT_EQ(CreateOwnedTask(manager, 22, secondKey), E_HYDRATE_IN_PROGRESS);
    ActivateTask(manager, firstKey);
    ASSERT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "file.txt", TEST_SYNC_FOLDER_INDEX, {1, 10, 22}), E_OK);
    ASSERT_EQ(CreateOwnedTask(manager, 33, secondKey), E_OK);
    EXPECT_NE(firstKey, secondKey);
    EXPECT_EQ(oldTask->owner.accessorId, 11U);
    EXPECT_EQ(manager.taskMap_.at(secondKey)->owner.accessorId, 33U);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, MakeRequest(firstKey)), E_CANCELLED);
    progress.Drain();
    ASSERT_EQ(first->events.size(), 2U);
    EXPECT_EQ(first->events.back().state, static_cast<int32_t>(HydrateProgressState::IN_PROGRESS));
    EXPECT_EQ(first->events.back().accessorId, 11U);
    ASSERT_EQ(second->events.size(), 1U);
    EXPECT_EQ(second->events.front().state, static_cast<int32_t>(HydrateProgressState::CANCELLED));
    EXPECT_EQ(second->events.front().accessorId, 22U);
    ASSERT_EQ(nextOwner->events.size(), 1U);
    EXPECT_EQ(nextOwner->events.front().state, static_cast<int32_t>(HydrateProgressState::PENDING));
    EXPECT_EQ(nextOwner->events.front().accessorId, 33U);
}

/**
 * @tc.name: AccessorOwner_002
 * @tc.desc: Tasks survive off/on and automatic cancellation still notifies the original owner.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, AccessorOwner_002, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    auto &progress = PlaceholderProgressManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(CreateOwnedTask(manager, 11, key), E_OK);
    progress.Drain();
    auto first = sptr(new TaskProgressRecorder());
    auto replacement = sptr(new TaskProgressRecorder());
    auto other = sptr(new TaskProgressRecorder());
    ASSERT_EQ(progress.Register({1, 10, 11}, 100, first), E_OK);
    ASSERT_EQ(progress.Register({1, 10, 22}, 100, other), E_OK);
    ActivateTask(manager, key);
    progress.Drain();
    ASSERT_EQ(first->events.size(), 1U);
    EXPECT_EQ(first->events.front().state, static_cast<int32_t>(HydrateProgressState::IN_PROGRESS));
    ASSERT_EQ(progress.Unregister({1, 10, 11}), E_OK);
    EXPECT_TRUE(manager.HasOutstandingTask(TEST_SYNC_FOLDER, "file.txt", TEST_SYNC_FOLDER_INDEX));
    ASSERT_EQ(progress.Register({1, 10, 11}, 100, replacement), E_OK);
    manager.CancelAllTasks(PlaceholderTaskCancelReason::TIMEOUT);
    progress.Drain();
    EXPECT_EQ(first->events.size(), 1U);
    ASSERT_EQ(replacement->events.size(), 1U);
    EXPECT_EQ(replacement->events.front().state, static_cast<int32_t>(HydrateProgressState::CANCELLED));
    EXPECT_EQ(replacement->events.front().accessorId, 11U);
    EXPECT_TRUE(other->events.empty());
}

/**
 * @tc.name: AccessorOwner_003
 * @tc.desc: A failed write stays retryable and its later completion never reaches another accessor.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, AccessorOwner_003, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    auto &progress = PlaceholderProgressManager::GetInstance();
    auto first = sptr(new TaskProgressRecorder());
    auto other = sptr(new TaskProgressRecorder());
    ASSERT_EQ(progress.Register({1, 10, 11}, 100, first), E_OK);
    ASSERT_EQ(progress.Register({1, 10, 22}, 100, other), E_OK);
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(CreateOwnedTask(manager, 11, key), E_OK);
    int32_t fd = ActivateTask(manager, key);
    auto task = manager.taskMap_.at(key);
    task->totalSize = 1;
    Assistant::mockPwriteApi = true;
    Assistant::mockSendfileApi = true;
    auto request = MakeRequest(key, 0, {1}, 1, true);
    EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 1, 0)).WillOnce(Return(0));
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_TRY_AGAIN);
    progress.Drain();
    EXPECT_EQ(first->events.size(), 2U);
    EXPECT_TRUE(other->events.empty());
    uint8_t state = MakeFileSyncState(PLACEHOLDER_STATE_UNHYDRATED, 0);
    ExpectStateTransitions(fd, state);
    EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 1, 0)).WillOnce(Return(1));
    EXPECT_CALL(*mock_, Sendfile(fd, task->tempFd.Get(), _, 1)).WillOnce(Return(1));
    ASSERT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX, request), E_OK);
    progress.Drain();
    ASSERT_FALSE(first->events.empty());
    EXPECT_EQ(first->events.back().state, static_cast<int32_t>(HydrateProgressState::COMPLETED));
    EXPECT_EQ(first->events.back().accessorId, 11U);
    EXPECT_EQ(first->events.back().processedSize, 1U);
    EXPECT_TRUE(other->events.empty());
    EXPECT_FALSE(manager.HasOutstandingTask(TEST_SYNC_FOLDER, "file.txt", TEST_SYNC_FOLDER_INDEX));
}
/**
 * @tc.name: AccessorCancellation_001
 * @tc.desc: Two accessors cancelling each other's files receive only their own cancellation notifications.
 * @tc.type: SECU
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, AccessorCancellation_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    auto &progress = PlaceholderProgressManager::GetInstance();
    auto first = sptr(new TaskProgressRecorder());
    auto second = sptr(new TaskProgressRecorder());
    ASSERT_EQ(progress.Register({1, 10, 11}, 100, first), E_OK);
    ASSERT_EQ(progress.Register({1, 10, 22}, 100, second), E_OK);
    PlaceholderTaskManager::RequestKey firstKey;
    PlaceholderTaskManager::RequestKey secondKey;
    ASSERT_EQ(CreateOwnedTask(manager, 11, firstKey, "first.txt"), E_OK);
    ASSERT_EQ(CreateOwnedTask(manager, 22, secondKey, "second.txt"), E_OK);
    auto firstTask = manager.taskMap_.at(firstKey);
    auto secondTask = manager.taskMap_.at(secondKey);
    ASSERT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "first.txt", TEST_SYNC_FOLDER_INDEX, {1, 10, 22}), E_OK);
    ASSERT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "second.txt", TEST_SYNC_FOLDER_INDEX, {1, 10, 11}), E_OK);
    progress.Drain();
    ASSERT_EQ(first->events.size(), 2U);
    ASSERT_EQ(second->events.size(), 2U);
    EXPECT_EQ(first->events.front().filePath, TEST_SYNC_FOLDER + "/first.txt");
    EXPECT_EQ(second->events.front().filePath, TEST_SYNC_FOLDER + "/second.txt");
    EXPECT_EQ(first->events.back().filePath, TEST_SYNC_FOLDER + "/second.txt");
    EXPECT_EQ(second->events.back().filePath, TEST_SYNC_FOLDER + "/first.txt");
    EXPECT_EQ(first->events.back().state, static_cast<int32_t>(HydrateProgressState::CANCELLED));
    EXPECT_EQ(second->events.back().state, static_cast<int32_t>(HydrateProgressState::CANCELLED));
    EXPECT_EQ(first->events.back().accessorId, 11U);
    EXPECT_EQ(second->events.back().accessorId, 22U);
    EXPECT_EQ(firstTask->owner.accessorId, 11U);
    EXPECT_EQ(secondTask->owner.accessorId, 22U);
}

/**
 * @tc.name: AccessorCancellation_002
 * @tc.desc: An unsubscribed canceller gets no replay or fallback; a repeated failed cancel sends no event.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, AccessorCancellation_002, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    auto &progress = PlaceholderProgressManager::GetInstance();
    auto first = sptr(new TaskProgressRecorder());
    auto second = sptr(new TaskProgressRecorder());
    auto unrelated = sptr(new TaskProgressRecorder());
    ASSERT_EQ(progress.Register({1, 10, 11}, 100, first), E_OK);
    ASSERT_EQ(progress.Register({1, 10, 33}, 100, unrelated), E_OK);
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(CreateOwnedTask(manager, 11, key), E_OK);
    ASSERT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "file.txt", TEST_SYNC_FOLDER_INDEX, {1, 10, 22}), E_OK);
    progress.Drain();
    ASSERT_EQ(first->events.size(), 1U);
    EXPECT_EQ(first->events.front().state, static_cast<int32_t>(HydrateProgressState::PENDING));
    EXPECT_TRUE(unrelated->events.empty());
    ASSERT_EQ(progress.Register({1, 10, 22}, 100, second), E_OK);
    progress.Drain();
    EXPECT_TRUE(second->events.empty());
    EXPECT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "file.txt", TEST_SYNC_FOLDER_INDEX, {1, 10, 22}),
        E_NO_HYDRATION_IN_PROGRESS);
    progress.Drain();
    EXPECT_EQ(first->events.size(), 1U);
    EXPECT_TRUE(second->events.empty());
    EXPECT_TRUE(unrelated->events.empty());
}

/**
 * @tc.name: HydrationTemp_TargetUntouchedWhileReceiving_001
 * @tc.desc: Hydration chunks only land in the temp file; the target keeps its bytes, size and mtime.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationTemp_TargetUntouchedWhileReceiving_001, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    const std::vector<uint8_t> originalContent = {9, 9, 9, 9};
    ASSERT_EQ(pwrite(file.DuplicateFd().Get(), originalContent.data(), originalContent.size(), 0),
              static_cast<ssize_t>(originalContent.size()));
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key),
              E_OK);
    int32_t targetFd = ActivateTask(manager, key, file.DuplicateFd());
    ASSERT_GE(targetFd, 0);
    auto task = manager.taskMap_.at(key);
    task->totalSize = 8;
    DelegateRealFstat();
    struct stat before {};
    ASSERT_EQ(fstat(targetFd, &before), 0);

    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2, 3, 4}, 8, false)),
              E_OK);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 4, {5, 6, 7, 8}, 8, false)),
              E_OK);

    std::vector<uint8_t> targetContent(originalContent.size(), 0);
    ASSERT_EQ(pread(targetFd, targetContent.data(), targetContent.size(), 0),
              static_cast<ssize_t>(targetContent.size()));
    EXPECT_EQ(targetContent, originalContent);
    struct stat after {};
    ASSERT_EQ(fstat(targetFd, &after), 0);
    EXPECT_EQ(after.st_size, before.st_size);
    EXPECT_EQ(after.st_mtim.tv_sec, before.st_mtim.tv_sec);
    EXPECT_EQ(after.st_mtim.tv_nsec, before.st_mtim.tv_nsec);

    std::vector<uint8_t> tempContent(8, 0);
    ASSERT_EQ(pread(task->tempFd.Get(), tempContent.data(), tempContent.size(), 0),
              static_cast<ssize_t>(tempContent.size()));
    EXPECT_EQ(tempContent, std::vector<uint8_t>({1, 2, 3, 4, 5, 6, 7, 8}));
    EXPECT_EQ(task->tempSize, 8u);
}

/**
 * @tc.name: HydrationTemp_CompleteDeliveryCopyBack_001
 * @tc.desc: The final chunk copies the accumulated temp bytes back, flips the state and removes the temp file.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationTemp_CompleteDeliveryCopyBack_001, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key),
              E_OK);
    int32_t targetFd = ActivateTask(manager, key, file.DuplicateFd());
    ASSERT_GE(targetFd, 0);
    auto task = manager.taskMap_.at(key);
    UniqueFd verifyFd = file.DuplicateFd();
    uint8_t stateByte = MakeFileSyncState(PLACEHOLDER_STATE_UNHYDRATED, 0);
    ExpectStateTransitions(targetFd, stateByte);

    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2, 3}, 6, false)),
              E_OK);
    struct stat tempStat {};
    ASSERT_EQ(stat(task->tempPath.c_str(), &tempStat), 0);
    EXPECT_EQ(tempStat.st_size, static_cast<off_t>(3));

    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 3, {4, 5, 6}, 6, true)),
              E_OK);
    std::vector<uint8_t> targetContent(6, 0);
    ASSERT_EQ(pread(verifyFd.Get(), targetContent.data(), targetContent.size(), 0),
              static_cast<ssize_t>(targetContent.size()));
    EXPECT_EQ(targetContent, std::vector<uint8_t>({1, 2, 3, 4, 5, 6}));
    EXPECT_EQ(GetPlaceholderStateFromFileSyncState(stateByte), PLACEHOLDER_STATE_FULLY_HYDRATED);
    EXPECT_NE(stat(task->tempPath.c_str(), &tempStat), 0);
    PlaceholderTaskState state;
    EXPECT_FALSE(manager.GetTaskState(key, state));
}

/**
 * @tc.name: HydrationTemp_UserCancelCopiesReceivedBytes_001
 * @tc.desc: A user cancellation copies the received bytes back, restores timestamps and removes the temp file.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationTemp_UserCancelCopiesReceivedBytes_001, TestSize.Level1)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    const std::vector<uint8_t> originalContent = {7, 7, 7, 7};
    ASSERT_EQ(pwrite(file.DuplicateFd().Get(), originalContent.data(), originalContent.size(), 0),
              static_cast<ssize_t>(originalContent.size()));
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key),
              E_OK);
    int32_t targetFd = ActivateTask(manager, key, file.DuplicateFd());
    ASSERT_GE(targetFd, 0);
    auto task = manager.taskMap_.at(key);
    task->totalSize = 8;
    task->timestampsSaved = true;
    task->savedAtime = {111, 222};
    task->savedMtime = {333, 444};
    UniqueFd verifyFd = file.DuplicateFd();
    // The partial copy-back restores on its own exit and EraseTaskLocked adds the catch-all.
    EXPECT_CALL(*mock_, futimens(targetFd, _))
        .Times(2)
        .WillRepeatedly(Invoke([](int, const struct timespec *times) {
            EXPECT_EQ(times[0].tv_sec, 111);
            EXPECT_EQ(times[0].tv_nsec, 222);
            EXPECT_EQ(times[1].tv_sec, 333);
            EXPECT_EQ(times[1].tv_nsec, 444);
            return 0;
        }));

    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2, 3, 4}, 8, false)),
              E_OK);
    EXPECT_EQ(manager.CancelTask(TEST_SYNC_FOLDER, "file.txt", TEST_SYNC_FOLDER_INDEX), E_OK);

    std::vector<uint8_t> targetContent(4, 0);
    ASSERT_EQ(pread(verifyFd.Get(), targetContent.data(), targetContent.size(), 0),
              static_cast<ssize_t>(targetContent.size()));
    EXPECT_EQ(targetContent, std::vector<uint8_t>({1, 2, 3, 4}));
    struct stat removedStat {};
    EXPECT_NE(stat(task->tempPath.c_str(), &removedStat), 0);
    PlaceholderTaskState state;
    EXPECT_FALSE(manager.GetTaskState(key, state));
}

/**
 * @tc.name: HydrationTemp_TimeoutCancelKeepsTargetUntouched_001
 * @tc.desc: A timeout cancellation copies nothing back; the target keeps its original bytes and the temp is removed.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationTemp_TimeoutCancelKeepsTargetUntouched_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    const std::vector<uint8_t> originalContent = {7, 7, 7, 7};
    ASSERT_EQ(pwrite(file.DuplicateFd().Get(), originalContent.data(), originalContent.size(), 0),
              static_cast<ssize_t>(originalContent.size()));
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key),
              E_OK);
    int32_t targetFd = ActivateTask(manager, key, file.DuplicateFd());
    ASSERT_GE(targetFd, 0);
    auto task = manager.taskMap_.at(key);
    task->totalSize = 8;
    UniqueFd verifyFd = file.DuplicateFd();

    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2, 3, 4}, 8, false)),
              E_OK);
    manager.CancelTaskRecordLocked(task, PlaceholderTaskCancelReason::TIMEOUT);

    std::vector<uint8_t> targetContent(originalContent.size(), 0);
    ASSERT_EQ(pread(verifyFd.Get(), targetContent.data(), targetContent.size(), 0),
              static_cast<ssize_t>(targetContent.size()));
    EXPECT_EQ(targetContent, originalContent);
    struct stat removedStat {};
    EXPECT_NE(stat(task->tempPath.c_str(), &removedStat), 0);
    PlaceholderTaskState state;
    EXPECT_FALSE(manager.GetTaskState(key, state));
}

/**
 * @tc.name: HydrationTemp_OrphanCleanup_001
 * @tc.desc: Orphan cleanup removes every leftover temp file of the user and skips invalid users.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationTemp_OrphanCleanup_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    const std::string orphanA = TEST_HYDRATION_TEMP_DIR + "/orphan_a.tmp";
    const std::string orphanB = TEST_HYDRATION_TEMP_DIR + "/1_100.tmp";
    const auto writeOrphan = [](const std::string &path) {
        int32_t fd = open(path.c_str(), O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, S_IRUSR | S_IWUSR);
        return fd >= 0 && write(fd, "x", 1) == 1 && close(fd) == 0;
    };
    ASSERT_TRUE(writeOrphan(orphanA));
    ASSERT_TRUE(writeOrphan(orphanB));

    manager.CleanOrphanHydrationTempFiles(-1);

    DelegateRealDirApis();
    manager.CleanOrphanHydrationTempFiles(100);

    struct stat st {};
    EXPECT_NE(stat(orphanA.c_str(), &st), 0);
    EXPECT_NE(stat(orphanB.c_str(), &st), 0);
}

/**
 * @tc.name: HydrationTemp_RetryOverwritesStaleTemp_001
 * @tc.desc: A real activation truncates a stale same-inode temp file and completes without residue.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationTemp_RetryOverwritesStaleTemp_001, TestSize.Level2)
{
    auto callback = sptr(new HydrationCallbackStub());
    ASSERT_EQ(PlaceholderCallbackManager::GetInstance().RegisterCallbackTable(TEST_BUNDLE_NAME,
                                                                              TEST_SYNC_FOLDER_INDEX, callback),
              E_OK);
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    const std::string staleTempPath = TEST_HYDRATION_TEMP_DIR + "/1_100.tmp";
    {
        int32_t fd = open(staleTempPath.c_str(), O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, S_IRUSR | S_IWUSR);
        ASSERT_GE(fd, 0);
        const std::string staleData(32, 'x');
        ASSERT_EQ(write(fd, staleData.data(), staleData.size()),
                  static_cast<ssize_t>(staleData.size()));
        ASSERT_EQ(close(fd), 0);
    }

    ExpectAdmissionAndActivationMetadata(TEST_INODE_ID, TEST_INODE_ID);
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key,
                                        {100, "", file.GetPath(), TEST_TEMP_ROOT}),
              E_OK);
    manager.ActivateAndDispatch(manager.taskMap_.at(key));
    auto task = manager.FindTask(key);
    ASSERT_NE(task, nullptr);
    EXPECT_EQ(task->tempPath, staleTempPath);
    struct stat tempStat {};
    ASSERT_EQ(stat(staleTempPath.c_str(), &tempStat), 0);
    EXPECT_EQ(tempStat.st_size, 0);
    EXPECT_EQ(task->tempSize, 0u);

    uint8_t stateByte = MakeFileSyncState(PLACEHOLDER_STATE_UNHYDRATED, 0);
    ExpectStateTransitions(task->outputFd.Get(), stateByte);
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2}, 2, true)),
              E_OK);
    UniqueFd verifyFd = file.DuplicateFd();
    std::vector<uint8_t> targetContent(2, 0);
    ASSERT_EQ(pread(verifyFd.Get(), targetContent.data(), targetContent.size(), 0),
              static_cast<ssize_t>(targetContent.size()));
    EXPECT_EQ(targetContent, std::vector<uint8_t>({1, 2}));
    EXPECT_NE(stat(staleTempPath.c_str(), &tempStat), 0);
}

/**
 * @tc.name: HydrationTemp_CopyFailureKeepsTaskRetryable_001
 * @tc.desc: A failing copy-back keeps the task retryable with its temp file; a retry recopies from the start.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationTemp_CopyFailureKeepsTaskRetryable_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key),
              E_OK);
    int32_t targetFd = ActivateTask(manager, key, file.DuplicateFd());
    ASSERT_GE(targetFd, 0);
    auto task = manager.taskMap_.at(key);
    task->totalSize = 4;
    Assistant::mockPwriteApi = true;
    Assistant::mockSendfileApi = true;

    EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 2, 0)).WillOnce(Return(2));
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2}, 4, false)),
              E_OK);

    EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 2, 2)).WillOnce(Return(2));
    EXPECT_CALL(*mock_, Sendfile(targetFd, task->tempFd.Get(), _, 4))
        .WillOnce(Invoke([](int, int, off_t *, size_t) {
            errno = EIO;
            return -1;
        }));
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 2, {3, 4}, 4, true)),
              E_TRY_AGAIN);
    PlaceholderTaskState state;
    ASSERT_TRUE(manager.GetTaskState(key, state));
    EXPECT_EQ(state, PlaceholderTaskState::IN_PROGRESS);
    struct stat tempStat {};
    ASSERT_EQ(stat(task->tempPath.c_str(), &tempStat), 0);

    Mock::VerifyAndClearExpectations(mock_.get());
    EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 2, 2)).WillOnce(Return(2));
    EXPECT_CALL(*mock_, Sendfile(targetFd, task->tempFd.Get(), _, 4)).WillOnce(Return(4));
    EXPECT_CALL(*mock_, fgetxattr(targetFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
        .Times(2)
        .WillRepeatedly(Invoke(ReturnPlaceholderState(PLACEHOLDER_STATE_PARTIALLY_HYDRATED)));
    EXPECT_CALL(*mock_, fsetxattr(targetFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t), 0))
        .WillOnce(Return(0));
    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 2, {3, 4}, 4, true)),
              E_OK);
    EXPECT_NE(stat(task->tempPath.c_str(), &tempStat), 0);
    EXPECT_FALSE(manager.GetTaskState(key, state));
}

/**
 * @tc.name: HydrationTemp_CopyRetriesEINTR_001
 * @tc.desc: An interrupted sendfile is retried in place and the completion still succeeds.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationTemp_CopyRetriesEINTR_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    int32_t taskFd = ActivateTask(manager, key);
    ASSERT_GE(taskFd, 0);
    auto task = manager.taskMap_.at(key);
    task->totalSize = 4;
    task->timestampsSaved = true;
    task->savedAtime = {111, 222};
    task->savedMtime = {333, 444};
    Assistant::mockPwriteApi = true;
    Assistant::mockFsyncApi = true;
    Assistant::mockSendfileApi = true;

    EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 4, 0)).WillOnce(Return(4));
    {
        InSequence sequence;
        EXPECT_CALL(*mock_, Sendfile(taskFd, task->tempFd.Get(), _, 4))
            .WillOnce(Invoke([](int, int, off_t *, size_t) {
                errno = EINTR;
                return -1;
            }));
        EXPECT_CALL(*mock_, Sendfile(taskFd, task->tempFd.Get(), _, 4)).WillOnce(Return(4));
    }
    EXPECT_CALL(*mock_, Fsync(taskFd)).WillOnce(Return(0));
    EXPECT_CALL(*mock_, fgetxattr(taskFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
        .Times(2)
        .WillRepeatedly(Invoke(ReturnPlaceholderState(PLACEHOLDER_STATE_PARTIALLY_HYDRATED)));
    EXPECT_CALL(*mock_, fsetxattr(taskFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t), 0))
        .WillOnce(Return(0));
    // Copy exit restore plus the EraseTaskLocked catch-all.
    EXPECT_CALL(*mock_, futimens(taskFd, _))
        .Times(2)
        .WillRepeatedly(Invoke([](int, const struct timespec *times) {
            EXPECT_EQ(times[0].tv_sec, 111);
            EXPECT_EQ(times[0].tv_nsec, 222);
            EXPECT_EQ(times[1].tv_sec, 333);
            EXPECT_EQ(times[1].tv_nsec, 444);
            return 0;
        }));

    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2, 3, 4}, 4, true)),
              E_OK);
    PlaceholderTaskState state;
    EXPECT_FALSE(manager.GetTaskState(key, state));
}

/**
 * @tc.name: HydrationTemp_CopyUnexpectedEofRestores_001
 * @tc.desc: A short sendfile sequence ends with EOF, returns E_TRY_AGAIN and restores once.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationTemp_CopyUnexpectedEofRestores_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    int32_t taskFd = ActivateTask(manager, key);
    ASSERT_GE(taskFd, 0);
    auto task = manager.taskMap_.at(key);
    task->totalSize = 4;
    task->timestampsSaved = true;
    task->savedAtime = {111, 222};
    task->savedMtime = {333, 444};
    Assistant::mockPwriteApi = true;
    Assistant::mockSendfileApi = true;

    EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 4, 0)).WillOnce(Return(4));
    {
        InSequence sequence;
        EXPECT_CALL(*mock_, Sendfile(taskFd, task->tempFd.Get(), _, 4)).WillOnce(Return(2));
        EXPECT_CALL(*mock_, Sendfile(taskFd, task->tempFd.Get(), _, 2)).WillOnce(Return(0));
    }
    // Only the copy exit restores: the task stays retryable, so no EraseTaskLocked yet.
    EXPECT_CALL(*mock_, futimens(taskFd, _))
        .WillOnce(Invoke([](int, const struct timespec *times) {
            EXPECT_EQ(times[0].tv_sec, 111);
            EXPECT_EQ(times[0].tv_nsec, 222);
            EXPECT_EQ(times[1].tv_sec, 333);
            EXPECT_EQ(times[1].tv_nsec, 444);
            return 0;
        }));

    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2, 3, 4}, 4, true)),
              E_TRY_AGAIN);
    PlaceholderTaskState state;
    ASSERT_TRUE(manager.GetTaskState(key, state));
    EXPECT_EQ(state, PlaceholderTaskState::IN_PROGRESS);
    struct stat tempStat {};
    ASSERT_EQ(stat(task->tempPath.c_str(), &tempStat), 0);
    // Keep the teardown catch-all silent so the single-restore count above stays scoped.
    Mock::VerifyAndClearExpectations(mock_.get());
}

/**
 * @tc.name: HydrationTemp_CopyLseekFailSkipsRestore_001
 * @tc.desc: A failing lseek before any sendfile keeps the task retryable without a restore.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationTemp_CopyLseekFailSkipsRestore_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, OpenTaskFd(), key),
              E_OK);
    int32_t pipeFds[2];
    ASSERT_EQ(pipe(pipeFds), 0);
    close(pipeFds[0]);
    int32_t taskFd = ActivateTask(manager, key, UniqueFd(pipeFds[1]));
    ASSERT_GE(taskFd, 0);
    auto task = manager.taskMap_.at(key);
    task->totalSize = 4;
    task->timestampsSaved = true;
    task->savedAtime = {111, 222};
    task->savedMtime = {333, 444};
    Assistant::mockPwriteApi = true;

    EXPECT_CALL(*mock_, Pwrite(task->tempFd.Get(), _, 4, 0)).WillOnce(Return(4));
    // lseek on the pipe output fd fails with ESPIPE before any byte is sent, so the
    // copy must not touch the timestamps at all.
    EXPECT_CALL(*mock_, futimens(taskFd, _)).Times(0);
    EXPECT_CALL(*mock_, Sendfile(_, _, _, _)).Times(0);

    EXPECT_EQ(manager.Execute(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                              MakeRequest(key, 0, {1, 2, 3, 4}, 4, true)),
              E_TRY_AGAIN);
    PlaceholderTaskState state;
    ASSERT_TRUE(manager.GetTaskState(key, state));
    EXPECT_EQ(state, PlaceholderTaskState::IN_PROGRESS);
    struct stat tempStat {};
    ASSERT_EQ(stat(task->tempPath.c_str(), &tempStat), 0);
    // Keep the teardown catch-all silent so the zero-restore count above stays scoped.
    Mock::VerifyAndClearExpectations(mock_.get());
}

/**
 * @tc.name: HydrationTemp_TempCreationFailCancelsActivation_001
 * @tc.desc: A failing temp file open at activation cancels the task after the partial-state flip.
 * @tc.type: RELI
 * @tc.require: NA
 */
HWTEST_F(PlaceholderTaskManagerTest, HydrationTemp_TempCreationFailCancelsActivation_001, TestSize.Level2)
{
    auto &manager = PlaceholderTaskManager::GetInstance();
    HydrationTestFile file;
    ASSERT_TRUE(file.IsValid());
    // The temp file path is derived from the validated device and inode; occupying it
    // with a directory makes the O_CREAT open fail with EISDIR without any mocks.
    const std::string tempSlot = TEST_HYDRATION_TEMP_DIR + "/1_100.tmp";
    ASSERT_EQ(mkdir(tempSlot.c_str(), S_IRWXU | S_IRWXG | S_IRWXO), 0);

    ExpectAdmissionAndActivationMetadata(TEST_INODE_ID, TEST_INODE_ID);
    PlaceholderTaskManager::RequestKey key;
    ASSERT_EQ(manager.CreateHydrateTask(TEST_SYNC_FOLDER, "file.txt", TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                        CLOUD_DISK_HYDRATE_PRIORITY_NORMAL, file.DuplicateFd(), key,
                                        {100, "", file.GetPath(), TEST_TEMP_ROOT}),
              E_OK);
    manager.ActivateAndDispatch(manager.taskMap_.at(key));

    // Activation validated the file and flipped PARTIALLY before the temp creation
    // failed, so the task ends cancelled with a record while the slot stays occupied.
    EXPECT_EQ(manager.taskMap_.count(key), 0U);
    EXPECT_EQ(manager.cancellationRecordMap_.count(key), 1U);
    struct stat slotStat {};
    EXPECT_EQ(stat(tempSlot.c_str(), &slotStat), 0);
    ASSERT_EQ(rmdir(tempSlot.c_str()), 0);
}
} // namespace OHOS::FileManagement::CloudDiskService::Test
