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

#include <gtest/gtest.h>
#include <memory>

#include "cloud_sync_service_proxy.h"
#include "service_callback_mock.h"
#include "i_cloud_sync_service_mock.h"
#include "dfs_error.h"

namespace OHOS {
namespace FileManagement::CloudSync {
namespace Test {
using namespace testing::ext;
using namespace testing;
using namespace std;

class CloudSyncServiceProxyTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    shared_ptr<CloudSyncServiceProxy> proxy_ = nullptr;
    sptr<CloudSyncServiceMock> mock_ = nullptr;
    sptr<CloudSyncCallbackMock> remote_ = nullptr;
};

void CloudSyncServiceProxyTest::SetUpTestCase(void)
{
    std::cout << "SetUpTestCase" << std::endl;
}

void CloudSyncServiceProxyTest::TearDownTestCase(void)
{
    std::cout << "TearDownTestCase" << std::endl;
}

void CloudSyncServiceProxyTest::SetUp(void)
{
    mock_ = sptr(new CloudSyncServiceMock());
    proxy_ = make_shared<CloudSyncServiceProxy>(mock_);
    remote_ = sptr(new CloudSyncCallbackMock());
    std::cout << "SetUp" << std::endl;
}

void CloudSyncServiceProxyTest::TearDown(void)
{
    proxy_ = nullptr;
    mock_ = nullptr;
    remote_ = nullptr;
    std::cout << "TearDown" << std::endl;
}

/**
 * @tc.name: RegisterCallbackInner
 * @tc.desc: Verify the RegisterCallbackInner function.
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncServiceProxyTest, RegisterCallbackInner, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RegisterCallbackInner Start";
    EXPECT_CALL(*mock_, SendRequest(_, _, _, _))
            .WillOnce(Invoke(mock_.GetRefPtr(), &CloudSyncServiceMock::InvokeSendRequest));
    int result = proxy_->RegisterCallbackInner(remote_);
    EXPECT_EQ(result, E_OK);

    result = proxy_->RegisterCallbackInner(nullptr);
    EXPECT_NE(result, E_OK);
    GTEST_LOG_(INFO) << "RegisterCallbackInner End";
}

/**
 * @tc.name: StartSyncInner
 * @tc.desc: Verify the StartSyncInner function.
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncServiceProxyTest, StartSyncInner, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StartSyncInner Start";
    EXPECT_CALL(*mock_, SendRequest(_, _, _, _))
        .Times(2)
        .WillOnce(Invoke(mock_.GetRefPtr(), &CloudSyncServiceMock::InvokeSendRequest))
        .WillOnce(Return(EPERM));

    const bool forceFlag = true;
    int result = proxy_->StartSyncInner(forceFlag);
    EXPECT_EQ(result, E_OK);

    result = proxy_->StartSyncInner(forceFlag);
    EXPECT_NE(result, E_OK);
    GTEST_LOG_(INFO) << "StartSyncInner End";
}
/**
 * @tc.name: StopSyncInner
 * @tc.desc: Verify the StopSyncInner function.
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncServiceProxyTest, StopSyncInner, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StopSyncInner Start";
    EXPECT_CALL(*mock_, SendRequest(_, _, _, _))
        .Times(2)
        .WillOnce(Invoke(mock_.GetRefPtr(), &CloudSyncServiceMock::InvokeSendRequest))
        .WillOnce(Return(EPERM));

    int result = proxy_->StopSyncInner();
    EXPECT_EQ(result, E_OK);

    result = proxy_->StopSyncInner();
    EXPECT_NE(result, E_OK);
    GTEST_LOG_(INFO) << "StopSyncInner End";
}

/**
 * @tc.name: UploadAsset001
 * @tc.desc: Verify the UploadAsset function.
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncServiceProxyTest, UploadAsset001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UploadAsset001 Start";
    int32_t userId = 100;
    string request = "test_request";
    string result;
    EXPECT_CALL(*mock_, SendRequest(_, _, _, _)).Times(1).WillOnce(Return(-1));
    int ret = proxy_->UploadAsset(userId, request, result);
    EXPECT_EQ(ret, E_BROKEN_IPC);
    GTEST_LOG_(INFO) << "UploadAsset001 End";
}

/**
 * @tc.name: UploadAsset002
 * @tc.desc: Verify the UploadAsset function.
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncServiceProxyTest, UploadAsset002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UploadAsset002 Start";
    int32_t userId = 100;
    string request = "test_request";
    string result;
    EXPECT_CALL(*mock_, SendRequest(_, _, _, _)).Times(1).WillOnce(Return(E_OK));
    int ret = proxy_->UploadAsset(userId, request, result);
    EXPECT_EQ(ret, E_OK);
    GTEST_LOG_(INFO) << "UploadAsset002 End";
}

/**
 * @tc.name: DownloadFile001
 * @tc.desc: Verify the DownloadFile function.
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncServiceProxyTest, DownloadFile001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DownloadFile001 Start";
    int32_t userId = 100;
    string bundleName = "test_bundleName";
    AssetInfoObj assetInfoObj;
    EXPECT_CALL(*mock_, SendRequest(_, _, _, _)).Times(1).WillOnce(Return(-1));
    int ret = proxy_->DownloadFile(userId, bundleName, assetInfoObj);
    EXPECT_EQ(ret, E_BROKEN_IPC);
    GTEST_LOG_(INFO) << "DownloadFile001 End";
}

/**
 * @tc.name: DownloadFile002
 * @tc.desc: Verify the DownloadFile function.
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncServiceProxyTest, DownloadFile002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DownloadFile002 Start";
    int32_t userId = 100;
    string bundleName = "test_bundleName";
    AssetInfoObj assetInfoObj;
    EXPECT_CALL(*mock_, SendRequest(_, _, _, _)).Times(1).WillOnce(Return(E_OK));
    int ret = proxy_->DownloadFile(userId, bundleName, assetInfoObj);
    EXPECT_EQ(ret, E_OK);
    GTEST_LOG_(INFO) << "DownloadFile002 End";
}

} // namespace Test
} // namespace FileManagement::CloudSync {
} // namespace OHOS
