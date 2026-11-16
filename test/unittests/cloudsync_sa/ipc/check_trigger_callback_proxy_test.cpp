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

#include <gtest/gtest.h>
#include <memory>

#include "ipc/check_trigger_callback_proxy.h"
#include "dfs_error.h"
#include "message_parcel_mock.h"
#include "service_callback_mock.h"

namespace OHOS {
namespace FileManagement::CloudSync {
namespace Test {
using namespace testing::ext;
using namespace testing;
using namespace std;
using namespace OHOS::Storage::DistributedFile;

class CheckTriggerCallbackProxyTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    shared_ptr<CheckTriggerCallbackProxy> proxy_ = nullptr;
    sptr<CheckTriggerCallbackMock> mock_ = nullptr;
    shared_ptr<MessageParcelMock> messageParcelMock_ = nullptr;
};

void CheckTriggerCallbackProxyTest::SetUpTestCase(void)
{
    std::cout << "SetUpTestCase" << std::endl;
}

void CheckTriggerCallbackProxyTest::TearDownTestCase(void)
{
    std::cout << "TearDownTestCase" << std::endl;
}

void CheckTriggerCallbackProxyTest::SetUp(void)
{
    mock_ = sptr(new CheckTriggerCallbackMock());
    proxy_ = make_shared<CheckTriggerCallbackProxy>(mock_);
    messageParcelMock_ = make_shared<MessageParcelMock>();
    DfsMessageParcel::messageParcel = messageParcelMock_;
    std::cout << "SetUp" << std::endl;
}

void CheckTriggerCallbackProxyTest::TearDown(void)
{
    DfsMessageParcel::messageParcel = nullptr;
    std::cout << "TearDown" << std::endl;
}

/**
 * @tc.name: OnCheckTriggeredTest001
 * @tc.desc: Verify OnCheckTriggered sends the callback request.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CheckTriggerCallbackProxyTest, OnCheckTriggeredTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest001 Start";
    try {
        EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
        EXPECT_CALL(*messageParcelMock_, WriteInt32(_)).WillOnce(Return(true));
        EXPECT_CALL(*messageParcelMock_, WriteString(_)).Times(2).WillRepeatedly(Return(true));
        EXPECT_CALL(*mock_, SendRequest(ICheckTriggerCallback::SERVICE_CMD_ON_CHECK_TRIGGERED, _, _, _))
            .Times(1).WillOnce(Return(E_OK));
        proxy_->OnCheckTriggered(CheckTriggerReason::MANUAL, "com.ohos.photos", "com.ohos.trigger");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnCheckTriggeredTest001 ERROR";
    }
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest001 End";
}

/**
 * @tc.name: OnCheckTriggeredTest002
 * @tc.desc: Verify OnCheckTriggered handles a send failure.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CheckTriggerCallbackProxyTest, OnCheckTriggeredTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest002 Start";
    try {
        EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
        EXPECT_CALL(*messageParcelMock_, WriteInt32(_)).WillOnce(Return(true));
        EXPECT_CALL(*messageParcelMock_, WriteString(_)).Times(2).WillRepeatedly(Return(true));
        EXPECT_CALL(*mock_, SendRequest(ICheckTriggerCallback::SERVICE_CMD_ON_CHECK_TRIGGERED, _, _, _))
            .Times(1).WillOnce(Return(-1));
        proxy_->OnCheckTriggered(CheckTriggerReason::PERIODIC, "com.ohos.photos", "");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnCheckTriggeredTest002 ERROR";
    }
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest002 End";
}

/**
 * @tc.name: OnCheckTriggeredTest003
 * @tc.desc: Verify OnCheckTriggered handles interface token write failure.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CheckTriggerCallbackProxyTest, OnCheckTriggeredTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest003 Start";
    try {
        EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(false));
        EXPECT_CALL(*mock_, SendRequest(_, _, _, _)).Times(0);
        proxy_->OnCheckTriggered(CheckTriggerReason::MANUAL, "com.ohos.photos", "com.ohos.trigger");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnCheckTriggeredTest003 ERROR";
    }
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest003 End";
}

/**
 * @tc.name: OnCheckTriggeredTest004
 * @tc.desc: Verify OnCheckTriggered handles reason write failure.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CheckTriggerCallbackProxyTest, OnCheckTriggeredTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest004 Start";
    try {
        EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
        EXPECT_CALL(*messageParcelMock_, WriteInt32(_)).WillOnce(Return(false));
        EXPECT_CALL(*mock_, SendRequest(_, _, _, _)).Times(0);
        proxy_->OnCheckTriggered(CheckTriggerReason::MANUAL, "com.ohos.photos", "com.ohos.trigger");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnCheckTriggeredTest004 ERROR";
    }
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest004 End";
}

/**
 * @tc.name: OnCheckTriggeredTest005
 * @tc.desc: Verify OnCheckTriggered handles bundleName write failure.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CheckTriggerCallbackProxyTest, OnCheckTriggeredTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest005 Start";
    try {
        EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
        EXPECT_CALL(*messageParcelMock_, WriteInt32(_)).WillOnce(Return(true));
        EXPECT_CALL(*messageParcelMock_, WriteString(_)).WillOnce(Return(false));
        EXPECT_CALL(*mock_, SendRequest(_, _, _, _)).Times(0);
        proxy_->OnCheckTriggered(CheckTriggerReason::MANUAL, "com.ohos.photos", "com.ohos.trigger");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnCheckTriggeredTest005 ERROR";
    }
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest005 End";
}

/**
 * @tc.name: OnCheckTriggeredTest006
 * @tc.desc: Verify OnCheckTriggered handles triggerBundleName write failure.
 * @type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CheckTriggerCallbackProxyTest, OnCheckTriggeredTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest006 Start";
    try {
        EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
        EXPECT_CALL(*messageParcelMock_, WriteInt32(_)).WillOnce(Return(true));
        EXPECT_CALL(*messageParcelMock_, WriteString(_))
            .WillOnce(Return(true))
            .WillOnce(Return(false));
        EXPECT_CALL(*mock_, SendRequest(_, _, _, _)).Times(0);
        proxy_->OnCheckTriggered(CheckTriggerReason::MANUAL, "com.ohos.photos", "com.ohos.trigger");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnCheckTriggeredTest006 ERROR";
    }
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest006 End";
}

/**
 * @tc.name: OnCheckTriggeredTest007
 * @tc.desc: Verify OnCheckTriggered handles a null remote object.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CheckTriggerCallbackProxyTest, OnCheckTriggeredTest007, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest007 Start";
    try {
        auto nullProxy = make_shared<CheckTriggerCallbackProxy>(nullptr);
        EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
        EXPECT_CALL(*messageParcelMock_, WriteInt32(_)).WillOnce(Return(true));
        EXPECT_CALL(*messageParcelMock_, WriteString(_)).Times(2).WillRepeatedly(Return(true));
        EXPECT_CALL(*mock_, SendRequest(_, _, _, _)).Times(0);
        nullProxy->OnCheckTriggered(CheckTriggerReason::MANUAL, "com.ohos.photos", "com.ohos.trigger");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnCheckTriggeredTest007 ERROR";
    }
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest007 End";
}
} // namespace Test
} // namespace FileManagement::CloudSync
} // namespace OHOS
