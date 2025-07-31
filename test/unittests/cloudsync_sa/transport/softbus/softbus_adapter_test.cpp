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
#include "softbus_adapter.h"
#include "session_manager.h"
#include "softbus_session.h"
#include "socket_mock.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement::CloudSync {
namespace Test {
using namespace testing;
using namespace testing::ext;
using namespace std;

class SoftbusAdapterTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static inline shared_ptr<Storage::DistributedFile::SocketMock> socketMock_ = nullptr;
};

void SoftbusAdapterTest::SetUpTestCase(void)
{
    std::cout << "SetUpTestCase" << std::endl;
    socketMock_ = make_shared<Storage::DistributedFile::SocketMock>();
    Storage::DistributedFile::SocketMock::dfsSocket = socketMock_;
}

void SoftbusAdapterTest::TearDownTestCase(void)
{
    std::cout << "TearDownTestCase" << std::endl;
    Storage::DistributedFile::SocketMock::dfsSocket = nullptr;
    socketMock_ = nullptr;
}

void SoftbusAdapterTest::SetUp(void)
{
    std::cout << "SetUp" << std::endl;
}

void SoftbusAdapterTest::TearDown(void)
{
    std::cout << "TearDown" << std::endl;
}

/**
 * @tc.name: CreateSessionServerTest001
 * @tc.desc: Verify the CreateSessionServer function
 * @tc.type: FUNC
 * @tc.require: IB3T80
 */
HWTEST_F(SoftbusAdapterTest, CreateSessionServerTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "CreateSessionServerTest001 start";
    try {
        SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();
        char packageName[] = "com.example.test";
        char sessionName[] = "testSession";
        EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(-1));
        int32_t result = adapter.CreateSessionServer(packageName, sessionName);
        EXPECT_EQ(result, ERR_BAD_VALUE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "CreateSessionServerTest001 failed";
    }
    GTEST_LOG_(INFO) << "CreateSessionServerTest001 end";
}

/**
 * @tc.name: CreateSessionServerTest002
 * @tc.desc: Verify the CreateSessionServer function
 * @tc.type: FUNC
 * @tc.require: IB3T80
 */
HWTEST_F(SoftbusAdapterTest, CreateSessionServerTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "CreateSessionServerTest002 start";
    try {
        SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();
        char packageName[] = "com.example.test";
        char sessionName[] = "testSession";

        EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(1));
        EXPECT_CALL(*socketMock_, Listen(_, _, _, _)).WillOnce(Return(0));
        int32_t result = adapter.CreateSessionServer(packageName, sessionName);
        EXPECT_EQ(result, E_OK);
        adapter.RemoveSessionServer(packageName, sessionName);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "CreateSessionServerTest002 failed";
    }
    GTEST_LOG_(INFO) << "CreateSessionServerTest002 end";
}

/**
 * @tc.name: CreateSessionServerTest003
 * @tc.desc: Verify the CreateSessionServer function
 * @tc.type: FUNC
 * @tc.require: IB3T80
 */
HWTEST_F(SoftbusAdapterTest, CreateSessionServerTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "CreateSessionServerTest003 start";
    try {
        SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();
        char packageName[] = "com.example.test";
        char sessionName[] = "testSession";

        EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(1));
        EXPECT_CALL(*socketMock_, Listen(_, _, _, _)).WillOnce(Return(1));
        int32_t result = adapter.CreateSessionServer(packageName, sessionName);
        EXPECT_EQ(result, 1);
        adapter.RemoveSessionServer(packageName, sessionName);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "CreateSessionServerTest003 failed";
    }
    GTEST_LOG_(INFO) << "CreateSessionServerTest003 end";
}

/**
 * @tc.name: OpenSessionByP2PTest001
 * @tc.desc: Verify the OpenSessionByP2P function
 * @tc.type: FUNC
 * @tc.require: IB3T80
 */
HWTEST_F(SoftbusAdapterTest, OpenSessionByP2PTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OpenSessionByP2PTest001 start";
    try {
        SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();
        char sessionName[] = "testSession";
        char peerNetworkId[] = "test peerNetworkId";
        char groupId[] = "test";

        EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(-1));
        int socketFd = adapter.OpenSessionByP2P(sessionName, peerNetworkId, groupId, true);
        EXPECT_EQ(socketFd, ERR_BAD_VALUE);

        EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(1));
        EXPECT_CALL(*socketMock_, Bind(_, _, _, _)).WillOnce(Return(0));
        socketFd = adapter.OpenSessionByP2P(sessionName, peerNetworkId, groupId, true);
        EXPECT_EQ(socketFd, 1);
        CloseSession(socketFd);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OpenSessionByP2PTest001 failed";
    }
    GTEST_LOG_(INFO) << "OpenSessionByP2PTest001 end";
}

