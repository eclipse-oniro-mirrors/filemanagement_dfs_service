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

// The placeholder state operations under test are file-local statics in
// cloud_disk_service.cpp, so the production source is compiled directly into
// this translation unit (same pattern as cloud_disk_service_static_test).
#include "cloud_disk_service.cpp"

#include <memory>

#include "assistant.h"
#include "placeholder_progress_manager.h"

namespace OHOS::FileManagement::CloudDiskService {
void ResetPlaceholderMetaFileMock();
void SetPlaceholderMetaFileUpdateResult(int32_t result);
uint32_t GetPlaceholderMetaFileUpdateCount();
uint8_t GetLastPlaceholderMetaFileUpdateState();
std::string GetLastPlaceholderMetaFileUpdateName();
} // namespace OHOS::FileManagement::CloudDiskService

namespace OHOS::FileManagement::CloudDiskService::Test {
using namespace testing;
using namespace testing::ext;

namespace {
const std::string TEST_SYNC_FOLDER = "/storage/Users/currentUser/sync";
const std::string TEST_BUNDLE_NAME = "com.example.cloud.disk";
constexpr uint32_t TEST_SYNC_FOLDER_INDEX = 100;
const std::string TEST_HOLE_PATH = "/root/hole-file.txt";
constexpr uint8_t HOLE_SYNC_STATE = static_cast<uint8_t>(SyncState::SYNCING);

class DehydrateAllowStub final : public IRemoteStub<ICloudDiskServiceCallbackTable> {
public:
    void OnCallback(const CloudDiskCallbackReqHead &reqHead, CloudDiskCallbackContext &reqContext) override
    {
        if (reqHead.callbackType == CloudDiskCallbackType::DEHYDRATE && reqContext.dehydrateData != nullptr) {
            reqContext.dehydrateData->allow = true;
        }
    }

    int32_t OnRemoteRequest(uint32_t, MessageParcel &, MessageParcel &, MessageOption &) override
    {
        return E_OK;
    }
};
auto ReturnFileState(uint8_t state)
{
    return [state](int, const char *, void *value, size_t size) {
        *static_cast<uint8_t *>(value) = MakeFileSyncState(state, HOLE_SYNC_STATE);
        return static_cast<ssize_t>(size);
    };
}

constexpr time_t EXPECTED_ATIME_SEC = 111;
constexpr long EXPECTED_ATIME_NSEC = 222;
constexpr time_t EXPECTED_MTIME_SEC = 333;
constexpr long EXPECTED_MTIME_NSEC = 444;

int CheckRestoredTimes(int, const struct timespec *times)
{
    EXPECT_EQ(times[0].tv_sec, EXPECTED_ATIME_SEC);
    EXPECT_EQ(times[0].tv_nsec, EXPECTED_ATIME_NSEC);
    EXPECT_EQ(times[1].tv_sec, EXPECTED_MTIME_SEC);
    EXPECT_EQ(times[1].tv_nsec, EXPECTED_MTIME_NSEC);
    return 0;
}

void ExpectRootCountRefresh(const std::shared_ptr<AssistantMock> &mock)
{
    EXPECT_CALL(*mock, getxattr(StrEq("/root"), StrEq(CLOUD_DISK_PLACEHOLDER_COUNT_XATTR), _, sizeof(uint32_t)))
        .WillOnce(Invoke([](const char *, const char *, void *, size_t) {
            errno = ENODATA;
            return static_cast<ssize_t>(-1);
        }));
    EXPECT_CALL(*mock, setxattr(StrEq("/root"), StrEq(CLOUD_DISK_PLACEHOLDER_COUNT_XATTR), _, sizeof(uint32_t), 0))
        .WillOnce(Invoke([](const char *, const char *, const void *value, size_t, int) {
            EXPECT_EQ(static_cast<const uint8_t *>(value)[0], 1);
            return 0;
        }));
    EXPECT_CALL(*mock, getxattr(StrEq("/root"), StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
        .WillOnce(Invoke([](const char *, const char *, void *, size_t) {
            errno = ENODATA;
            return static_cast<ssize_t>(-1);
        }));
    EXPECT_CALL(*mock, setxattr(StrEq("/root"), StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t), 0))
        .WillOnce(Return(0));
}

} // namespace

class PlaceholderDehydrateCountTest : public testing::Test {
public:
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

