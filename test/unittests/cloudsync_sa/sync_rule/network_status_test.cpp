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
#include <cstdint>
#include <unistd.h>

#include "dfs_error.h"
#include "net_conn_client.h"
#include "parameter.h"
#include "net_conn_callback_observer.h"
#include "network_status.h"
#include "utils_log.h"
#include "net_conn_client_mock.h"
#include "net_handle.h"

namespace OHOS::FileManagement::CloudSync::Test {
using namespace testing;
using namespace testing::ext;
using namespace std;
using namespace CloudFile;

class NetworkStatusTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static inline shared_ptr<NetConnClientMock> dfsNetConnClient_ = nullptr;
};
void NetworkStatusTest::SetUpTestCase(void)
{
    dfsNetConnClient_ = make_shared<NetConnClientMock>();
    NetConnClientMock::dfsNetConnClient = dfsNetConnClient_;
    GTEST_LOG_(INFO) << "SetUpTestCase";
}

void NetworkStatusTest::TearDownTestCase(void)
{
    NetConnClientMock::dfsNetConnClient = nullptr;
    dfsNetConnClient_ = nullptr;
    GTEST_LOG_(INFO) << "TearDownTestCase";
}

void NetworkStatusTest::SetUp(void)
{
    GTEST_LOG_(INFO) << "SetUp";
}

void NetworkStatusTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "TearDown";
}

