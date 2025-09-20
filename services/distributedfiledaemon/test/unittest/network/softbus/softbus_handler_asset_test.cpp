/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include "network/softbus/softbus_handler_asset.h"

#include <fcntl.h>
#include <gmock/gmock.h>
#include "gtest/gtest.h"
#include <memory>
#include <string>

#include "device_manager_impl_mock.h"
#include "dfs_error.h"
#include "mock_other_method.h"
#include "network/softbus/softbus_session_pool.h"
#include "socket_mock.h"
namespace OHOS {
namespace Storage {
namespace DistributedFile {
namespace Test {
using namespace OHOS::FileManagement;
using namespace std;
using namespace testing;
using namespace testing::ext;

DistributedHardware::DmDeviceInfo deviceInfo = {
    .deviceId = "testdevid",
    .deviceName = "testdevname",
    .deviceTypeId = 1,
    .networkId = "testNetWork",
    .authForm = DmAuthForm::ACROSS_ACCOUNT,
    .extraData = "{\"OS_TYPE\":10}",
};

constexpr int32_t INVALID_USER_ID = -1;

class SoftBusHandlerAssetTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    void CheckSrcSameAccountPass();
    void CheckSrcDiffAccountPass();
    void CheckSrcBothSamePass();
    void CheckSrcBothDiffPass();
    static inline shared_ptr<DfsDeviceOtherMethodMock> otherMethodMock_ = nullptr;
    static inline shared_ptr<SocketMock> socketMock_ = nullptr;
    static inline shared_ptr<DeviceManagerImplMock> deviceManagerImplMock_ = nullptr;
};

void SoftBusHandlerAssetTest::CheckSrcSameAccountPass()
{
    AccountSA::OhosAccountInfo osAccountInfo;
    osAccountInfo.uid_ = "test";
    std::vector<int32_t> userIds{100, 101};
    EXPECT_CALL(*otherMethodMock_, QueryActiveOsAccountIds(_))
        .WillOnce(DoAll(SetArgReferee<0>(userIds), Return(FileManagement::E_OK)));
    EXPECT_CALL(*otherMethodMock_, GetOhosAccountInfo(_))
        .WillOnce(DoAll(SetArgReferee<0>(osAccountInfo), Return(FileManagement::E_OK)));
    EXPECT_CALL(*deviceManagerImplMock_, GetLocalDeviceInfo(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*deviceManagerImplMock_, CheckSrcIsSameAccount(_, _)).WillOnce(Return(true));
}

void SoftBusHandlerAssetTest::CheckSrcDiffAccountPass()
{
    std::vector<int32_t> userIds{100, 101};
    EXPECT_CALL(*otherMethodMock_, QueryActiveOsAccountIds(_))
        .WillOnce(DoAll(SetArgReferee<0>(userIds), Return(FileManagement::E_OK)));
    EXPECT_CALL(*deviceManagerImplMock_, GetLocalDeviceInfo(_, _)).WillOnce(Return(0));
}

void SoftBusHandlerAssetTest::CheckSrcBothSamePass()
{
    AccountSA::OhosAccountInfo osAccountInfo;
    osAccountInfo.uid_ = "test";
    std::vector<int32_t> userIds{100, 101};
    EXPECT_CALL(*otherMethodMock_, QueryActiveOsAccountIds(_))
        .WillOnce(DoAll(SetArgReferee<0>(userIds), Return(FileManagement::E_OK)))
        .WillOnce(DoAll(SetArgReferee<0>(userIds), Return(FileManagement::E_OK)));
    EXPECT_CALL(*otherMethodMock_, GetOhosAccountInfo(_))
        .WillOnce(DoAll(SetArgReferee<0>(osAccountInfo), Return(FileManagement::E_OK)))
        .WillOnce(DoAll(SetArgReferee<0>(osAccountInfo), Return(FileManagement::E_OK)));
    EXPECT_CALL(*deviceManagerImplMock_, GetLocalDeviceInfo(_, _)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(*deviceManagerImplMock_, CheckSrcIsSameAccount(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*socketMock_, SetAccessInfo(_, _)).WillOnce(Return(FileManagement::E_OK));
}

void SoftBusHandlerAssetTest::CheckSrcBothDiffPass()
{
    std::vector<int32_t> userIds{100, 101};
    EXPECT_CALL(*otherMethodMock_, QueryActiveOsAccountIds(_))
        .WillOnce(DoAll(SetArgReferee<0>(userIds), Return(FileManagement::E_OK)))
        .WillOnce(DoAll(SetArgReferee<0>(userIds), Return(FileManagement::E_OK)));
    EXPECT_CALL(*deviceManagerImplMock_, GetLocalDeviceInfo(_, _)).WillOnce(Return(0)).WillOnce(Return(0));
}

void SoftBusHandlerAssetTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
    otherMethodMock_ = make_shared<DfsDeviceOtherMethodMock>();
    DfsDeviceOtherMethodMock::otherMethod = otherMethodMock_;
    socketMock_ = make_shared<SocketMock>();
    SocketMock::dfsSocket = socketMock_;
    deviceManagerImplMock_ = make_shared<DeviceManagerImplMock>();
    DeviceManagerImplMock::dfsDeviceManagerImpl = deviceManagerImplMock_;
}

void SoftBusHandlerAssetTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
    SocketMock::dfsSocket = nullptr;
    socketMock_ = nullptr;
    DeviceManagerImplMock::dfsDeviceManagerImpl = nullptr;
    deviceManagerImplMock_ = nullptr;
    DfsDeviceOtherMethodMock::otherMethod = nullptr;
    otherMethodMock_ = nullptr;
}

void SoftBusHandlerAssetTest::SetUp(void)
{
    GTEST_LOG_(INFO) << "SetUp";
}

void SoftBusHandlerAssetTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "TearDown";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_CreateAssetLocalSessionServer_0100
 * @tc.desc: Verify the CreateAssetLocalSessionServe function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_CreateAssetLocalSessionServer_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_CreateAssetLocalSessionServer_0100 start";
    std::string sessionName = SoftBusHandlerAsset::ASSET_LOCAL_SESSION_NAME;
    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    softBusHandlerAsset.serverIdMap_.clear();
    EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(-1));
    softBusHandlerAsset.CreateAssetLocalSessionServer();
    EXPECT_EQ(softBusHandlerAsset.serverIdMap_.size(), 0);

    EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(0));
    EXPECT_CALL(*socketMock_, Listen(_, _, _, _)).WillOnce(Return(-1));
    softBusHandlerAsset.CreateAssetLocalSessionServer();
    EXPECT_EQ(softBusHandlerAsset.serverIdMap_.size(), 0);

    EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(0));
    EXPECT_CALL(*socketMock_, Listen(_, _, _, _)).WillOnce(Return(0));
    softBusHandlerAsset.CreateAssetLocalSessionServer();
    if (softBusHandlerAsset.serverIdMap_.find(sessionName) != softBusHandlerAsset.serverIdMap_.end()) {
        EXPECT_TRUE(true);
    } else {
        EXPECT_TRUE(false);
    }
    SoftBusSessionPool::SessionInfo sessionInfo;
    bool flag = SoftBusSessionPool::GetInstance().GetSessionInfo(sessionName, sessionInfo);
    EXPECT_EQ(flag, true);
    softBusHandlerAsset.CreateAssetLocalSessionServer();
    EXPECT_EQ(softBusHandlerAsset.serverIdMap_.size(), 1);

    softBusHandlerAsset.serverIdMap_.clear();
    SoftBusSessionPool::GetInstance().DeleteSessionInfo(sessionName);
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_CreateAssetLocalSessionServer_0100 end";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_DeleteAssetLocalSessionServer_0100
 * @tc.desc: Verify the DeleteAssetLocalSessionServer function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_DeleteAssetLocalSessionServer_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_DeleteAssetLocalSessionServer_0100 start";
    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    softBusHandlerAsset.DeleteAssetLocalSessionServer();
    std::string sessionName = SoftBusHandlerAsset::ASSET_LOCAL_SESSION_NAME;
    EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(0));
    EXPECT_CALL(*socketMock_, Listen(_, _, _, _)).WillOnce(Return(0));
    softBusHandlerAsset.CreateAssetLocalSessionServer();
    if (softBusHandlerAsset.serverIdMap_.find(sessionName) != softBusHandlerAsset.serverIdMap_.end()) {
        EXPECT_TRUE(true);
    } else {
        EXPECT_TRUE(false);
    }

    softBusHandlerAsset.serverIdMap_.insert(make_pair("test", 0));
    SoftBusSessionPool::SessionInfo sessionInfo;
    bool flag = SoftBusSessionPool::GetInstance().GetSessionInfo(sessionName, sessionInfo);
    EXPECT_EQ(flag, true);
    softBusHandlerAsset.DeleteAssetLocalSessionServer();
    EXPECT_EQ(softBusHandlerAsset.serverIdMap_.size(), 1);
    flag = SoftBusSessionPool::GetInstance().GetSessionInfo(sessionName, sessionInfo);
    EXPECT_EQ(flag, false);

    softBusHandlerAsset.DeleteAssetLocalSessionServer();
    EXPECT_EQ(softBusHandlerAsset.serverIdMap_.size(), 1);
    softBusHandlerAsset.serverIdMap_.clear();
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_DeleteAssetLocalSessionServer_0100 end";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_AssetBind_0100
 * @tc.desc: Verify the AssetBind function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_AssetBind_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_AssetBind_0100 start";