    std::shared_ptr<AssistantMock> mock_;
};

/**
 * @tc.name: DehydrateFullyHydratedCount_001
 * @tc.desc: Dehydrating a fully hydrated placeholder restores timestamps and increases the count.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderDehydrateCountTest, DehydrateFullyHydratedCount_001, TestSize.Level1)
{
    PlaceholderStatePathContext context = {TEST_HOLE_PATH, "/root", 100, TEST_SYNC_FOLDER_INDEX, TEST_BUNDLE_NAME,
                                            TEST_SYNC_FOLDER};
    Assistant::mockFdApi = true;
    auto callback = sptr(new DehydrateAllowStub());
    ASSERT_EQ(PlaceholderCallbackManager::GetInstance().RegisterCallbackTable(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                                                              callback),
              E_OK);
    ResetPlaceholderMetaFileMock();
    constexpr int32_t fileFd = 97;
    constexpr off_t logicalSize = 4096;
    struct stat fileStat = {};
    fileStat.st_mode = S_IFREG;
    fileStat.st_size = logicalSize;
    fileStat.st_atim = {EXPECTED_ATIME_SEC, EXPECTED_ATIME_NSEC};
    fileStat.st_mtim = {EXPECTED_MTIME_SEC, EXPECTED_MTIME_NSEC};
    struct stat dirStat = {};
    dirStat.st_mode = S_IFDIR;

    EXPECT_CALL(*mock_, MockStat(StrEq(TEST_HOLE_PATH), _))
        .WillOnce(DoAll(SetArgPointee<1>(fileStat), Return(0)));
    EXPECT_CALL(*mock_, Open(StrEq(TEST_HOLE_PATH), _, _)).WillOnce(Return(fileFd));
    EXPECT_CALL(*mock_, fgetxattr(fileFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
        .Times(2)
        .WillRepeatedly(Invoke(ReturnFileState(PLACEHOLDER_STATE_FULLY_HYDRATED)));
    EXPECT_CALL(*mock_, fstat(fileFd, _)).WillOnce(DoAll(SetArgPointee<1>(fileStat), Return(0)));
    {
        InSequence sequence;
        EXPECT_CALL(*mock_, ftruncate(fileFd, 0)).WillOnce(Return(0));
        EXPECT_CALL(*mock_, ftruncate(fileFd, logicalSize)).WillOnce(Return(0));
        EXPECT_CALL(*mock_, futimens(fileFd, _)).WillOnce(Invoke(CheckRestoredTimes));
    }
    EXPECT_CALL(*mock_, fsetxattr(fileFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t), 0))
        .WillOnce(Invoke([](int, const char *, const void *value, size_t, int) {
            EXPECT_EQ(*static_cast<const uint8_t *>(value),
                      MakeFileSyncState(PLACEHOLDER_STATE_UNHYDRATED, HOLE_SYNC_STATE));
            return 0;
        }));
    ExpectRootCountRefresh(mock_);
    EXPECT_CALL(*mock_, MockStat(StrEq("/root"), _)).WillOnce(DoAll(SetArgPointee<1>(dirStat), Return(0)));

    EXPECT_EQ(DehydratePlaceholderFile(context, "file.txt"), E_OK);
    EXPECT_EQ(GetLastPlaceholderMetaFileUpdateState(), PLACEHOLDER_STATE_UNHYDRATED);
}

/**
 * @tc.name: DehydratePartiallyHydratedCount_001
 * @tc.desc: Dehydrating a partially hydrated placeholder keeps the count untouched.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderDehydrateCountTest, DehydratePartiallyHydratedCount_001, TestSize.Level1)
{
    PlaceholderStatePathContext context = {TEST_HOLE_PATH, "/root", 100, TEST_SYNC_FOLDER_INDEX, TEST_BUNDLE_NAME,
                                            TEST_SYNC_FOLDER};
    Assistant::mockFdApi = true;
    auto callback = sptr(new DehydrateAllowStub());
    ASSERT_EQ(PlaceholderCallbackManager::GetInstance().RegisterCallbackTable(TEST_BUNDLE_NAME, TEST_SYNC_FOLDER_INDEX,
                                                                              callback),
              E_OK);
    ResetPlaceholderMetaFileMock();
    constexpr int32_t fileFd = 98;
    constexpr off_t logicalSize = 4096;
    struct stat fileStat = {};
    fileStat.st_mode = S_IFREG;
    fileStat.st_size = logicalSize;
    fileStat.st_atim = {EXPECTED_ATIME_SEC, EXPECTED_ATIME_NSEC};
    fileStat.st_mtim = {EXPECTED_MTIME_SEC, EXPECTED_MTIME_NSEC};
    struct stat dirStat = {};
    dirStat.st_mode = S_IFDIR;

    EXPECT_CALL(*mock_, MockStat(StrEq(TEST_HOLE_PATH), _))
        .WillOnce(DoAll(SetArgPointee<1>(fileStat), Return(0)));
    EXPECT_CALL(*mock_, Open(StrEq(TEST_HOLE_PATH), _, _)).WillOnce(Return(fileFd));
    EXPECT_CALL(*mock_, fgetxattr(fileFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
        .Times(2)
        .WillRepeatedly(Invoke(ReturnFileState(PLACEHOLDER_STATE_PARTIALLY_HYDRATED)));
    EXPECT_CALL(*mock_, fstat(fileFd, _)).WillOnce(DoAll(SetArgPointee<1>(fileStat), Return(0)));
    {
        InSequence sequence;
        EXPECT_CALL(*mock_, ftruncate(fileFd, 0)).WillOnce(Return(0));
        EXPECT_CALL(*mock_, ftruncate(fileFd, logicalSize)).WillOnce(Return(0));
        EXPECT_CALL(*mock_, futimens(fileFd, _)).WillOnce(Return(0));
    }
    EXPECT_CALL(*mock_, fsetxattr(fileFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t), 0))
        .WillOnce(Invoke([](int, const char *, const void *value, size_t, int) {
            EXPECT_EQ(*static_cast<const uint8_t *>(value),
                      MakeFileSyncState(PLACEHOLDER_STATE_UNHYDRATED, HOLE_SYNC_STATE));
            return 0;
        }));
    EXPECT_CALL(*mock_, getxattr(StrEq("/root"), _, _, _)).Times(0);
    EXPECT_CALL(*mock_, setxattr(StrEq("/root"), _, _, _, _)).Times(0);
    EXPECT_CALL(*mock_, MockStat(StrEq("/root"), _)).WillOnce(DoAll(SetArgPointee<1>(dirStat), Return(0)));

    EXPECT_EQ(DehydratePlaceholderFile(context, "file.txt"), E_OK);
    EXPECT_EQ(GetLastPlaceholderMetaFileUpdateState(), PLACEHOLDER_STATE_UNHYDRATED);
}

/**
 * @tc.name: MarkPlaceholderCountUnchanged_001
 * @tc.desc: Marking a normal file as fully hydrated keeps the placeholder count untouched.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(PlaceholderDehydrateCountTest, MarkPlaceholderCountUnchanged_001, TestSize.Level1)
{
    PlaceholderStatePathContext context = {TEST_HOLE_PATH, "/root", 100, TEST_SYNC_FOLDER_INDEX, TEST_BUNDLE_NAME,
                                            TEST_SYNC_FOLDER};
    Assistant::mockFdApi = true;
    ResetPlaceholderMetaFileMock();
    constexpr int32_t fileFd = 99;
    struct stat fileStat = {};
    fileStat.st_mode = S_IFREG;
    struct stat dirStat = {};
    dirStat.st_mode = S_IFDIR;

    EXPECT_CALL(*mock_, MockStat(StrEq(TEST_HOLE_PATH), _))
        .WillOnce(DoAll(SetArgPointee<1>(fileStat), Return(0)));
    EXPECT_CALL(*mock_, Open(StrEq(TEST_HOLE_PATH), _, _)).WillOnce(Return(fileFd));
    EXPECT_CALL(*mock_, fgetxattr(fileFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
        .Times(2)
        .WillRepeatedly(Invoke([](int, const char *, void *value, size_t size) {
            *static_cast<uint8_t *>(value) = MakeFileSyncState(PLACEHOLDER_STATE_NONE, HOLE_SYNC_STATE);
            return static_cast<ssize_t>(size);
        }));
    EXPECT_CALL(*mock_, fsetxattr(fileFd, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t), 0))
        .WillOnce(Invoke([](int, const char *, const void *value, size_t, int) {
            EXPECT_EQ(*static_cast<const uint8_t *>(value),
                      MakeFileSyncState(PLACEHOLDER_STATE_FULLY_HYDRATED, HOLE_SYNC_STATE));
            return 0;
        }));
    EXPECT_CALL(*mock_, getxattr(_, _, _, _)).Times(0);
    EXPECT_CALL(*mock_, setxattr(_, _, _, _, _)).Times(0);
    EXPECT_CALL(*mock_, MockStat(StrEq("/root"), _)).WillOnce(DoAll(SetArgPointee<1>(dirStat), Return(0)));

    EXPECT_EQ(ChangePlaceholderStateOnly(context, PlaceholderStateTransition::MARK), E_OK);
    EXPECT_EQ(GetLastPlaceholderMetaFileUpdateState(), PLACEHOLDER_STATE_FULLY_HYDRATED);
}
} // namespace OHOS::FileManagement::CloudDiskService::Test
