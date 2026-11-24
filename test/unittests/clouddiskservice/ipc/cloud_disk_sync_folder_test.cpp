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

#include <cstring>
#include <dlfcn.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "cloud_disk_sync_folder.h"
#include "cloud_disk_service_error.h"

#include "assistant.h"
#include "cloud_disk_sync_folder_system_mock.h"
#include "placeholder_helper.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement::CloudDiskService {
namespace Test {
using namespace std;
using namespace testing;
using namespace testing::ext;

ACTION_P(SetByteXattr, byteValue)
{
    memcpy(arg2, &byteValue, sizeof(char));
    return static_cast<ssize_t>(sizeof(char));
}

class CloudDiskSyncFolderTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static inline shared_ptr<AssistantMock> insMock;
    static inline shared_ptr<SyncFolderAssistantMock> syncFolderMock;
};

void CloudDiskSyncFolderTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
}

void CloudDiskSyncFolderTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
}

void CloudDiskSyncFolderTest::SetUp(void)
{
    GTEST_LOG_(INFO) << "SetUp";
    insMock = make_shared<AssistantMock>();
    Assistant::ins = insMock;
    Assistant::mockFdApi = false;
    Assistant::mockLstatApi = false;
    Assistant::mockErrno = 0;
    syncFolderMock = make_shared<SyncFolderAssistantMock>();
    SyncFolderAssistant::ins = syncFolderMock;

    // The libc interceptors forward these calls to the mock unconditionally. Cleanup tests use
    // the real filesystem, so delegate the directory/stat calls back to libc by default.
    ON_CALL(*insMock, readdir(_)).WillByDefault(Invoke([](DIR *dir) {
        auto realReaddir = reinterpret_cast<struct dirent *(*)(DIR *)>(dlsym(RTLD_NEXT, "readdir"));
        return realReaddir == nullptr ? nullptr : realReaddir(dir);
    }));
    ON_CALL(*insMock, CloseDir(_)).WillByDefault(Invoke([](DIR *dir) {
        auto realClosedir = reinterpret_cast<int (*)(DIR *)>(dlsym(RTLD_NEXT, "closedir"));
        return realClosedir == nullptr ? -1 : realClosedir(dir);
    }));
    ON_CALL(*insMock, fstat(_, _)).WillByDefault(Invoke([](int fd, struct stat *buf) {
        auto realFstat = reinterpret_cast<int (*)(int, struct stat *)>(dlsym(RTLD_NEXT, "fstat"));
        return realFstat == nullptr ? -1 : realFstat(fd, buf);
    }));
    ON_CALL(*insMock, fremovexattr(_, _)).WillByDefault(Invoke([](int, const char *) {
        errno = ENODATA;
        return -1;
    }));
    ON_CALL(*syncFolderMock, unlinkat(_, _, _)).WillByDefault(Invoke([](int dirFd, const char *name, int flags) {
        auto realUnlinkat = reinterpret_cast<int (*)(int, const char *, int)>(dlsym(RTLD_NEXT, "unlinkat"));
        return realUnlinkat == nullptr ? -1 : realUnlinkat(dirFd, name, flags);
    }));
}

void CloudDiskSyncFolderTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "TearDown";
    Assistant::ins = nullptr;
    insMock = nullptr;
    SyncFolderAssistant::ins = nullptr;
    syncFolderMock = nullptr;
}

static bool CleanupPathExists(const std::string &path)
{
    struct stat st {};
    return stat(path.c_str(), &st) == 0;
}

