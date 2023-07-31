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

#include "ipc/distributed_file_daemon_ipc_interface_code.h"
#include "ipc/daemon_stub.h"
#include "utils_log.h"

namespace OHOS::Storage::DistributedFile::Test {
using namespace testing;
using namespace testing::ext;
using namespace std;

namespace {
    DistributedHardware::DmDeviceInfo deviceInfo = {
        .deviceId = "testdevid",
        .deviceName = "testdevname",
        .networkId = "testnetworkid",
    };
}

class MockDaemonStub : public DaemonStub {
public:
    MOCK_METHOD1(OpenP2PConnection, int32_t(const DistributedHardware::DmDeviceInfo &deviceInfo));
    MOCK_METHOD1(CloseP2PConnection, int32_t(const DistributedHardware::DmDeviceInfo &deviceInfo));
};

class DaemonStubTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    std::shared_ptr<DaemonStub> daemonStub_;
};

void DaemonStubTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
}

void DaemonStubTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
}

void DaemonStubTest::SetUp(void)
{
    daemonStub_ = std::make_shared<MockDaemonStub>();
    GTEST_LOG_(INFO) << "SetUp";
}

void DaemonStubTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "TearDown";
}

/**
 * @tc.name: DaemonStubOnRemoteRequestTest001
 * @tc.desc: Verify the OnRemoteRequest function
 * @tc.type: FUNC
 * @tc.require: I7M6L1
 */
HWTEST_F(DaemonStubTest, DaemonStubOnRemoteRequestTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest001 Start";
    try {
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;
        int ret = daemonStub_->OnRemoteRequest(
            static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_OPEN_P2P_CONNECTION),
            data, reply, option);
        EXPECT_EQ(ret, IDaemon::DFS_DAEMON_DESCRIPTOR_IS_EMPTY);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest001  ERROR";
    }
    GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest001 End";
}

/**
 * @tc.name: DaemonStubOnRemoteRequestTest002
 * @tc.desc: Verify the OnRemoteRequest function
 * @tc.type: FUNC
 * @tc.require: I7M6L1
 */
HWTEST_F(DaemonStubTest, DaemonStubOnRemoteRequestTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest002 Start";
    try {
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;
        int ret = daemonStub_->OnRemoteRequest(
            static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_CLOSE_P2P_CONNECTION),
            data, reply, option);
        EXPECT_EQ(ret, IDaemon::DFS_DAEMON_DESCRIPTOR_IS_EMPTY);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest002  ERROR";
    }
    GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest002 End";
}

/**
 * @tc.name: DaemonStubOnRemoteRequestTest003
 * @tc.desc: Verify the OnRemoteRequest function
 * @tc.type: FUNC
 * @tc.require: I7M6L1
 */
HWTEST_F(DaemonStubTest, DaemonStubOnRemoteRequestTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest003 Start";
    try {
        int32_t code = 1000;
        MessageParcel data;
        EXPECT_TRUE(data.WriteInterfaceToken(IDaemon::GetDescriptor()));
        MessageParcel reply;
        MessageOption option;
        int ret = daemonStub_->OnRemoteRequest(code, data, reply, option);
        EXPECT_NE(ret, 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest003  ERROR";
    }
    GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest003 End";
}

/**
 * @tc.name: DaemonStubOnRemoteRequestTest004
 * @tc.desc: Verify the DaemonStubOnRemoteRequest function
 * @tc.type: FUNC
 * @tc.require: I7M6L1
 */
HWTEST_F(DaemonStubTest, DaemonStubOnRemoteRequestTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest004 Start";
    try {
        MessageParcel data;
        EXPECT_TRUE(data.WriteInterfaceToken(IDaemon::GetDescriptor()));
        MessageParcel reply;
        MessageOption option;

        EXPECT_TRUE(data.WriteCString(deviceInfo.deviceId));
        EXPECT_TRUE(data.WriteCString(deviceInfo.deviceName));
        EXPECT_TRUE(data.WriteCString(deviceInfo.networkId));
        
        int ret = daemonStub_->OnRemoteRequest(
            static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_OPEN_P2P_CONNECTION),
            data, reply, option);
        EXPECT_EQ(ret, 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest004  ERROR";
    }
    GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest004 End";
}

/**
 * @tc.name: DaemonStubOnRemoteRequestTest005
 * @tc.desc: Verify the DaemonStubOnRemoteRequest function
 * @tc.type: FUNC
 * @tc.require: I7M6L1
 */
HWTEST_F(DaemonStubTest, DaemonStubOnRemoteRequestTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest005 Start";
    try {
        MessageParcel data;
        EXPECT_TRUE(data.WriteInterfaceToken(IDaemon::GetDescriptor()));
        MessageParcel reply;
        MessageOption option;

        EXPECT_TRUE(data.WriteCString(deviceInfo.deviceId));
        EXPECT_TRUE(data.WriteCString(deviceInfo.deviceName));
        EXPECT_TRUE(data.WriteCString(deviceInfo.networkId));

        int ret = daemonStub_->OnRemoteRequest(
            static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_CLOSE_P2P_CONNECTION),
            data, reply, option);
        EXPECT_EQ(ret, 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest005  ERROR";
    }
    GTEST_LOG_(INFO) << "DaemonStubOnRemoteRequestTest005 End";
}
} // namespace OHOS::Storage::DistributedFile::Test