/**
 * @tc.name: OpenSessionByP2PTest002
 * @tc.desc: Verify the OpenSessionByP2P function
 * @tc.type: FUNC
 * @tc.require: IB3T80
 */
HWTEST_F(SoftbusAdapterTest, OpenSessionByP2PTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OpenSessionByP2PTest002 start";
    try {
        SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();
        char sessionName[] = "testSession";
        char peerNetworkId[] = "test peerNetworkId";
        char groupId[] = "test";

        EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(-1));
        int socketFd = adapter.OpenSessionByP2P(sessionName, peerNetworkId, groupId, false);
        EXPECT_EQ(socketFd, ERR_BAD_VALUE);

        EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(1));
        EXPECT_CALL(*socketMock_, Bind(_, _, _, _)).WillOnce(Return(0));
        socketFd = adapter.OpenSessionByP2P(sessionName, peerNetworkId, groupId, false);
        EXPECT_EQ(socketFd, 1);
        CloseSession(socketFd);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OpenSessionByP2PTest002 failed";
    }
    GTEST_LOG_(INFO) << "OpenSessionByP2PTest002 end";
}

/**
 * @tc.name: EventTest001
 * @tc.desc: Verify the OpenSessionByP2P function
 * @tc.type: FUNC
 * @tc.require: IB3T80
 */
HWTEST_F(SoftbusAdapterTest, EventTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "EventTest001 start";
    try {
        SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();
        char sessionName[] = "testSession";
        char peerNetworkId[] = "test peerNetworkId";
        char groupId[] = "test";
        PeerSocketInfo info;
        info.name = sessionName;
        info.networkId = peerNetworkId;

        EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(1));
        EXPECT_CALL(*socketMock_, Bind(_, _, _, _)).WillOnce(Return(0));
        int socketFd = adapter.OpenSessionByP2P(sessionName, peerNetworkId, groupId, false);
        // OnBind
        SoftbusAdapter::OnBind(socketFd, info);
        // OnShutdown
        SoftbusAdapter::OnShutdown(socketFd, SHUTDOWN_REASON_UNKNOWN);
        // OnBytes
        char data[] = "test data";
        SoftbusAdapter::OnBytes(socketFd, data, sizeof(data));
        // OnFile
        FileEvent event;
        event.type = FileEventType::FILE_EVENT_RECV_UPDATE_PATH;
        SoftbusAdapter::OnFile(socketFd, &event);
        // OnReceiveFileProcess
        int32_t result = SoftbusAdapter::OnReceiveFileProcess(socketFd, "", 0, 0);
        EXPECT_EQ(result, 0);
        // OnReceiveFileFinished
        SoftbusAdapter::OnReceiveFileFinished(socketFd, "", 0);
        SoftbusAdapter::pathDir_ = "/mnt/hmdfs/100/account/device_view/local/data/com.ohos.a";
        std::string res = std::string(SoftbusAdapter::GetRecvPath());
        // GetRecvPath
        EXPECT_EQ(res, "/mnt/hmdfs/100/account/device_view/local/data/com.ohos.a");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "EventTest001 failed";
    }
    GTEST_LOG_(INFO) << "EventTest001 end";
}

/**
 * @tc.name: SendBytesTest001
 * @tc.desc: Verify the OpenSessionByP2P function
 * @tc.type: FUNC
 * @tc.require: IB3T80
 */
HWTEST_F(SoftbusAdapterTest, SendBytesTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SendBytesTest001 start";
    try {
        SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();
        char sessionName[] = "testSession";
        char peerNetworkId[] = "test peerNetworkId";
        char groupId[] = "test";
        char data[] = "test data";

        EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(1));
        EXPECT_CALL(*socketMock_, Bind(_, _, _, _)).WillOnce(Return(0));
        int socketFd = adapter.OpenSessionByP2P(sessionName, peerNetworkId, groupId, false);
        int result = adapter.SendBytes(socketFd, data, sizeof(data));
#ifdef CLOUD_ADAPTER_ENABLED
        EXPECT_NE(result, 0);
#else
        EXPECT_EQ(result, 0);
#endif
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SendBytesTest001 failed";
    }
    GTEST_LOG_(INFO) << "SendBytesTest001 end";
}