#ifndef SUPPORT_SAME_ACCOUNT
    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    int32_t socketId = 0;

    EXPECT_CALL(*otherMethodMock_, QueryActiveOsAccountIds(_)).WillOnce(Return(INVALID_USER_ID));
    EXPECT_EQ(softBusHandlerAsset.AssetBind("", socketId), E_PERMISSION_DENIED);

    CheckSrcDiffAccountPass();
    EXPECT_EQ(softBusHandlerAsset.AssetBind("", socketId), E_OPEN_SESSION);

    CheckSrcDiffAccountPass();
    EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(-1));
    EXPECT_EQ(softBusHandlerAsset.AssetBind("test", socketId), E_OPEN_SESSION);

    std::vector<DmDeviceInfo> deviceList;
    deviceInfo.authForm = DmAuthForm::IDENTICAL_ACCOUNT;
    deviceList.push_back(deviceInfo);
    CheckSrcDiffAccountPass();
    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(deviceList), Return(0)));
    EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(-1));
    EXPECT_EQ(softBusHandlerAsset.AssetBind("testNetWork", socketId), E_OPEN_SESSION);
    
    CheckSrcBothDiffPass();
    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(deviceList), Return(0)));
    EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(*socketMock_, Bind(_, _, _, _)).WillOnce(Return(-1));
    EXPECT_EQ(softBusHandlerAsset.AssetBind("testNetWork", socketId), -1);

    CheckSrcBothDiffPass();
    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(deviceList), Return(0)));
    EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(*socketMock_, Bind(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(softBusHandlerAsset.AssetBind("testNetWork", socketId), E_OK);
#endif
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_AssetBind_0100 end";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_AssetBind_0200
 * @tc.desc: Verify the AssetBind function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_AssetBind_0200, TestSize.Level1)
{
#ifdef SUPPORT_SAME_ACCOUNT
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_AssetBind_0200 start";
    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    int32_t socketId = 0;

    EXPECT_CALL(*otherMethodMock_, QueryActiveOsAccountIds(_)).WillOnce(Return(INVALID_USER_ID));
    EXPECT_EQ(softBusHandlerAsset.AssetBind("", socketId), E_PERMISSION_DENIED);

    CheckSrcSameAccountPass();
    EXPECT_EQ(softBusHandlerAsset.AssetBind("", socketId), E_OPEN_SESSION);

    CheckSrcSameAccountPass();
    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _)).WillOnce(Return(0));
    EXPECT_EQ(softBusHandlerAsset.AssetBind("test", socketId), E_OPEN_SESSION);

    std::vector<DmDeviceInfo> deviceList;
    deviceInfo.authForm = DmAuthForm::IDENTICAL_ACCOUNT;
    deviceList.push_back(deviceInfo);
    CheckSrcSameAccountPass();
    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(deviceList), Return(0)));
    EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(-1));
    EXPECT_EQ(softBusHandlerAsset.AssetBind("testNetWork", socketId), E_OPEN_SESSION);

    CheckSrcBothSamePass();
    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(deviceList), Return(0)));
    EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(*socketMock_, Bind(_, _, _, _)).WillOnce(Return(-1));
    EXPECT_EQ(softBusHandlerAsset.AssetBind("testNetWork", socketId), -1);

    CheckSrcBothSamePass();
    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(deviceList), Return(0)));
    EXPECT_CALL(*socketMock_, Socket(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(*socketMock_, Bind(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(softBusHandlerAsset.AssetBind("testNetWork", socketId), E_OK);
#endif
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_AssetBind_0200 end";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_OnAssetRecvBind_0100
 * @tc.desc: Verify the OnAssetRecvBind function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_OnAssetRecvBind_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_OnAssetRecvBind_0100 start";
#ifdef SUPPORT_SAME_ACCOUNT
    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _)).WillOnce(Return(0));
    softBusHandlerAsset.OnAssetRecvBind(0, "test");
    auto iter = softBusHandlerAsset.clientInfoMap_.find(0);
    if (iter == softBusHandlerAsset.clientInfoMap_.end()) {
        EXPECT_TRUE(true);
    } else {
        EXPECT_TRUE(false);
    }
    EXPECT_EQ(softBusHandlerAsset.GetClientInfo(0), "");

    std::vector<DmDeviceInfo> deviceList;
    deviceInfo.authForm = DmAuthForm::ACROSS_ACCOUNT;
    deviceList.push_back(deviceInfo);
    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(deviceList), Return(0)));
    softBusHandlerAsset.OnAssetRecvBind(0, deviceInfo.networkId);
    EXPECT_EQ(softBusHandlerAsset.GetClientInfo(0), "");

    deviceList.clear();
    deviceInfo.authForm = DmAuthForm::IDENTICAL_ACCOUNT;
    deviceList.push_back(deviceInfo);
    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(deviceList), Return(0)));
    softBusHandlerAsset.OnAssetRecvBind(0, deviceInfo.networkId);
    EXPECT_EQ(softBusHandlerAsset.GetClientInfo(0), deviceInfo.networkId);

    softBusHandlerAsset.RemoveClientInfo(0);
    EXPECT_EQ(softBusHandlerAsset.GetClientInfo(0), "");
    iter = softBusHandlerAsset.clientInfoMap_.find(0);
    if (iter == softBusHandlerAsset.clientInfoMap_.end()) {
        EXPECT_TRUE(true);
    } else {
        EXPECT_TRUE(false);
    }
    softBusHandlerAsset.RemoveClientInfo(0);