static void ExpectCleanupState(uint8_t state)
{
    EXPECT_CALL(*CloudDiskSyncFolderTest::insMock,
                fgetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
        .WillOnce(SetByteXattr(MakeFileSyncState(state, 0)))
        .RetiresOnSaturation();
}

static void ResetCleanupDir(const std::string &path)
{
    std::string command = "rm -rf " + path;
    std::system(command.c_str());
    command = "mkdir -p " + path;
    std::system(command.c_str());
}

/**
 * @tc.name: GetCloudDiskSyncFolderInstanceTest001
 * @tc.desc: Verify the GetCloudDiskSyncFolderInstance function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, GetCloudDiskSyncFolderInstanceTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetCloudDiskSyncFolderInstanceTest001 start";
    try {
        CloudDiskSyncFolder& instance1 = CloudDiskSyncFolder::GetInstance();
        CloudDiskSyncFolder& instance2 = CloudDiskSyncFolder::GetInstance();
        EXPECT_EQ(&instance1, &instance2);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetCloudDiskSyncFolderInstanceTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetCloudDiskSyncFolderInstanceTest001 end";
}

/**
 * @tc.name: AddSyncFolderTest001
 * @tc.desc: Verify the AddSyncFolder function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, AddSyncFolderTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AddSyncFolderTest001 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        uint32_t syncFolderIndex = 12345;
        SyncFolderValue value;
        value.bundleName = "com.test.example";
        value.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        EXPECT_EQ(syncFolder.GetSyncFolderSize(), 1);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AddSyncFolderTest001 failed";
    }
    GTEST_LOG_(INFO) << "AddSyncFolderTest001 end";
}

/**
 * @tc.name: DeleteSyncFolderTest001
 * @tc.desc: Verify the DeleteSyncFolder function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, DeleteSyncFolderTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeleteSyncFolderTest001 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        uint32_t syncFolderIndex = 12345;
        SyncFolderValue value;
        value.bundleName = "com.test.example";
        value.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        EXPECT_TRUE(syncFolder.CheckSyncFolder(syncFolderIndex));
        EXPECT_EQ(syncFolder.GetSyncFolderSize(), 1);

        syncFolder.DeleteSyncFolder(syncFolderIndex);
        EXPECT_FALSE(syncFolder.CheckSyncFolder(syncFolderIndex));
        EXPECT_EQ(syncFolder.GetSyncFolderSize(), 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "DeleteSyncFolderTest001 failed";
    }
    GTEST_LOG_(INFO) << "DeleteSyncFolderTest001 end";
}

/**
 * @tc.name: GetSyncFolderSizeTest001
 * @tc.desc: Verify the GetSyncFolderSize function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, GetSyncFolderSizeTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderSizeTest001 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        uint32_t syncFolderIndex1 = 12345;
        SyncFolderValue value1;
        value1.bundleName = "com.test.example";
        value1.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex1, value1);

        uint32_t syncFolderIndex2 = 67890;
        SyncFolderValue value2;
        value2.bundleName = "com.test.example";
        value2.path = "/test456";
        syncFolder.AddSyncFolder(syncFolderIndex2, value2);
        EXPECT_EQ(syncFolder.GetSyncFolderSize(), 2);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderSizeTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderSizeTest001 end";
}

/**
 * @tc.name: CheckSyncFolderTest001
 * @tc.desc: Verify the CheckSyncFolder function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, CheckSyncFolderTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "CheckSyncFolderTest001 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        uint32_t syncFolderIndex = 12345;
        SyncFolderValue value;
        value.bundleName = "com.test.example";
        value.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        bool ret = syncFolder.CheckSyncFolder(syncFolderIndex);
        EXPECT_EQ(ret, true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "CheckSyncFolderTest001 failed";
    }
    GTEST_LOG_(INFO) << "CheckSyncFolderTest001 end";
}

/**
 * @tc.name: CheckSyncFolderTest002
 * @tc.desc: Verify the CheckSyncFolder function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, CheckSyncFolderTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "CheckSyncFolderTest002 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        uint32_t syncFolderIndex = 12345;
        SyncFolderValue value;
        value.bundleName = "com.test.example";
        value.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        uint32_t syncFolderIndex_notExists = 999;
        bool ret = syncFolder.CheckSyncFolder(syncFolderIndex_notExists);
        EXPECT_EQ(ret, false);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "CheckSyncFolderTest002 failed";
    }
    GTEST_LOG_(INFO) << "CheckSyncFolderTest002 end";
}

/**
 * @tc.name: GetSyncFolderByIndexTest001
 * @tc.desc: Verify the GetSyncFolderByIndex function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, GetSyncFolderByIndexTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderByIndexTest001 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        uint32_t syncFolderIndex = 12345;
        SyncFolderValue value;
        value.bundleName = "com.test.example";
        value.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        string path;
        bool ret = syncFolder.GetSyncFolderByIndex(syncFolderIndex, path);
        EXPECT_EQ(ret, true);
        EXPECT_EQ(path, "/test123");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderByIndexTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderByIndexTest001 end";
}

/**
 * @tc.name: GetSyncFolderByIndexTest002
 * @tc.desc: Verify the GetSyncFolderByIndex function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, GetSyncFolderByIndexTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderByIndexTest002 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        uint32_t syncFolderIndex = 12345;
        SyncFolderValue value;
        value.bundleName = "com.test.example";
        value.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        string path;
        uint32_t syncFolderIndex_notExists = 999;
        bool ret = syncFolder.GetSyncFolderByIndex(syncFolderIndex_notExists, path);
        EXPECT_EQ(ret, false);
        EXPECT_EQ(path, "");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderByIndexTest002 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderByIndexTest002 end";
}

/**
 * @tc.name: GetIndexBySyncFolderTest001
 * @tc.desc: Verify the GetIndexBySyncFolder function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, GetIndexBySyncFolderTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetIndexBySyncFolderTest001 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        uint32_t syncFolderIndex = 12345;
        SyncFolderValue value;
        value.bundleName = "com.test.example";
        value.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        uint32_t index = 0;
        bool ret = syncFolder.GetIndexBySyncFolder(index, "/test123");
        EXPECT_EQ(ret, true);
        EXPECT_EQ(index, syncFolderIndex);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetIndexBySyncFolderTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetIndexBySyncFolderTest001 end";
}

/**
 * @tc.name: GetIndexBySyncFolderTest002
 * @tc.desc: Verify the GetIndexBySyncFolder function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, GetIndexBySyncFolderTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetIndexBySyncFolderTest002 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        uint32_t syncFolderIndex = 12345;
        SyncFolderValue value;
        value.bundleName = "com.test.example";
        value.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        uint32_t index = 0;
        bool ret = syncFolder.GetIndexBySyncFolder(index, "/not_exists");
        EXPECT_EQ(ret, false);
        EXPECT_EQ(index, 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetIndexBySyncFolderTest002 failed";
    }
    GTEST_LOG_(INFO) << "GetIndexBySyncFolderTest002 end";
}

/**
 * @tc.name: GetIndexBySyncFolderTest003
 * @tc.desc: Verify the GetIndexBySyncFolder function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, GetIndexBySyncFolderTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetIndexBySyncFolderTest003 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        uint32_t syncFolderIndex1 = 12345;
        SyncFolderValue value1;
        value1.bundleName = "com.test.example";
        value1.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex1, value1);

        uint32_t syncFolderIndex2 = 67890;
        SyncFolderValue value2;
        value2.bundleName = "com.test.example";
        value2.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex2, value2);

        uint32_t index = 0;
        bool ret = syncFolder.GetIndexBySyncFolder(index, "/test123");
        EXPECT_EQ(ret, true);
        EXPECT_EQ(index, syncFolderIndex2);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetIndexBySyncFolderTest003 failed";
    }
    GTEST_LOG_(INFO) << "GetIndexBySyncFolderTest003 end";
}

/**
 * @tc.name: GetSyncFolderMapTest001
 * @tc.desc: Verify the GetSyncFolderMap function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, GetSyncFolderMapTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderMapTest001 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        uint32_t syncFolderIndex1 = 123;
        SyncFolderValue value1;
        value1.bundleName = "com.test.example";
        value1.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex1, value1);

        uint32_t syncFolderIndex2 = 456;
        SyncFolderValue value2;
        value2.bundleName = "com.test.example";
        value2.path = "/test456";
        syncFolder.AddSyncFolder(syncFolderIndex2, value2);

        auto folderMap = syncFolder.GetSyncFolderMap();
        EXPECT_EQ(folderMap.size(), 2);

        auto it1 = folderMap.find(syncFolderIndex1);
        EXPECT_TRUE(it1 != folderMap.end());
        EXPECT_EQ(it1->second.path, "/test123");

        auto it2 = folderMap.find(syncFolderIndex2);
        EXPECT_TRUE(it2 != folderMap.end());
        EXPECT_EQ(it2->second.path, "/test456");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderMapTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderMapTest001 end";
}

/**
 * @tc.name: PathToPhysicalPathTest001
 * @tc.desc: Verify the PathToPhysicalPath function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, PathToPhysicalPathTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PathToPhysicalPathTest001 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        string path = "/storage/Users/currentUser/testfile.txt";
        string userId = "100";
        string realpath;
        EXPECT_CALL(*syncFolderMock, realpath(_, _)).WillOnce(Invoke([](const char *, char *) {
            errno = EACCES;
            return static_cast<char *>(nullptr);
        }));
        int32_t ret = syncFolder.PathToPhysicalPath(path, userId, realpath);
        EXPECT_EQ(ret, E_INVALID_ARG);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "PathToPhysicalPathTest001 failed";
    }
    GTEST_LOG_(INFO) << "PathToPhysicalPathTest001 end";
}

/**
 * @tc.name: PathToPhysicalPathTest002
 * @tc.desc: Verify the PathToPhysicalPath function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, PathToPhysicalPathTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PathToPhysicalPathTest002 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        string path = "/storage/Users/otherUser/testfile.txt";
        string userId = "100";
        string realpath;
        int32_t ret = syncFolder.PathToPhysicalPath(path, userId, realpath);
        EXPECT_EQ(ret, E_INVALID_ARG);
        EXPECT_TRUE(realpath.empty());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "PathToPhysicalPathTest002 failed";
    }
    GTEST_LOG_(INFO) << "PathToPhysicalPathTest002 end";
}

/**
 * @tc.name: PathToPhysicalPathTest003
 * @tc.desc: Verify the PathToPhysicalPath function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, PathToPhysicalPathTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PathToPhysicalPathTest003 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        string path = "/storage/Users/currentUser/testfile.txt";
        string userId = "100";
        string realpath;
        const char *resolvedPath = "/data/service/el2/100/hmdfs/account/files/Docs/testfile.txt";
        EXPECT_CALL(*syncFolderMock, realpath(_, _))
            .WillOnce(DoAll(
                SetArrayArgument<1>(resolvedPath, resolvedPath + strlen(resolvedPath) + 1),
                Return(const_cast<char*>(resolvedPath))
            ));
        int32_t ret = syncFolder.PathToPhysicalPath(path, userId, realpath);
        EXPECT_EQ(ret, E_OK);
        EXPECT_EQ(realpath, resolvedPath);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "PathToPhysicalPathTest003 failed";
    }
    GTEST_LOG_(INFO) << "PathToPhysicalPathTest003 end";
}

/**
 * @tc.name: PathToMntPathBySandboxPathTest001
 * @tc.desc: Verify the PathToMntPathBySandboxPath function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, PathToMntPathBySandboxPathTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PathToMntPathBySandboxPathTest001 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        string path = "/storage/Users/currentUser/testfile.txt";
        string userId = "100";
        string realpath;
        const char *resolvedPath = "/mnt/hmdfs/100/account/device_view/local/files/Docs/testfile.txt";
        EXPECT_CALL(*syncFolderMock, realpath(_, _))
            .WillOnce(DoAll(
                SetArrayArgument<1>(resolvedPath, resolvedPath + strlen(resolvedPath) + 1),
                Return(const_cast<char*>(resolvedPath))
            ));
        int32_t ret = syncFolder.PathToMntPathBySandboxPath(path, userId, realpath);
        EXPECT_EQ(ret, E_OK);
        EXPECT_EQ(realpath, resolvedPath);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "PathToMntPathBySandboxPathTest001 failed";
    }
    GTEST_LOG_(INFO) << "PathToMntPathBySandboxPathTest001 end";
}

/**
 * @tc.name: PathToMntPathBySandboxPathTest002
 * @tc.desc: Verify the PathToMntPathBySandboxPath function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, PathToMntPathBySandboxPathTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PathToMntPathBySandboxPathTest002 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        string path = "/storage/Users/otherUser/testfile.txt";
        string userId = "100";
        string realpath;
        int32_t ret = syncFolder.PathToMntPathBySandboxPath(path, userId, realpath);
        EXPECT_EQ(ret, E_INVALID_ARG);
        EXPECT_TRUE(realpath.empty());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "PathToMntPathBySandboxPathTest002 failed";
    }
    GTEST_LOG_(INFO) << "PathToMntPathBySandboxPathTest002 end";
}

/**
 * @tc.name: PathToMntPathBySandboxPathTest003
 * @tc.desc: Verify the PathToMntPathBySandboxPath function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, PathToMntPathBySandboxPathTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PathToMntPathBySandboxPathTest003 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        string path = "/storage/Users/currentUser/testfile.txt";
        string userId = "100";
        string realpath;
        EXPECT_CALL(*syncFolderMock, realpath(_, _)).WillOnce(Invoke([](const char *, char *) {
            errno = EACCES;
            return static_cast<char *>(nullptr);
        }));
        int32_t ret = syncFolder.PathToMntPathBySandboxPath(path, userId, realpath);
        EXPECT_EQ(ret, E_INVALID_ARG);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "PathToMntPathBySandboxPathTest003 failed";
    }
    GTEST_LOG_(INFO) << "PathToMntPathBySandboxPathTest003 end";
}

/**
 * @tc.name: PathToMntPathByPhysicalPathTest001
 * @tc.desc: Verify the PathToMntPathByPhysicalPath function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, PathToMntPathByPhysicalPathTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PathToMntPathByPhysicalPathTest001 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        string path = "/data/service/el2/100/hmdfs/account/files/Docs/testfile.txt";
        string userId = "100";
        string realpath;
        bool ret = syncFolder.PathToMntPathByPhysicalPath(path, userId, realpath);
        EXPECT_EQ(ret, true);
        EXPECT_EQ(realpath, "/mnt/hmdfs/100/account/device_view/local/files/Docs/testfile.txt");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "PathToMntPathByPhysicalPathTest001 failed";
    }
    GTEST_LOG_(INFO) << "PathToMntPathByPhysicalPathTest001 end";
}

/**
 * @tc.name: PathToMntPathByPhysicalPathTest002
 * @tc.desc: Verify the PathToMntPathByPhysicalPath function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, PathToMntPathByPhysicalPathTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PathToMntPathByPhysicalPathTest002 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        string path = "/data/service/el2/123456/hmdfs/account/files/Docs/testfile.txt";
        string userId = "100";
        string realpath;
        bool ret = syncFolder.PathToMntPathByPhysicalPath(path, userId, realpath);
        EXPECT_EQ(ret, false);
        EXPECT_TRUE(realpath.empty());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "PathToMntPathByPhysicalPathTest002 failed";
    }
    GTEST_LOG_(INFO) << "PathToMntPathByPhysicalPathTest002 end";
}

/**
 * @tc.name: PathToSandboxPathByPhysicalPathTest001
 * @tc.desc: Verify the PathToSandboxPathByPhysicalPath function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, PathToSandboxPathByPhysicalPathTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PathToSandboxPathByPhysicalPathTest001 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        string path = "/data/service/el2/100/hmdfs/account/files/Docs/testfile.txt";
        string userId = "100";
        string realpath;
        bool ret = syncFolder.PathToSandboxPathByPhysicalPath(path, userId, realpath);
        EXPECT_EQ(ret, true);
        EXPECT_EQ(realpath, "/storage/Users/currentUser/testfile.txt");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "PathToSandboxPathByPhysicalPathTest001 failed";
    }
    GTEST_LOG_(INFO) << "PathToSandboxPathByPhysicalPathTest001 end";
}

/**
 * @tc.name: PathToSandboxPathByPhysicalPathTest002
 * @tc.desc: Verify the PathToSandboxPathByPhysicalPath function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, PathToSandboxPathByPhysicalPathTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PathToSandboxPathByPhysicalPathTest002 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        string path = "/data/service/el2/123456/hmdfs/account/files/Docs/testfile.txt";
        string userId = "100";
        string realpath;
        bool ret = syncFolder.PathToSandboxPathByPhysicalPath(path, userId, realpath);
        EXPECT_EQ(ret, false);
        EXPECT_TRUE(realpath.empty());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "PathToSandboxPathByPhysicalPathTest002 failed";
    }
    GTEST_LOG_(INFO) << "PathToSandboxPathByPhysicalPathTest002 end";
}

/**
 * @tc.name: ClearMapTest001
 * @tc.desc: Verify the ClearMap function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, ClearMapTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ClearMapTest001 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        syncFolder.ClearMap();
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ClearMapTest001 failed";
    }
    GTEST_LOG_(INFO) << "ClearMapTest001 end";
}

/**
 * @tc.name: ReplacePathPrefixTest001
 * @tc.desc: Verify the ReplacePathPrefix function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, ReplacePathPrefixTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReplacePathPrefixTest001 start";
    try {
        string oldPrefix = "/storage/Users/currentUser";
        string newPrefix = "/data/service/el2/100/hmdfs/account/files/Docs";
        string inputPath = "/storage/Users/currentUser/a/b/file.txt";
        const char *outputRealPath = "/data/service/el2/100/hmdfs/account/files/Docs/a/b/file.txt";
        string outputPath = "";
        string outputPathTrue = "/data/service/el2/100/hmdfs/account/files/Docs/a/b/file.txt";
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        EXPECT_CALL(*syncFolderMock, realpath(_, _))
            .WillOnce(DoAll(
                SetArrayArgument<1>(outputRealPath, outputRealPath + strlen(outputRealPath) + 1),
                Return(const_cast<char*>(outputRealPath))
            ));
        int32_t ret = syncFolder.ReplacePathPrefix(oldPrefix, newPrefix, inputPath, outputPath);
        EXPECT_EQ(ret, E_OK);
        EXPECT_EQ(outputPath, outputPathTrue);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ReplacePathPrefixTest001 failed";
    }
    GTEST_LOG_(INFO) << "ReplacePathPrefixTest001 end";
}

/**
 * @tc.name: ReplacePathPrefixTest002
 * @tc.desc: Verify the ReplacePathPrefix function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, ReplacePathPrefixTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReplacePathPrefixTest002 start";
    try {
        string oldPrefix = "/storage/Users/currentUser";
        string newPrefix = "/data/service/el2/100/hmdfs/account/files/Docs";
        string inputPath = "/storage/Users/currentUser/a/../../file.txt";
        const char *outputRealPath = "/data/service/el2/100/hmdfs/account/files/file.txt";
        string outputPath = "";
        string outputPathTrue = "";
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        EXPECT_CALL(*syncFolderMock, realpath(_, _))
            .WillOnce(DoAll(
                SetArrayArgument<1>(outputRealPath, outputRealPath + strlen(outputRealPath) + 1),
                Return(const_cast<char*>(outputRealPath))
            ));
        int32_t ret = syncFolder.ReplacePathPrefix(oldPrefix, newPrefix, inputPath, outputPath);
        EXPECT_EQ(ret, E_INVALID_ARG);
        EXPECT_EQ(outputPath, outputPathTrue);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ReplacePathPrefixTest002 failed";
    }
    GTEST_LOG_(INFO) << "ReplacePathPrefixTest002 end";
}

/**
 * @tc.name: ReplacePathPrefixTest003
 * @tc.desc: Verify the ReplacePathPrefix function with invalid prefix
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, ReplacePathPrefixTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReplacePathPrefixTest003 start";
    try {
        string oldPrefix = "/storage/Users/currentUser";
        string newPrefix = "/data/service/el2/100/hmdfs/account/files/Docs";
        string inputPath = "/storage/Users/otherUser/testfile.txt";
        string outputPath = "";
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        int32_t ret = syncFolder.ReplacePathPrefix(oldPrefix, newPrefix, inputPath, outputPath);
        EXPECT_EQ(ret, E_INVALID_ARG);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ReplacePathPrefixTest003 failed";
    }
    GTEST_LOG_(INFO) << "ReplacePathPrefixTest003 end";
}

/**
 * @tc.name: ReplacePathPrefixTest004
 * @tc.desc: Verify the ReplacePathPrefix function with short input path
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, ReplacePathPrefixTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReplacePathPrefixTest004 start";
    try {
        string oldPrefix = "/storage/Users/currentUser";
        string newPrefix = "/data/service/el2/100/hmdfs/account/files/Docs";
        string inputPath = "/storage/Users";
        string outputPath = "";
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        int32_t ret = syncFolder.ReplacePathPrefix(oldPrefix, newPrefix, inputPath, outputPath);
        EXPECT_EQ(ret, E_INVALID_ARG);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ReplacePathPrefixTest004 failed";
    }
    GTEST_LOG_(INFO) << "ReplacePathPrefixTest004 end";
}

/**
 * @tc.name: ReplacePathPrefixTest005
 * @tc.desc: Verify the ReplacePathPrefix function with path traversal
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, ReplacePathPrefixTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReplacePathPrefixTest005 start";
    try {
        string oldPrefix = "/storage/Users/currentUser";
        string newPrefix = "/data/service/el2/100/hmdfs/account/files/Docs";
        string inputPath = "/storage/Users/currentUser/testfile.txt";
        const char *outputRealPath = "/data/service/el2/100/hmdfs/account/files/testfile.txt";
        string outputPath = "";
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        EXPECT_CALL(*syncFolderMock, realpath(_, _))
            .WillOnce(DoAll(
                SetArrayArgument<1>(outputRealPath, outputRealPath + strlen(outputRealPath) + 1),
                Return(const_cast<char*>(outputRealPath))
            ));
        int32_t ret = syncFolder.ReplacePathPrefix(oldPrefix, newPrefix, inputPath, outputPath);
        EXPECT_EQ(ret, E_INVALID_ARG);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ReplacePathPrefixTest005 failed";
    }
    GTEST_LOG_(INFO) << "ReplacePathPrefixTest005 end";
;
}

/**
 * @tc.name: GetSyncFolderValueByIndexTest001
 * @tc.desc: Verify the GetSyncFolderValueByIndex function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, GetSyncFolderValueByIndexTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderValueByIndexTest001 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        uint32_t syncFolderIndex = 12345;
        SyncFolderValue value;
        value.bundleName = "com.test.example";
        value.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        
        SyncFolderValue resultValue;
        bool ret = syncFolder.GetSyncFolderValueByIndex(syncFolderIndex, resultValue);
        EXPECT_EQ(ret, true);
        EXPECT_EQ(resultValue.bundleName, "com.test.example");
        EXPECT_EQ(resultValue.path, "/test123");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderValueByIndexTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderValueByIndexTest001 end";
}

/**
 * @tc.name: GetSyncFolderValueByIndexTest002
 * @tc.desc: Verify the GetSyncFolderValueByIndex function with non-existent index
 * @tc.type: FUNC FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, GetSyncFolderValueByIndexTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderValueByIndexTest002 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        uint32_t syncFolderIndex = 12345;
        SyncFolderValue value;
        value.bundleName = "com.test.example";
        value.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        
        SyncFolderValue resultValue;
        uint32_t notExistsIndex = 999;
        bool ret = syncFolder.GetSyncFolderValueByIndex(notExistsIndex, resultValue);
        EXPECT_EQ(ret, false);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderValueByIndexTestTest002 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderValueByIndexTest002 end";
}

/**
 * @tc.name: DeleteSyncFolderTest002
 * @tc.desc: Verify the DeleteSyncFolder function with non-existent index
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, DeleteSyncFolderTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeleteSyncFolderTest002 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        uint32_t syncFolderIndex = 12345;
        SyncFolderValue value;
        value.bundleName = "com.test.example";
        value.path = "/test123";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        EXPECT_EQ(syncFolder.GetSyncFolderSize(), 1);
        
        uint32_t notExistsIndex = 999;
        syncFolder.DeleteSyncFolder(notExistsIndex);
        EXPECT_EQ(syncFolder.GetSyncFolderSize(), 1);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "DeleteSyncFolderTest002 failed";
    }
    GTEST_LOG_(INFO) << "DeleteSyncFolderTest002 end";
}

/**
 * @tc.name: PathToPhysicalPathTest004
 * @tc.desc: Verify the PathToPhysicalPath function with short input path
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, PathToPhysicalPathTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PathToPhysicalPathTest004 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        string path = "/storage/Users";
        string userId = "100";
        string realpath;
        int32_t ret = syncFolder.PathToPhysicalPath(path, userId, realpath);
        EXPECT_EQ(ret, E_INVALID_ARG);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "PathToPhysicalPathTest004 failed";
    }
    GTEST_LOG_(INFO) << "PathToPhysicalPathTest004 end";
}

/**
 * @tc.name: PathToMntPathBySandboxPathTest004
 * @tc.desc: Verify the PathToMntPathBySandboxPath function with short input path
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, PathToMntPathBySandboxPathTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PathToMntPathBySandboxPathTest004 start";
    try {
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        string path = "/storage/Users";
        string userId = "100";
        string realpath;
        int32_t ret = syncFolder.PathToMntPathBySandboxPath(path, userId, realpath);
        EXPECT_EQ(ret, E_INVALID_ARG);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "PathToMntPathBySandboxPathTest004 failed";
    }
    GTEST_LOG_(INFO) << "PathToMntPathBySandboxPathTest004 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest001
 * @tc.desc: Verify a missing cleanup root reports a failure without touching anything.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest001 start";
    try {
        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_cleanup_missing_root/file.txt");
        EXPECT_EQ(stats.failed, 1U);
        EXPECT_NE(stats.firstError, 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest001 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest001 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest002
 * @tc.desc: Verify an unhydrated placeholder is removed through unlinkat.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest002 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        std::system("touch /data/test_tdd_placeholder/placeholder_file.txt");
        ExpectCleanupState(PLACEHOLDER_STATE_UNHYDRATED);

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.removed, 1U);
        EXPECT_FALSE(CleanupPathExists("/data/test_tdd_placeholder/placeholder_file.txt"));
        std::system("rm -rf /data/test_tdd_placeholder");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest002 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest002 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest003
 * @tc.desc: Verify a partially hydrated placeholder is removed through unlinkat.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest003 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        std::system("touch /data/test_tdd_placeholder/hydrating_file.txt");
        ExpectCleanupState(PLACEHOLDER_STATE_PARTIALLY_HYDRATED);

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.removed, 1U);
        EXPECT_FALSE(CleanupPathExists("/data/test_tdd_placeholder/hydrating_file.txt"));
        std::system("rm -rf /data/test_tdd_placeholder");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest003 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest003 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest004
 * @tc.desc: Verify a normal file is kept and its placeholder xattrs are cleared.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest004 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        std::system("touch /data/test_tdd_placeholder/regular_file.txt");
        ExpectCleanupState(PLACEHOLDER_STATE_NONE);

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.kept, 1U);
        EXPECT_EQ(stats.removed, 0U);
        EXPECT_TRUE(CleanupPathExists("/data/test_tdd_placeholder/regular_file.txt"));
        std::system("rm -rf /data/test_tdd_placeholder");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest004 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest004 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest005
 * @tc.desc: Verify a file with missing placeholder state is kept and reported.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest005 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        std::system("touch /data/test_tdd_placeholder/noxattr_file.txt");
        EXPECT_CALL(*insMock, fgetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
            .WillOnce(Invoke([](int, const char *, void *, size_t) {
                errno = ENODATA;
                return static_cast<ssize_t>(-1);
            }));

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.inconsistent, 1U);
        EXPECT_EQ(stats.removed, 0U);
        EXPECT_TRUE(CleanupPathExists("/data/test_tdd_placeholder/noxattr_file.txt"));
        std::system("rm -rf /data/test_tdd_placeholder");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest005 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest005 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest006
 * @tc.desc: Verify placeholders are removed recursively through directory fds.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest006 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        std::system("mkdir -p /data/test_tdd_placeholder/subdir");
        std::system("touch /data/test_tdd_placeholder/placeholder_root.txt");
        std::system("touch /data/test_tdd_placeholder/subdir/placeholder_sub.txt");
        ExpectCleanupState(PLACEHOLDER_STATE_UNHYDRATED);
        ExpectCleanupState(PLACEHOLDER_STATE_UNHYDRATED);

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.removed, 2U);
        EXPECT_FALSE(CleanupPathExists("/data/test_tdd_placeholder/placeholder_root.txt"));
        EXPECT_FALSE(CleanupPathExists("/data/test_tdd_placeholder/subdir/placeholder_sub.txt"));
        std::system("rm -rf /data/test_tdd_placeholder");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest006 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest006 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest007
 * @tc.desc: Verify a state read failure keeps the file and reports the error.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest007, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest007 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        std::system("touch /data/test_tdd_placeholder/error_file.txt");
        EXPECT_CALL(*insMock, fgetxattr(_, StrEq(CLOUD_DISK_FILE_SYNC_STATE_XATTR), _, sizeof(uint8_t)))
            .WillOnce(Invoke([](int, const char *, void *, size_t) {
                errno = EACCES;
                return static_cast<ssize_t>(-1);
            }));

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.failed, 1U);
        EXPECT_EQ(stats.firstError, EACCES);
        EXPECT_TRUE(CleanupPathExists("/data/test_tdd_placeholder/error_file.txt"));
        std::system("rm -rf /data/test_tdd_placeholder");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest007 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest007 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest008
 * @tc.desc: Verify an unlinkat failure keeps the incomplete placeholder and reports the error.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest008, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest008 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        std::system("touch /data/test_tdd_placeholder/unlink_fail.txt");
        ExpectCleanupState(PLACEHOLDER_STATE_UNHYDRATED);
        EXPECT_CALL(*syncFolderMock, unlinkat(_, _, _)).WillOnce(Invoke([](int, const char *, int) {
            errno = EACCES;
            return -1;
        }));

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.failed, 1U);
        EXPECT_EQ(stats.firstError, EACCES);
        EXPECT_TRUE(CleanupPathExists("/data/test_tdd_placeholder/unlink_fail.txt"));
        std::system("rm -rf /data/test_tdd_placeholder");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest008 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest008 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest009
 * @tc.desc: Verify a symlink is skipped and its target is never removed.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest009, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest009 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        ResetCleanupDir("/data/test_tdd_placeholder_outside");
        std::system("touch /data/test_tdd_placeholder_outside/target.txt");
        std::system("ln -sfn /data/test_tdd_placeholder_outside "
                    "/data/test_tdd_placeholder/outside_link");

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.skippedLinks, 1U);
        EXPECT_EQ(stats.removed, 0U);
        EXPECT_TRUE(CleanupPathExists("/data/test_tdd_placeholder_outside/target.txt"));
        std::system("rm -rf /data/test_tdd_placeholder /data/test_tdd_placeholder_outside");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest009 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest009 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest010
 * @tc.desc: Verify mixed placeholder and normal files keep the expected removal counts.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest010, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest010 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        std::system("touch /data/test_tdd_placeholder/placeholder1.txt");
        std::system("touch /data/test_tdd_placeholder/placeholder2.txt");
        std::system("touch /data/test_tdd_placeholder/regular.txt");
        ExpectCleanupState(PLACEHOLDER_STATE_UNHYDRATED);
        ExpectCleanupState(PLACEHOLDER_STATE_UNHYDRATED);
        ExpectCleanupState(PLACEHOLDER_STATE_NONE);

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.removed, 2U);
        EXPECT_EQ(stats.kept, 1U);
        std::system("rm -rf /data/test_tdd_placeholder");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest010 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest010 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest011
 * @tc.desc: Verify a dangling symlink is skipped while a real placeholder is still cleaned.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest011, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest011 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        std::system("ln -sfn /data/test_tdd_placeholder_missing "
                    "/data/test_tdd_placeholder/dangling_link");
        std::system("touch /data/test_tdd_placeholder/placeholder_file.txt");
        ExpectCleanupState(PLACEHOLDER_STATE_UNHYDRATED);

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.skippedLinks, 1U);
        EXPECT_EQ(stats.removed, 1U);
        EXPECT_FALSE(CleanupPathExists("/data/test_tdd_placeholder/placeholder_file.txt"));
        std::system("rm -rf /data/test_tdd_placeholder");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest011 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest011 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest012
 * @tc.desc: Verify a FIFO entry is kept without being opened.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest012, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest012 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        std::system("mkfifo /data/test_tdd_placeholder/fifo_file");

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.kept, 1U);
        EXPECT_EQ(stats.removed, 0U);
        EXPECT_TRUE(CleanupPathExists("/data/test_tdd_placeholder/fifo_file"));
        std::system("rm -rf /data/test_tdd_placeholder");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest012 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest012 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest_RootSymlink_001
 * @tc.desc: Verify a symlink used as the cleanup root is skipped.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest_RootSymlink_001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_RootSymlink_001 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder_real");
        std::system("touch /data/test_tdd_placeholder_real/file.txt");
        std::system("ln -sfn /data/test_tdd_placeholder_real /data/test_tdd_placeholder_link");

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder_link");
        EXPECT_EQ(stats.skippedLinks, 1U);
        EXPECT_EQ(stats.removed, 0U);
        EXPECT_TRUE(CleanupPathExists("/data/test_tdd_placeholder_real/file.txt"));
        std::system("rm -rf /data/test_tdd_placeholder_real /data/test_tdd_placeholder_link");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_RootSymlink_001 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_RootSymlink_001 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest_RootSymlinkComponent_001
 * @tc.desc: Verify a symlink in a root path component rejects the whole cleanup.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest_RootSymlinkComponent_001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_RootSymlinkComponent_001 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder_real/subdir");
        std::system("touch /data/test_tdd_placeholder_real/subdir/file.txt");
        std::system("ln -sfn /data/test_tdd_placeholder_real /data/test_tdd_placeholder_link");

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder_link/subdir");
        EXPECT_EQ(stats.skippedLinks, 1U);
        EXPECT_EQ(stats.failed, 0U);
        EXPECT_TRUE(CleanupPathExists("/data/test_tdd_placeholder_real/subdir/file.txt"));
        std::system("rm -rf /data/test_tdd_placeholder_real /data/test_tdd_placeholder_link");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_RootSymlinkComponent_001 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_RootSymlinkComponent_001 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest_HardLink_001
 * @tc.desc: Verify a hard-linked file is kept and its xattrs are not touched.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest_HardLink_001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_HardLink_001 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        ResetCleanupDir("/data/test_tdd_placeholder_outside");
        std::system("touch /data/test_tdd_placeholder_outside/target.txt");
        std::system("ln /data/test_tdd_placeholder_outside/target.txt "
                    "/data/test_tdd_placeholder/hard_link.txt");

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.hardLinks, 1U);
        EXPECT_EQ(stats.removed, 0U);
        EXPECT_TRUE(CleanupPathExists("/data/test_tdd_placeholder/hard_link.txt"));
        EXPECT_TRUE(CleanupPathExists("/data/test_tdd_placeholder_outside/target.txt"));
        std::system("rm -rf /data/test_tdd_placeholder /data/test_tdd_placeholder_outside");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_HardLink_001 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_HardLink_001 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest_XattrFailure_001
 * @tc.desc: Keep a fully hydrated file and report file and directory xattr removal failures.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest_XattrFailure_001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_XattrFailure_001 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        std::system("touch /data/test_tdd_placeholder/fully_file.txt");
        ExpectCleanupState(PLACEHOLDER_STATE_FULLY_HYDRATED);
        EXPECT_CALL(*insMock, fremovexattr(_, _)).Times(4).WillRepeatedly(Invoke([](int, const char *) {
            errno = EACCES;
            return -1;
        }));

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.kept, 1U);
        EXPECT_EQ(stats.removed, 0U);
        EXPECT_EQ(stats.failed, 4U);
        EXPECT_EQ(stats.firstError, EACCES);
        EXPECT_TRUE(CleanupPathExists("/data/test_tdd_placeholder/fully_file.txt"));
        std::system("rm -rf /data/test_tdd_placeholder");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_XattrFailure_001 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_XattrFailure_001 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest_FullyHydrated_001
 * @tc.desc: Verify the RemovePlaceholderFilesBatch function keeps FULLY_HYDRATED placeholder files
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest_FullyHydrated_001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_FullyHydrated_001 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        std::system("touch /data/test_tdd_placeholder/fully_file.txt");
        ExpectCleanupState(PLACEHOLDER_STATE_FULLY_HYDRATED);
        EXPECT_CALL(*insMock, fremovexattr(_, _)).WillRepeatedly(Return(0));

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.kept, 1U);
        EXPECT_EQ(stats.removed, 0U);
        EXPECT_TRUE(CleanupPathExists("/data/test_tdd_placeholder/fully_file.txt"));
        std::system("rm -rf /data/test_tdd_placeholder");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_FullyHydrated_001 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_FullyHydrated_001 end";
}

/**
 * @tc.name: RemovePlaceholderFilesBatchTest_FullyHydrated_NoUnlink_002
 * @tc.desc: Verify the RemovePlaceholderFilesBatch function does not unlink FULLY_HYDRATED files
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskSyncFolderTest, RemovePlaceholderFilesBatchTest_FullyHydrated_NoUnlink_002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_FullyHydrated_NoUnlink_002 start";
    try {
        ResetCleanupDir("/data/test_tdd_placeholder");
        std::system("touch /data/test_tdd_placeholder/fully_file.txt");
        ExpectCleanupState(PLACEHOLDER_STATE_FULLY_HYDRATED);
        EXPECT_CALL(*insMock, fremovexattr(_, _)).WillRepeatedly(Return(0));

        PlaceholderCleanupStats stats = CloudDiskSyncFolder::GetInstance().RemovePlaceholderFilesBatch(
            "/data/test_tdd_placeholder");
        EXPECT_EQ(stats.kept, 1U);
        EXPECT_TRUE(CleanupPathExists("/data/test_tdd_placeholder/fully_file.txt"));
        std::system("rm -rf /data/test_tdd_placeholder");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_FullyHydrated_NoUnlink_002 failed";
    }
    GTEST_LOG_(INFO) << "RemovePlaceholderFilesBatchTest_FullyHydrated_NoUnlink_002 end";
}
} // namespace Test
} // namespace FileManagement::CloudDiskService
} // namespace OHOS