/**
 * @tc.name: SendFileTest001
 * @tc.desc: Verify the OpenSessionByP2P function
 * @tc.type: FUNC
 * @tc.require: IB3T80
 */
HWTEST_F(SoftbusAdapterTest, SendFileTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SendFileTest001 start";
    try {
        SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();
        char sessionName[] = "testSession";
        char peerNetworkId[] = "test peerNetworkId";
        char groupId[] = "test";
        char data[] = "test data";

        EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(1));
        EXPECT_CALL(*socketMock_, Bind(_, _, _, _)).WillOnce(Return(0));
        int socketFd = adapter.OpenSessionByP2P(sessionName, peerNetworkId, groupId, false);
        int result = adapter.SendFile(socketFd, {"data/test"}, {"data/test"});
        EXPECT_EQ(result, 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SendFileTest001 failed";
    }
    GTEST_LOG_(INFO) << "SendFileTest001 end";
}

/**
 * @tc.name: GetSessionNameTest001
 * @tc.desc: Verify the OpenSessionByP2P function
 * @tc.type: FUNC
 * @tc.require: IB3T80
 */
HWTEST_F(SoftbusAdapterTest, GetSessionNameTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSessionNameTest001 start";
    try {
        SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();
        char sessionName[] = "testSession";
        char peerNetworkId[] = "test peerNetworkId";
        char groupId[] = "test";
        char data[] = "test data";

        EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(1));
        EXPECT_CALL(*socketMock_, Bind(_, _, _, _)).WillOnce(Return(0));
        int socketFd = adapter.OpenSessionByP2P(sessionName, peerNetworkId, groupId, false);

        EXPECT_EQ(adapter.GetSessionNameFromMap(socketFd), "testSession");
        EXPECT_EQ(adapter.GetPeerNetworkId(socketFd), "test peerNetworkId");
        EXPECT_EQ(adapter.GetSocketNameFromMap("testSession"), -1);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSessionNameTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetSessionNameTest001 end";
}

/**
 * @tc.name: RegisterSessionTest001
 * @tc.desc: Verify the OpenSessionByP2P function
 * @tc.type: FUNC
 * @tc.require: IB3T80
 */
HWTEST_F(SoftbusAdapterTest, RegisterSessionTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RegisterSessionTest001 start";
    try {
        SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();
        char sessionName[] = "testSession";
        char peerNetworkId[] = "test peerNetworkId";
        char groupId[] = "test";
        char data[] = "test data";

        EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(1));
        EXPECT_CALL(*socketMock_, Bind(_, _, _, _)).WillOnce(Return(0));
        int socketFd = adapter.OpenSessionByP2P(sessionName, peerNetworkId, groupId, false);

        adapter.RegisterSessionListener(sessionName, nullptr);
        adapter.UnRegisterSessionListener(sessionName);
        EXPECT_EQ(adapter.IsSessionOpened(socketFd), true);

        adapter.AcceptSesion(socketFd, sessionName, peerNetworkId);
        adapter.RemoveSesion(socketFd);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RegisterSessionTest001 failed";
    }
    GTEST_LOG_(INFO) << "RegisterSessionTest001 end";
}

HWTEST_F(SoftbusAdapterTest, RemoveSessionServerTest001, TestSize.Level1)
{
    SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();

    char packageName[] = "";
    char sessionName[] = "";

    int32_t res = adapter.RemoveSessionServer(packageName, sessionName);
    EXPECT_EQ(res, E_OK);
}

HWTEST_F(SoftbusAdapterTest, OnBindTest001, TestSize.Level1)
{
    SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();

    int socket = 1;
    char sessionName[] = "";
    char peerNetworkId[] = "test peerNetworkId";
    PeerSocketInfo info;
    info.name = sessionName;
    info.networkId = peerNetworkId;

    adapter.OnBind(socket, info);
    EXPECT_EQ(info.name, sessionName);

    auto ptr = std::make_shared<SessionManager>();
    (adapter.listeners_)["socket"] = ptr;
    char socket2[] = "socket";
    info.name = socket2;
    adapter.OnBind(socket, info);
    EXPECT_NE(adapter.listeners_.count("socket"), 0);
    adapter.listeners_.erase("socket");
}