#endif
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_OnAssetRecvBind_0100 end";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_AddAssetObj_0100
 * @tc.desc: Verify the OnAssetRecvBind function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_AddAssetObj_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_AddAssetObj_0100 start";
    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    sptr<AssetObj> assetObj (new (std::nothrow) AssetObj());
    softBusHandlerAsset.AddAssetObj(0, assetObj);
    softBusHandlerAsset.AddAssetObj(0, assetObj);
    auto iter = softBusHandlerAsset.assetObjMap_.find(0);
    if (iter != softBusHandlerAsset.assetObjMap_.end()) {
        EXPECT_TRUE(true);
    } else {
        EXPECT_TRUE(false);
    }

    EXPECT_EQ(softBusHandlerAsset.GetAssetObj(0), assetObj);

    softBusHandlerAsset.RemoveAssetObj(0);
    EXPECT_EQ(softBusHandlerAsset.GetAssetObj(0), nullptr);
    iter = softBusHandlerAsset.assetObjMap_.find(0);
    if (iter == softBusHandlerAsset.assetObjMap_.end()) {
        EXPECT_TRUE(true);
    } else {
        EXPECT_TRUE(false);
    }

    softBusHandlerAsset.RemoveAssetObj(0);
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_AddAssetObj_0100 end";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_GetDstFile_0100
 * @tc.desc: Verify the GetDstFile function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_GetDstFile_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_GetDstFile_0100 start";
    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    string file = "test123";
    auto rlt = softBusHandlerAsset.GetDstFile(file, "demoA", "demoB", "0", true);
    EXPECT_EQ(rlt, "");

    file = "demoA/test";
    rlt = softBusHandlerAsset.GetDstFile(file, "demoA", "demoB", "0", true);
    string execptRlt = "demoB/ASSET_TEMP/test.asset_single?srcBundleName=demoA&sessionId=0";
    EXPECT_EQ(rlt, execptRlt);

    execptRlt = "demoB/ASSET_TEMP/test.asset_zip?srcBundleName=demoA&sessionId=0";
    rlt = softBusHandlerAsset.GetDstFile(file, "demoA", "demoB", "0", false);
    EXPECT_EQ(rlt, execptRlt);
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_GetDstFile_0100 end";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_AssetSendFile_0100
 * @tc.desc: Verify the AssetSendFile function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_AssetSendFile_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_AssetSendFile_0100 start";
