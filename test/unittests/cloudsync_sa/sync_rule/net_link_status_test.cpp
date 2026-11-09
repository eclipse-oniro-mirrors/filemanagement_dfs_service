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
#include <cstdint>
#include <unistd.h>

#include "dfs_error.h"
#include "net_link_status.h"
#include "utils_log.h"
#include "link_status_func_mock.h"

namespace OHOS::FileManagement::CloudSync::Test {
using namespace std;
using namespace testing;
using namespace testing::ext;

static int32_t MockGetNetworkStatusSuccess(int &type)
{
    type = NET_NORMAL;
    return 0;
}

static int32_t MockGetNetworkStatusFail(int &type)
{
    return -1;
}

static int32_t MockGetNetworkStatusAirPlane(int &type)
{
    type = WIFI_AIRPLANE;
    return 0;
}

class NetLinkStatusTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

public:
    static inline std::shared_ptr<LibraryFuncMock> libraryFuncMock_ = nullptr;
};

void NetLinkStatusTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "NetLinkStatusTest SetUpTestCase";
    libraryFuncMock_ = std::make_shared<LibraryFuncMock>();
    LibraryFunc::libraryFunc_ = libraryFuncMock_;
}

void NetLinkStatusTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "NetLinkStatusTest TearDownTestCase";
    LibraryFunc::libraryFunc_ = nullptr;
    libraryFuncMock_ = nullptr;
}

void NetLinkStatusTest::SetUp(void)
{
    GTEST_LOG_(INFO) << "SetUp";
    NetworkLinkStatus::netMgrLibHandle_ = nullptr;
    NetworkLinkStatus::getCurrentStatus = nullptr;
}

void NetLinkStatusTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "TearDown";
}

/**
 * @tc.name: InitNetwork001
 * @tc.desc: Verify that InitNetwork handles dlopen failure
 * @tc.type: FUNC
 * @tc.require: IB3SWZ
 */