/**
 * @tc.name: RegisterNetConnCallbackTest
 * @tc.desc: Verify the RegisterNetConnCallback function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, RegisterNetConnCallbackTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RegisterNetConnCallbackTest Start";
    try {
        auto dataSyncManager = std::make_shared<DataSyncManager>();
        int32_t ret = NetworkStatus::RegisterNetConnCallback(dataSyncManager);
        EXPECT_EQ(ret, E_GET_NETWORK_MANAGER_FAILED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " RegisterNetConnCallbackTest FAILED";
    }
    GTEST_LOG_(INFO) << "RegisterNetConnCallbackTest End";
}

/**
 * @tc.name: GetDefaultNetTest001
 * @tc.desc: Verify the GetDefaultNet function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, GetDefaultNetTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetDefaultNetTest001 Start";
    EXPECT_CALL(*dfsNetConnClient_, GetDefaultNet(_)).WillOnce(Return(E_GET_NETWORK_MANAGER_FAILED));
    NetworkStatus netStatus;
    int32_t ret = netStatus.GetDefaultNet();
    EXPECT_EQ(ret, E_GET_NETWORK_MANAGER_FAILED);
    GTEST_LOG_(INFO) << "GetDefaultNetTest001 End";
}

/**
 * @tc.name: GetDefaultNetTest002
 * @tc.desc: Verify the GetDefaultNet function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, GetDefaultNetTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetDefaultNetTest002 Start";
    EXPECT_CALL(*dfsNetConnClient_, GetDefaultNet(_)).WillOnce(Return(NetManagerStandard::NETMANAGER_SUCCESS));
    NetworkStatus netStatus;
    int32_t ret = netStatus.GetDefaultNet();
    EXPECT_EQ(ret, E_OK);
    GTEST_LOG_(INFO) << "GetDefaultNetTest002 End";
}

/**
 * @tc.name: SetNetConnStatusTest001
 * @tc.desc: Verify the SetNetConnStatus function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, SetNetConnStatusTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetNetConnStatusTest001 Start";
    try {
        NetManagerStandard::NetAllCapabilities netAllCap;
        NetworkStatus::SetNetConnStatus(netAllCap);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " SetNetConnStatusTest001 FAILED";
    }
    GTEST_LOG_(INFO) << "SetNetConnStatusTest001 End";
}

/**
 * @tc.name: SetNetConnStatusTest002
 * @tc.desc: Verify the SetNetConnStatus function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, SetNetConnStatusTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetNetConnStatusTest001 Start";
    try {
        NetManagerStandard::NetAllCapabilities netAllCap;
        netAllCap.netCaps_.insert(NetManagerStandard::NetCap::NET_CAPABILITY_INTERNET);
        NetworkStatus::SetNetConnStatus(netAllCap);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " SetNetConnStatusTest001 FAILED";
    }
    GTEST_LOG_(INFO) << "SetNetConnStatusTest001 End";
}

/**
 * @tc.name: SetNetConnStatusTest003
 * @tc.desc: Verify the SetNetConnStatus function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, SetNetConnStatusTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetNetConnStatusTest001 Start";
    try {
        NetManagerStandard::NetAllCapabilities netAllCap;
        netAllCap.netCaps_.insert(NetManagerStandard::NetCap::NET_CAPABILITY_INTERNET);
        netAllCap.bearerTypes_.insert(NetManagerStandard::BEARER_ETHERNET);
        NetworkStatus::SetNetConnStatus(netAllCap);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " SetNetConnStatusTest001 FAILED";
    }
    GTEST_LOG_(INFO) << "SetNetConnStatusTest001 End";
}

/**
 * @tc.name: SetNetConnStatusTest004
 * @tc.desc: Verify the SetNetConnStatus function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, SetNetConnStatusTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetNetConnStatusTest001 Start";
    try {
        NetManagerStandard::NetAllCapabilities netAllCap;
        netAllCap.netCaps_.insert(NetManagerStandard::NetCap::NET_CAPABILITY_INTERNET);
        netAllCap.bearerTypes_.insert(NetManagerStandard::BEARER_WIFI);
        NetworkStatus::SetNetConnStatus(netAllCap);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " SetNetConnStatusTest001 FAILED";
    }
    GTEST_LOG_(INFO) << "SetNetConnStatusTest001 End";
}

/**
 * @tc.name: SetNetConnStatusTest005
 * @tc.desc: Verify the SetNetConnStatus function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, SetNetConnStatusTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetNetConnStatusTest001 Start";
    try {
        NetManagerStandard::NetAllCapabilities netAllCap;
        netAllCap.netCaps_.insert(NetManagerStandard::NetCap::NET_CAPABILITY_INTERNET);
        netAllCap.bearerTypes_.insert(NetManagerStandard::BEARER_CELLULAR);
        NetworkStatus::SetNetConnStatus(netAllCap);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " SetNetConnStatusTest001 FAILED";
    }
    GTEST_LOG_(INFO) << "SetNetConnStatusTest001 End";
}

/**
 * @tc.name: GetAndRegisterNetworkTest
 * @tc.desc: Verify the GetAndRegisterNetwork function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, GetAndRegisterNetworkTest, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetAndRegisterNetworkTest Start";
    try {
        auto dataSyncManager = std::make_shared<DataSyncManager>();
        int32_t ret = NetworkStatus::GetAndRegisterNetwork(dataSyncManager);
        EXPECT_EQ(ret, E_GET_NETWORK_MANAGER_FAILED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " GetAndRegisterNetworkTest FAILED";
    }
    GTEST_LOG_(INFO) << "GetAndRegisterNetworkTest End";
}

/**
 * @tc.name: InitNetworkTest
 * @tc.desc: Verify the InitNetwork function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, InitNetworkTest, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "InitNetworkTest Start";
    try {
        auto dataSyncManager = std::make_shared<DataSyncManager>();
        NetworkStatus::InitNetwork(dataSyncManager);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " InitNetworkTest FAILED";
    }
    GTEST_LOG_(INFO) << "InitNetworkTest End";
}

/**
 * @tc.name: CheckMobileNetworkTest001
 * @tc.desc: Verify the CheckMobileNetwork function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, CheckMobileNetworkTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "CheckMobileNetworkTest Start";
    try {
        NetworkStatus networkStatus;
        string bundleName = "";
        int32_t userId = 100;
        bool ret = networkStatus.CheckMobileNetwork(bundleName, userId);
        EXPECT_EQ(ret, true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "CheckMobileNetworkTest FAILED";
    }
    GTEST_LOG_(INFO) << "CheckMobileNetworkTest End";
}

/**
 * @tc.name: CheckMobileNetworkTest002
 * @tc.desc: Verify the CheckMobileNetwork function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, CheckMobileNetworkTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "CheckMobileNetworkTest Start";
    try {
        NetworkStatus networkStatus;
        string bundleName = "com.ohos.photos";
        int32_t userId = 1;
        bool ret = networkStatus.CheckMobileNetwork(bundleName, userId);
        EXPECT_EQ(ret, true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "CheckMobileNetworkTest FAILED";
    }
    GTEST_LOG_(INFO) << "CheckMobileNetworkTest End";
}

/**
 * @tc.name: CheckMobileNetworkTest003
 * @tc.desc: Verify the CheckMobileNetwork function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, CheckMobileNetworkTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "CheckMobileNetworkTest Start";
    try {
        NetworkStatus networkStatus;
        string bundleName = "com.ohos.photos";
        int32_t userId = 100;
        networkStatus.SetNetConnStatus(NetworkStatus::WIFI_CONNECT);
        bool ret = networkStatus.CheckMobileNetwork(bundleName, userId);
        EXPECT_EQ(ret, true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "CheckMobileNetworkTest FAILED";
    }
    GTEST_LOG_(INFO) << "CheckMobileNetworkTest End";
}

/**
 * @tc.name: CheckMobileNetworkTest004
 * @tc.desc: Verify the CheckMobileNetwork function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, CheckMobileNetworkTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "CheckMobileNetworkTest Start";
    try {
        NetworkStatus networkStatus;
        string bundleName = "com.ohos.photos";
        int32_t userId = 100;
        networkStatus.SetNetConnStatus(NetworkStatus::NO_NETWORK);
        bool ret = networkStatus.CheckMobileNetwork(bundleName, userId);
        EXPECT_EQ(ret, false);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "CheckMobileNetworkTest FAILED";
    }
    GTEST_LOG_(INFO) << "CheckMobileNetworkTest End";
}

/**
 * @tc.name: CheckNetworkTest001
 * @tc.desc: Verify the CheckNetwork function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, CheckNetworkTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "CheckNetworkTest Start";
    try {
        NetworkStatus networkStatus;
        string bundleName = "";
        int32_t userId = 100;
        bool ret = networkStatus.CheckNetwork(bundleName, userId);
        EXPECT_EQ(ret, true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "CheckNetworkTest FAILED";
    }
    GTEST_LOG_(INFO) << "CheckNetworkTest End";
}

/**
 * @tc.name: CheckNetworkTest002
 * @tc.desc: Verify the CheckNetwork function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, CheckNetworkTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "CheckNetworkTest Start";
    try {
        NetworkStatus networkStatus;
        string bundleName = "com.ohos.photos";
        int32_t userId = 1;
        bool ret = networkStatus.CheckNetwork(bundleName, userId);
        EXPECT_EQ(ret, true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "CheckNetworkTest FAILED";
    }
    GTEST_LOG_(INFO) << "CheckNetworkTest End";
}

/**
 * @tc.name: CheckNetworkTest003
 * @tc.desc: Verify the CheckNetwork function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, CheckNetworkTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "CheckNetworkTest Start";
    try {
        NetworkStatus networkStatus;
        string bundleName = "com.ohos.photos";
        int32_t userId = 100;
        bool ret = networkStatus.CheckNetwork(bundleName, userId);
        EXPECT_EQ(ret, false);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "CheckNetworkTest FAILED";
    }
    GTEST_LOG_(INFO) << "CheckNetworkTest End";
}

/**
 * @tc.name: OnNetworkAvailTest001
 * @tc.desc: Verify the OnNetworkAvail function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, OnNetworkAvailTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnNetworkAvailTest Start";
    try {
        NetworkStatus networkStatus;
        networkStatus.SetNetConnStatus(NetworkStatus::WIFI_CONNECT);
        networkStatus.OnNetworkAvail();
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnNetworkAvailTest FAILED";
    }
    GTEST_LOG_(INFO) << "OnNetworkAvailTest End";
}

/**
 * @tc.name: OnNetworkAvailTest002
 * @tc.desc: Verify the OnNetworkAvail function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(NetworkStatusTest, OnNetworkAvailTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnNetworkAvailTest Start";
    try {
        NetworkStatus networkStatus;
        networkStatus.SetNetConnStatus(NetworkStatus::NO_NETWORK);
        networkStatus.OnNetworkAvail();
        EXPECT_EQ(networkStatus.netStatus_, NetworkStatus::NETWORK_AVAIL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnNetworkAvailTest FAILED";
    }
    GTEST_LOG_(INFO) << "OnNetworkAvailTest End";
}
} // namespace OHOS::FileManagement::CloudSync::Test