HWTEST_F(SoftbusAdapterTest, OnShutdownTest, TestSize.Level1)
{
    SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();

    int32_t socket = 0;
    ShutdownReason reason = ShutdownReason::SHUTDOWN_REASON_UNKNOWN;
    adapter.OnShutdown(socket, reason);

    (adapter.sessionNameMap_)[0] = "test";
    auto ptr = std::make_shared<SessionManager>();
    (adapter.listeners_)["test"] = ptr;
    adapter.OnShutdown(socket, reason);
    EXPECT_NE(adapter.listeners_.count("test"), 0);
    adapter.listeners_.erase("test");
}

HWTEST_F(SoftbusAdapterTest, OnBytesTest, TestSize.Level1)
{
    SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();

    int socket = 0;
    unsigned int dataLen = 1;
    adapter.OnBytes(socket, nullptr, dataLen);

    (adapter.sessionNameMap_)[0] = "test";
    adapter.OnBytes(socket, nullptr, dataLen);

    (adapter.networkIdMap_)[0] = "test";
    auto ptr = std::make_shared<SessionManager>();
    (adapter.listeners_)["test"] = ptr;
    adapter.OnBytes(socket, nullptr, dataLen);

    EXPECT_NE(adapter.listeners_.count("test"), 0);
    adapter.listeners_.erase("test");
}

HWTEST_F(SoftbusAdapterTest, OnReceiveFileFinishedTest, TestSize.Level1)
{
    SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();

    int sessionId = -1;
    char *files = nullptr;
    int fileCnt = 1;
    adapter.OnReceiveFileFinished(sessionId, files, fileCnt);

    (adapter.sessionNameMap_)[-1] = "test1";
    adapter.OnReceiveFileFinished(sessionId, files, fileCnt);
    
    (adapter.networkIdMap_)[-1] = "test2";
    auto ptr = std::make_shared<SessionManager>();
    (adapter.listeners_)["test1"] = ptr;
    adapter.OnReceiveFileFinished(sessionId, files, fileCnt);

    EXPECT_NE(adapter.listeners_.count("test1"), 0);
    adapter.listeners_.erase("test1");
}

HWTEST_F(SoftbusAdapterTest, OpenSessionTest, TestSize.Level1)
{
    SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();

    TransDataType dataType = TransDataType::DATA_TYPE_MESSAGE;
    char sessionName[] = "";
    char peerDeviceId[] = "";
    char groupId[] = "";
    EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(1));
    EXPECT_CALL(*socketMock_, Bind(_, _, _, _)).WillOnce(Return(1));

    auto res = adapter.OpenSession(sessionName, peerDeviceId, groupId, dataType);
    EXPECT_EQ(res, 1);
}

HWTEST_F(SoftbusAdapterTest, GetPeerNetworkIdTest, TestSize.Level1)
{
    SoftbusAdapter& adapter = SoftbusAdapter::GetInstance();

    TransDataType dataType = TransDataType::DATA_TYPE_MESSAGE;
    char sessionName[] = "";
    char peerDeviceId[] = "";
    char groupId[] = "";
    EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(1));
    EXPECT_CALL(*socketMock_, Bind(_, _, _, _)).WillOnce(Return(1));

    auto res = adapter.OpenSession(sessionName, peerDeviceId, groupId, dataType);
    EXPECT_EQ(res, 1);
}

/**
 * @tc.name: UpdateFileRecvPathTest001
 * @tc.desc: Verify the UpdateFileRecvPath function
 * @tc.type: FUNC
 * @tc.require: #NA
 */
HWTEST_F(SoftbusAdapterTest, UpdateFileRecvPathTest001, TestSize.Level1)
{
    SoftbusAdapter &adapter = SoftbusAdapter::GetInstance();

    std::string bundleName = "com.ohos.a";
    int32_t userId = 100;
    SoftbusAdapter::UpdateFileRecvPath(bundleName, userId);
    std::string res = "/mnt/hmdfs/100/account/device_view/local/data/com.ohos.a";
    EXPECT_EQ(adapter.pathDir_, res);
}
} // namespace Test
} // namespace CloudSync
} // namespace OHOS