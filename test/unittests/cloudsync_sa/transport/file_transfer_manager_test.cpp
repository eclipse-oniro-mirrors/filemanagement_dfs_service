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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "cloud_sync_common.h"
#include "dfs_error.h"
#include "file_transfer_manager.h"
#include "softbus_adapter_mock.h"
#include "softbus_session_mock.h"
#include "session_manager.h"
#include "socket_mock.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement::CloudSync {
namespace Test {
using namespace testing;
using namespace testing::ext;
using namespace std;

class FileTransferManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void FileTransferManagerTest::SetUpTestCase(void)
{
    std::cout << "SetUpTestCase" << std::endl;
}

void FileTransferManagerTest::TearDownTestCase(void)
{
    std::cout << "TearDownTestCase" << std::endl;
}

void FileTransferManagerTest::SetUp(void)
{
    std::cout << "SetUp" << std::endl;
}

void FileTransferManagerTest::TearDown(void)
{
    std::cout << "TearDown" << std::endl;
}

/**
 * @tc.name: InitTest001
 * @tc.desc: Verify the Init function
 * @tc.type: FUNC
 * @tc.require: IB3T5H
 */
HWTEST_F(FileTransferManagerTest, InitTest001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "InitTest001 start";
    try {
        auto sessionManager = make_shared<SessionManager>();
        auto fileTransferManager = make_shared<FileTransferManager>(sessionManager);
        fileTransferManager->Init();
        fileTransferManager->OnSessionClosed();
        EXPECT_NE(fileTransferManager->sessionManager_, nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "InitTest001 failed";
    }
    GTEST_LOG_(INFO) << "InitTest001 end";
}

/**
 * @tc.name: DownloadFileFromRemoteDeviceTest001
 * @tc.desc: Verify the DownloadFileFromRemoteDevice function
 * @tc.type: FUNC
 * @tc.require: IB3T5H
 */
HWTEST_F(FileTransferManagerTest, DownloadFileFromRemoteDeviceTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DownloadFileFromRemoteDeviceTest001 start";
    try {
        auto sessionManager = make_shared<SessionManager>();
        char data[] = "test data";
        string peerNetworkId = "test peerNetworkId";
        auto fileTransferManager = make_shared<FileTransferManager>(sessionManager);
        fileTransferManager->DownloadFileFromRemoteDevice(peerNetworkId, 0, 0, "data/test");
        EXPECT_NE(fileTransferManager->sessionManager_, nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "DownloadFileFromRemoteDeviceTest001 failed";
    }
    GTEST_LOG_(INFO) << "DownloadFileFromRemoteDeviceTest001 end";
}

/**
 * @tc.name: DownloadFileFromRemoteDeviceTest002
 * @tc.desc: bundleName is not empty
 * @tc.type: FUNC
 * @tc.require: IB3T5H
 */
HWTEST_F(FileTransferManagerTest, DownloadFileFromRemoteDeviceTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DownloadFileFromRemoteDeviceTest002 start";
    try {
        auto sessionManager = make_shared<SessionManager>();
        char data[] = "test data";
        string peerNetworkId = "test peerNetworkId";
        std::string uri = "file://com.ohos.a/data/storage/el2/distrubutedfiles/1.txt";
        auto fileTransferManager = make_shared<FileTransferManager>(sessionManager);
        fileTransferManager->DownloadFileFromRemoteDevice(peerNetworkId, 0, 0, uri);
        EXPECT_NE(fileTransferManager->sessionManager_, nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "DownloadFileFromRemoteDeviceTest002 failed";
    }
    GTEST_LOG_(INFO) << "DownloadFileFromRemoteDeviceTest002 end";
}

/**
 * @tc.name: GetBundleNameForUriTest001
 * @tc.desc: bundleName is not empty
 * @tc.type: FUNC
 * @tc.require: #NA
 */
HWTEST_F(FileTransferManagerTest, GetBundleNameForUriTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetBundleNameForUriTest001 start";
    try {
        auto sessionManager = make_shared<SessionManager>();
        std::string uri = "file://com.ohos.a/data/storage/el2/distrubutedfiles/1.txt";
        auto fileTransferManager = make_shared<FileTransferManager>(sessionManager);
        auto bundleName = fileTransferManager->GetBundleNameForUri(uri);
        EXPECT_EQ(bundleName, "com.ohos.a");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetBundleNameForUriTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetBundleNameForUriTest001 end";
}

/**
 * @tc.name: GetBundleNameForUriTest002
 * @tc.desc: bundleName is empty
 * @tc.type: FUNC
 * @tc.require: #NA
 */
HWTEST_F(FileTransferManagerTest, GetBundleNameForUriTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetBundleNameForUriTest002 start";
    try {
        auto sessionManager = make_shared<SessionManager>();
        std::string uri = "data/";
        auto fileTransferManager = make_shared<FileTransferManager>(sessionManager);
        auto bundleName = fileTransferManager->GetBundleNameForUri(uri);
        EXPECT_TRUE(bundleName.empty());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetBundleNameForUriTest002 failed";
    }
    GTEST_LOG_(INFO) << "GetBundleNameForUriTest002 end";
}

/**
 * @tc.name: GetBundleNameForUriTest003
 * @tc.desc: bundleName is empty
 * @tc.type: FUNC
 * @tc.require: #NA
 */
HWTEST_F(FileTransferManagerTest, GetBundleNameForUriTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetBundleNameForUriTest003 start";
    try {
        auto sessionManager = make_shared<SessionManager>();
        std::string uri = "file://com.ohos.a";
        auto fileTransferManager = make_shared<FileTransferManager>(sessionManager);
        auto bundleName = fileTransferManager->GetBundleNameForUri(uri);
        EXPECT_TRUE(bundleName.empty());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetBundleNameForUriTest003 failed";
    }
    GTEST_LOG_(INFO) << "GetBundleNameForUriTest003 end";
}
/**
 * @tc.name: OnMessageHandleTest001
 * @tc.desc: Verify the OnMessageHandle function
 * @tc.type: FUNC
 * @tc.require: IB3T5H
 */
HWTEST_F(FileTransferManagerTest, OnMessageHandleTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnMessageHandleTest001 start";
    try {
        auto sessionManager = make_shared<SessionManager>();
        char data[] = "test data";
        string peerNetworkId = "test peerNetworkId";
        auto fileTransferManager = make_shared<FileTransferManager>(sessionManager);
        fileTransferManager->OnMessageHandle(peerNetworkId, 0, data, sizeof(data));
        EXPECT_NE(fileTransferManager->sessionManager_, nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnMessageHandleTest001 failed";
    }
    GTEST_LOG_(INFO) << "OnMessageHandleTest001 end";
}

/**
 * @tc.name: HandleDownloadFileRequestTest001
 * @tc.desc: Verify the HandleDownloadFileRequest function
 * @tc.type: FUNC
 * @tc.require: IB3T5H
 */
HWTEST_F(FileTransferManagerTest, HandleDownloadFileRequestTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleDownloadFileRequestTest001 start";
    try {
        auto sessionManager = make_shared<SessionManager>();
        char data[] = "test data";
        string peerNetworkId = "test peerNetworkId";
        auto fileTransferManager = make_shared<FileTransferManager>(sessionManager);

        MessageInputInfo msgInputInfo = {};
        msgInputInfo.userId = 100;
        msgInputInfo.taskId = 100;
        msgInputInfo.uri = "data/test";
        MessageHandler msgHandle(msgInputInfo);
        fileTransferManager->HandleDownloadFileRequest(msgHandle, peerNetworkId, 0);
        fileTransferManager->HandleDownloadFileResponse(msgHandle);
        fileTransferManager->HandleRecvFileFinished();
        EXPECT_NE(fileTransferManager->sessionManager_, nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "HandleDownloadFileRequestTest001 failed";
    }
    GTEST_LOG_(INFO) << "HandleDownloadFileRequestTest001 end";
}

/**
 * @tc.name: IsFileExistsTest001
 * @tc.desc: Verify the IsFileExists function
 * @tc.type: FUNC
 * @tc.require: IB3T5H
 */
HWTEST_F(FileTransferManagerTest, IsFileExistsTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsFileExistsTest001 start";
    try {
        auto sessionManager = make_shared<SessionManager>();
        char data[] = "test data";
        string peerNetworkId = "test peerNetworkId";
        auto fileTransferManager = make_shared<FileTransferManager>(sessionManager);
        EXPECT_TRUE(fileTransferManager->IsFileExists("data/errorPath") == false);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "IsFileExistsTest001 failed";
    }
    GTEST_LOG_(INFO) << "IsFileExistsTest001 end";
}

/**
 * @tc.name: AddTransTaskTest001
 * @tc.desc: Verify the AddTransTask function
 * @tc.type: FUNC
 * @tc.require: IB3T5H
 */
HWTEST_F(FileTransferManagerTest, AddTransTaskTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AddTransTaskTest001 start";
    try {
        auto sessionManager = make_shared<SessionManager>();
        char data[] = "test data";
        string peerNetworkId = "test peerNetworkId";
        auto fileTransferManager = make_shared<FileTransferManager>(sessionManager);
        fileTransferManager->AddTransTask("data/test", 0, 100);
        fileTransferManager->FinishTransTask("data/test", 0);
        fileTransferManager->RemoveTransTask(100);
        EXPECT_NE(fileTransferManager->sessionManager_, nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AddTransTaskTest001 failed";
    }
    GTEST_LOG_(INFO) << "AddTransTaskTest001 end";
}

/**
 * @tc.name: UriToPathTest001
 * @tc.desc: Invalid uri
 * @tc.type: FUNC
 * @tc.require: IB3T5H
 */
HWTEST_F(FileTransferManagerTest, UriToPathTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UriToPathTest001 start";
    try {
        auto sessionManager = make_shared<SessionManager>();
        auto fileTransferManager = make_shared<FileTransferManager>(sessionManager);
        auto [first, second] = fileTransferManager->UriToPath("data/test", 100, false);
        EXPECT_TRUE(first.empty());
        EXPECT_TRUE(second.empty());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UriToPathTest001 failed";
    }
    GTEST_LOG_(INFO) << "UriToPathTest001 end";
}

/**
 * @tc.name: UriToPathTest002
 * @tc.desc: Verify the UriToPath function
 * @tc.type: FUNC
 * @tc.require: IB3T5H
 */
HWTEST_F(FileTransferManagerTest, UriToPathTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UriToPathTest002 start";
    try {
        auto sessionManager = make_shared<SessionManager>();
        auto fileTransferManager = make_shared<FileTransferManager>(sessionManager);
        std::string uri = "file://com.ohos.a/data/storage/el2/distributedfiles/1.txt";
        std::string relativePath = "/1.txt";
        auto [first, second] = fileTransferManager->UriToPath(uri, 100, false);
        EXPECT_EQ(second, relativePath);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UriToPathTest002 failed";
    }
    GTEST_LOG_(INFO) << "UriToPathTest002 end";
}
} // namespace Test
} // namespace CloudSync
} // namespace OHOS