HWTEST_F(NetLinkStatusTest, InitNetwork001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "InitNetwork001 start";
    try {
        EXPECT_CALL(*libraryFuncMock_, dlopen(_, _)).WillOnce(Return(nullptr));

        NetworkLinkStatus::InitNetwork();
        EXPECT_EQ(NetworkLinkStatus::netMgrLibHandle_, nullptr);
        EXPECT_EQ(NetworkLinkStatus::getCurrentStatus, nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
    }
    GTEST_LOG_(INFO) << "InitNetwork001 end";
}

/**
 * @tc.name: InitNetwork002
 * @tc.desc: Verify that InitNetwork handles dlsym failure
 * @tc.type: FUNC
 * @tc.require: IB3SWZ
 */
HWTEST_F(NetLinkStatusTest, InitNetwork002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "InitNetwork002 start";
    try {
        EXPECT_CALL(*libraryFuncMock_, dlopen(_, _))
            .WillOnce(Return(reinterpret_cast<void *>(0x1)));
        EXPECT_CALL(*libraryFuncMock_, dlsym(_, _))
            .WillOnce(Return(nullptr));
        EXPECT_CALL(*libraryFuncMock_, dlclose(_))
            .WillOnce(Return(0));

        NetworkLinkStatus::InitNetwork();
        EXPECT_EQ(NetworkLinkStatus::netMgrLibHandle_, nullptr);
        EXPECT_EQ(NetworkLinkStatus::getCurrentStatus, nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
    }
    GTEST_LOG_(INFO) << "InitNetwork002 end";
}

/**
 * @tc.name: InitNetwork003
 * @tc.desc: Verify that InitNetwork initializes successfully
 * @tc.type: FUNC
 * @tc.require: IB3SWZ
 */
HWTEST_F(NetLinkStatusTest, InitNetwork003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "InitNetwork003 start";
    try {
        EXPECT_CALL(*libraryFuncMock_, dlopen(_, _))
            .WillRepeatedly(Return(reinterpret_cast<void *>(0x1)));
        EXPECT_CALL(*libraryFuncMock_, dlsym(_, _))
            .WillOnce(Return(reinterpret_cast<void *>(&MockGetNetworkStatusSuccess)));

        NetworkLinkStatus::InitNetwork();
        EXPECT_NE(NetworkLinkStatus::netMgrLibHandle_, nullptr);
        EXPECT_NE(NetworkLinkStatus::getCurrentStatus, nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
    }
    GTEST_LOG_(INFO) << "InitNetwork003 end";
}

/**
 * @tc.name: InitNetwork004
 * @tc.desc: Verify that InitNetwork doesn't reinitialize when already loaded
 * @tc.type: FUNC
 * @tc.require: IB3SWZ
 */
HWTEST_F(NetLinkStatusTest, InitNetwork004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "InitNetwork004 start";
    try {
        EXPECT_CALL(*libraryFuncMock_, dlopen(_, _))
            .WillOnce(Return(reinterpret_cast<void *>(0x1)));
        EXPECT_CALL(*libraryFuncMock_, dlsym(_, _))
            .WillOnce(Return(reinterpret_cast<void *>(&MockGetNetworkStatusSuccess)));

        NetworkLinkStatus::InitNetwork();
        void *handle = NetworkLinkStatus::netMgrLibHandle_;
        NetworkLinkStatus::InitNetwork();
        EXPECT_NE(handle, nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
    }
    GTEST_LOG_(INFO) << "InitNetwork004 end";
}

/**
 * @tc.name: GetCurrentNetStatus001
 * @tc.desc: Verify GetCurrentNetStatus returns error when getCurrentStatus is null
 * @tc.type: FUNC
 * @tc.require: IB3SWZ
 */
HWTEST_F(NetLinkStatusTest, GetCurrentNetStatus001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetCurrentNetStatus001 start";
    try {
        NetworkLinkStatus::getCurrentStatus = nullptr;
        int32_t ret = NetworkLinkStatus::GetCurrentNetStatus();
        EXPECT_EQ(ret, E_GET_NETWORK_MANAGER_FAILED);
    } catch (...) {
        EXPECT_TRUE(false);
    }
    GTEST_LOG_(INFO) << "GetCurrentNetStatus001 end";
}

/**
 * @tc.name: GetCurrentNetStatus002
 * @tc.desc: Verify GetCurrentNetStatus returns error on mocked failure
 * @tc.type: FUNC
 * @tc.require: IB3SWZ
 */
HWTEST_F(NetLinkStatusTest, GetCurrentNetStatus002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetCurrentNetStatus002 start";
    try {
        EXPECT_CALL(*libraryFuncMock_, dlopen(_, _))
            .WillRepeatedly(Return(reinterpret_cast<void *>(0x1)));
        EXPECT_CALL(*libraryFuncMock_, dlsym(_, _))
            .WillOnce(Return(reinterpret_cast<void *>(&MockGetNetworkStatusFail)));

        NetworkLinkStatus::InitNetwork();
        int32_t ret = NetworkLinkStatus::GetCurrentNetStatus();
        EXPECT_EQ(ret, E_GET_NETWORK_MANAGER_FAILED);
    } catch (...) {
        EXPECT_TRUE(false);
    }
    GTEST_LOG_(INFO) << "GetCurrentNetStatus002 end";
}

/**
 * @tc.name: GetCurrentNetStatus003
 * @tc.desc: Verify GetCurrentNetStatus succeeds with valid mock
 * @tc.type: FUNC
 * @tc.require: IB3SWZ
 */
HWTEST_F(NetLinkStatusTest, GetCurrentNetStatus003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetCurrentNetStatus003 start";
    try {
        EXPECT_CALL(*libraryFuncMock_, dlopen(_, _))
            .WillRepeatedly(Return(reinterpret_cast<void *>(0x1)));
        EXPECT_CALL(*libraryFuncMock_, dlsym(_, _))
            .WillOnce(Return(reinterpret_cast<void *>(&MockGetNetworkStatusSuccess)));

        NetworkLinkStatus::InitNetwork();
        int32_t ret = NetworkLinkStatus::GetCurrentNetStatus();
        EXPECT_EQ(ret, E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
    }
    GTEST_LOG_(INFO) << "GetCurrentNetStatus003 end";
}

/**
 * @tc.name: IsAirPlaneMode001
 * @tc.desc: Verify IsAirPlaneMode returns false when GetCurrentNetStatus fails
 * @tc.type: FUNC
 * @tc.require: IB3SWZ
 */
HWTEST_F(NetLinkStatusTest, IsAirPlaneMode001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsAirPlaneMode001 start";
    try {
        NetworkLinkStatus::getCurrentStatus = nullptr;
        NetworkLinkStatus::netStatus_.store(NET_NORMAL);
        NetworkLinkStatus::lastCheckTime_.store(0);
        bool isAirPlane = NetworkLinkStatus::IsAirPlaneMode();
        EXPECT_FALSE(isAirPlane);
    } catch (...) {
        EXPECT_TRUE(false);
    }
    GTEST_LOG_(INFO) << "IsAirPlaneMode001 end";
}

/**
 * @tc.name: IsAirPlaneMode002
 * @tc.desc: Verify IsAirPlaneMode returns false when GetCurrentNetStatus fails
 * @tc.type: FUNC
 * @tc.require: IB3SWZ
 */
HWTEST_F(NetLinkStatusTest, IsAirPlaneMode002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsAirPlaneMode002 start";
    try {
        EXPECT_CALL(*libraryFuncMock_, dlopen(_, _))
            .WillRepeatedly(Return(reinterpret_cast<void *>(0x1)));
        EXPECT_CALL(*libraryFuncMock_, dlsym(_, _))
            .WillOnce(Return(reinterpret_cast<void *>(&MockGetNetworkStatusFail)));

        NetworkLinkStatus::InitNetwork();
        NetworkLinkStatus::lastCheckTime_.store(0);
        bool isAirPlane = NetworkLinkStatus::IsAirPlaneMode();
        EXPECT_FALSE(isAirPlane);
    } catch (...) {
        EXPECT_TRUE(false);
    }
    GTEST_LOG_(INFO) << "IsAirPlaneMode002 end";
}

/**
 * @tc.name: IsAirPlaneMode003
 * @tc.desc: Verify IsAirPlaneMode returns true for airplane mode
 * @tc.type: FUNC
 * @tc.require: IB3SWZ
 */
HWTEST_F(NetLinkStatusTest, IsAirPlaneMode003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsAirPlaneMode003 start";
    try {
        EXPECT_CALL(*libraryFuncMock_, dlopen(_, _))
            .WillRepeatedly(Return(reinterpret_cast<void *>(0x1)));
        EXPECT_CALL(*libraryFuncMock_, dlsym(_, _))
            .WillOnce(Return(reinterpret_cast<void *>(&MockGetNetworkStatusAirPlane)));

        NetworkLinkStatus::InitNetwork();
        NetworkLinkStatus::lastCheckTime_.store(0);
        bool isAirPlane = NetworkLinkStatus::IsAirPlaneMode();
        EXPECT_TRUE(isAirPlane);
    } catch (...) {
        EXPECT_TRUE(false);
    }
    GTEST_LOG_(INFO) << "IsAirPlaneMode003 end";
}

/**
 * @tc.name: IsAirPlaneMode004
 * @tc.desc: Verify IsAirPlaneMode returns false for normal network
 * @tc.type: FUNC
 * @tc.require: IB3SWZ
 */
HWTEST_F(NetLinkStatusTest, IsAirPlaneMode004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsAirPlaneMode004 start";
    try {
        EXPECT_CALL(*libraryFuncMock_, dlopen(_, _))
            .WillRepeatedly(Return(reinterpret_cast<void *>(0x1)));
        EXPECT_CALL(*libraryFuncMock_, dlsym(_, _))
            .WillOnce(Return(reinterpret_cast<void *>(&MockGetNetworkStatusSuccess)));

        NetworkLinkStatus::InitNetwork();
        NetworkLinkStatus::lastCheckTime_.store(0);
        bool isAirPlane = NetworkLinkStatus::IsAirPlaneMode();
        EXPECT_FALSE(isAirPlane);
    } catch (...) {
        EXPECT_TRUE(false);
    }
    GTEST_LOG_(INFO) << "IsAirPlaneMode004 end";
}

/**
 * @tc.name: IsAirPlaneMode005
 * @tc.desc: Verify IsAirPlaneMode caches result within check interval
 * @tc.type: FUNC
 * @tc.require: IB3SWZ
 */
HWTEST_F(NetLinkStatusTest, IsAirPlaneMode005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsAirPlaneMode005 start";
    try {
        EXPECT_CALL(*libraryFuncMock_, dlopen(_, _))
            .WillRepeatedly(Return(reinterpret_cast<void *>(0x1)));
        EXPECT_CALL(*libraryFuncMock_, dlsym(_, _))
            .WillOnce(Return(reinterpret_cast<void *>(&MockGetNetworkStatusSuccess)));
 
        NetworkLinkStatus::InitNetwork();
        NetworkLinkStatus::lastCheckTime_.store(NetworkLinkStatus::GetRelativeTimeMillis());
        bool isAirPlane = NetworkLinkStatus::IsAirPlaneMode();
        EXPECT_FALSE(isAirPlane);
    } catch (...) {
        EXPECT_TRUE(false);
    }
    GTEST_LOG_(INFO) << "IsAirPlaneMode005 end";
}

/**
 * @tc.name: GetRelativeTimeMillis001
 * @tc.desc: Verify the GetRelativeTimeMillis function returns increasing values
 * @tc.type: FUNC
 * @tc.require: IB3SWZ
 */
HWTEST_F(NetLinkStatusTest, GetRelativeTimeMillis001, TestSize.Level0)
{
    int64_t time1 = NetworkLinkStatus::GetRelativeTimeMillis();
    sleep(1);
    int64_t time2 = NetworkLinkStatus::GetRelativeTimeMillis();
    EXPECT_GT(time2, time1);
}
} // namespace OHOS::FileManagement::CloudSync::Test