#ifdef SUPPORT_SAME_ACCOUNT
    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    string file = "test123";
    EXPECT_EQ(softBusHandlerAsset.AssetSendFile(0, file, true), ERR_BAD_VALUE);

    sptr<AssetObj> assetObj (new (std::nothrow) AssetObj());
    softBusHandlerAsset.AddAssetObj(0, assetObj);
    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _)).WillOnce(Return(0));
    EXPECT_EQ(softBusHandlerAsset.AssetSendFile(0, file, true), ERR_BAD_VALUE);

    softBusHandlerAsset.RemoveAssetObj(0);
    assetObj->dstNetworkId_ = "testNetWork";
    assetObj->srcBundleName_ = "demoA";
    assetObj->dstBundleName_ = "demoB";
    assetObj->sessionId_ = "0";
    softBusHandlerAsset.AddAssetObj(0, assetObj);

    std::vector<DmDeviceInfo> deviceList;
    deviceList.push_back(deviceInfo);
    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(deviceList), Return(0)));
    EXPECT_EQ(softBusHandlerAsset.AssetSendFile(0, file, true), ERR_BAD_VALUE);

    deviceList.clear();
    deviceInfo.authForm = DmAuthForm::IDENTICAL_ACCOUNT;
    deviceList.push_back(deviceInfo);
    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(deviceList), Return(0)));
    EXPECT_EQ(softBusHandlerAsset.AssetSendFile(0, file, true), ERR_BAD_VALUE);

    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(deviceList), Return(0)));
    EXPECT_CALL(*socketMock_, SendFile(_, _, _, _)).WillOnce(Return(-1));
    file = "demoA/test";
    EXPECT_EQ(softBusHandlerAsset.AssetSendFile(0, file, true), -1);

    EXPECT_CALL(*deviceManagerImplMock_, GetTrustedDeviceList(_, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(deviceList), Return(0)));
    EXPECT_CALL(*socketMock_, SendFile(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(softBusHandlerAsset.AssetSendFile(0, file, true), E_OK);
#endif
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_AssetSendFile_0100 end";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_GetLocalNetworkId_0100
 * @tc.desc: Verify the GetLocalNetworkId function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_GetLocalNetworkId_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_GetLocalNetworkId_0100 start";
    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    EXPECT_CALL(*deviceManagerImplMock_, GetLocalNodeDeviceInfo(_, _)).WillOnce(Return(-1));
    string ret = softBusHandlerAsset.GetLocalNetworkId();
    EXPECT_EQ(ret, "");

    EXPECT_CALL(*deviceManagerImplMock_, GetLocalNodeDeviceInfo(_, _)).WillOnce(Return(0));
    ret = softBusHandlerAsset.GetLocalNetworkId();
    EXPECT_EQ(ret, "100");
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_GetLocalNetworkId_0100 end";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_GenerateAssetObjInfo_0100
 * @tc.desc: Verify the GenerateAssetObjInfo function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_GenerateAssetObjInfo_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_GenerateAssetObjInfo_0100 start";
    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    sptr<AssetObj> assetObj (new (std::nothrow) AssetObj());
    string fileName = "test";
    auto ret = softBusHandlerAsset.GenerateAssetObjInfo(0, fileName, assetObj);
    EXPECT_EQ(ret, ERR_BAD_VALUE);

    fileName = "/account/device_view/local/data/test";
    ret = softBusHandlerAsset.GenerateAssetObjInfo(0, fileName, assetObj);
    EXPECT_EQ(ret, ERR_BAD_VALUE);

    fileName = "/account/device_view/local/data/test/";
    ret = softBusHandlerAsset.GenerateAssetObjInfo(0, fileName, assetObj);
    EXPECT_EQ(ret, ERR_BAD_VALUE);

    fileName = "/account/device_view/local/data/test/sessionId=20";
    ret = softBusHandlerAsset.GenerateAssetObjInfo(0, fileName, assetObj);
    EXPECT_EQ(ret, ERR_BAD_VALUE);

    fileName = "/account/device_view/local/data/test/sessionId=20&srcBundleName=demoA";
    EXPECT_CALL(*deviceManagerImplMock_, GetLocalNodeDeviceInfo(_, _)).WillOnce(Return(-1));
    ret = softBusHandlerAsset.GenerateAssetObjInfo(0, fileName, assetObj);
    EXPECT_EQ(ret, ERR_BAD_VALUE);

    fileName = "/account/device_view/local/data/test/sessionId=20&srcBundleName=demoA";
    EXPECT_CALL(*deviceManagerImplMock_, GetLocalNodeDeviceInfo(_, _)).WillOnce(Return(0));
    ret = softBusHandlerAsset.GenerateAssetObjInfo(0, fileName, assetObj);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(assetObj->dstBundleName_, "test");
    EXPECT_EQ(assetObj->sessionId_, "20");
    EXPECT_EQ(assetObj->srcBundleName_, "demoA");
    EXPECT_EQ(assetObj->dstNetworkId_, "100");
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_GenerateAssetObjInfo_0100 end";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_GenerateUris_0100
 * @tc.desc: Verify the GenerateUris function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_GenerateUris_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_GenerateUris_0100 start";
    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    string fileName = "test";
    vector<string> fileList;
    fileList.push_back(fileName);
    auto rlt = softBusHandlerAsset.GenerateUris(fileList, "demoB", true);
    EXPECT_EQ(rlt.size(), 0);

    fileList[0].clear();
    fileList[0] = "demoB/ASSET_TEMP";
    rlt = softBusHandlerAsset.GenerateUris(fileList, "demoB", true);
    EXPECT_EQ(rlt.size(), 0);

    fileList[0].clear();
    fileList[0] = "demoB/ASSET_TEMP/test.asset_single?";
    rlt = softBusHandlerAsset.GenerateUris(fileList, "demoB", true);
    EXPECT_EQ(rlt.size(), 1);
    EXPECT_EQ(rlt[0], "file://demoB/data/storage/el2/distributedfiles/test");
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_GenerateUris_0100 end";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_GenerateUris_0200
 * @tc.desc: Verify the GenerateUris function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_GenerateUris_0200, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_GenerateUris_0200 start";
    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    string fileName = "test";
    vector<string> fileList;
    fileList.push_back(fileName);
    auto rlt = softBusHandlerAsset.GenerateUris(fileList, "demoB", false);
    EXPECT_EQ(rlt.size(), 0);

    fileList[0].clear();
    fileList[0] = "demoB/ASSET_TEMP/test1";
    fileList.push_back("demoB/ASSET_TEMP/test2");
    rlt = softBusHandlerAsset.GenerateUris(fileList, "demoB", false);
    EXPECT_EQ(rlt.size(), 2);
    EXPECT_EQ(rlt[0], "file://demoB/data/storage/el2/distributedfiles/test1");
    EXPECT_EQ(rlt[1], "file://demoB/data/storage/el2/distributedfiles/test2");
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_GenerateUris_0200 end";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_IsDir_0100
 * @tc.desc: Verify the IsDir function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_IsDir_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_IsDir_0100 start";
    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    string fileName =  "/data/test/";
    auto rlt = softBusHandlerAsset.IsDir(fileName);
    EXPECT_EQ(rlt, true);

    fileName.clear();
    fileName = "/data/test/test.txt";
    int32_t fd = open(fileName.c_str(), O_RDWR | O_CREAT);
    ASSERT_TRUE(fd != -1) << "SoftBusHandlerAssetTest_IsDir_0100 Create File Failed!";
    close(fd);
    rlt = softBusHandlerAsset.IsDir(fileName);
    EXPECT_EQ(rlt, false);

    softBusHandlerAsset.RemoveFile(fileName, true);
    rlt = softBusHandlerAsset.IsDir(fileName);
    EXPECT_EQ(rlt, false);
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_IsDir_0100 end";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_RemoveFile_0100
 * @tc.desc: Verify the RemoveFile function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_RemoveFile_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_RemoveFile_0100 start";
    string fileName = "/data/test/test.txt";
    int32_t fd = open(fileName.c_str(), O_RDWR | O_CREAT);
    ASSERT_TRUE(fd != -1) << "SoftBusHandlerAssetTest_RemoveFile_0100 Create File Failed!";
    close(fd);

    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    softBusHandlerAsset.RemoveFile(fileName, false);
    auto ret = access(fileName.c_str(), F_OK);
    EXPECT_EQ(ret, 0);

    softBusHandlerAsset.RemoveFile(fileName, true);
    ret = access(fileName.c_str(), F_OK);
    EXPECT_NE(ret, 0);
    softBusHandlerAsset.RemoveFile(fileName, true);
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_RemoveFile_0100 end";
}

/**
 * @tc.name: SoftBusHandlerAssetTest_GetSocketIdFromAssetObj_0100
 * @tc.desc: Verify the GetSocketIdFromAssetObj function.
 * @tc.type: FUNC
 * @tc.require: I9JXPR
 */
HWTEST_F(SoftBusHandlerAssetTest, SoftBusHandlerAssetTest_GetSocketIdFromAssetObj_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_GetSocketIdFromAssetObj_0100 start";
    auto &&softBusHandlerAsset = SoftBusHandlerAsset::GetInstance();
    std::string peerNetworkId = "test";

    sptr<AssetObj> assetObj (new (std::nothrow) AssetObj());
    ASSERT_TRUE(assetObj != nullptr) << "assetObj assert failed!";
    assetObj->dstNetworkId_ = peerNetworkId;
    softBusHandlerAsset.assetObjMap_.emplace(0, nullptr);
    auto ret = softBusHandlerAsset.GetSocketIdFromAssetObj(peerNetworkId);
    EXPECT_EQ(ret.size(), 0);

    softBusHandlerAsset.assetObjMap_.emplace(1, assetObj);
    ret = softBusHandlerAsset.GetSocketIdFromAssetObj(peerNetworkId);
    EXPECT_EQ(ret.size(), 1);
    ASSERT_TRUE(ret.size() == 1) << "GetSocketIdFromAssetObj size failed!";
    EXPECT_EQ(ret[0], 1);
    GTEST_LOG_(INFO) << "SoftBusHandlerAssetTest_GetSocketIdFromAssetObj_0100 end";
}
} // namespace Test
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
