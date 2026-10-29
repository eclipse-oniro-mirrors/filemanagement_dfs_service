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
#include <gmock/gmock.h>

#include "check_trigger_callback_stub.h"
#include "cloud_sync_callback_stub.h"
#include "cloud_sync_constants.h"
#include "service_callback_mock.h"
#include "i_cloud_sync_callback.h"
#include "dfs_error.h"
#include "message_parcel_mock.h"

namespace OHOS {
namespace FileManagement::CloudSync {
namespace Test {
using namespace testing;
using namespace testing::ext;
using namespace std;
using namespace OHOS::Storage::DistributedFile;

class MockCallback final : public CloudSyncCallbackStub {
public:
    MOCK_METHOD2(OnSyncStateChanged, void(CloudSyncState state, ErrorType error));
};

class MockCheckTriggerCallback final : public CheckTriggerCallbackStub {
public:
    MOCK_METHOD3(OnCheckTriggered, void(CheckTriggerReason reason, const std::string &bundleName,
        const std::string &triggerBundleName));
};

class CloudSyncCallbackStubTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    std::shared_ptr<MessageParcelMock> messageParcelMock_;
};

void CloudSyncCallbackStubTest::SetUpTestCase(void)
{
    std::cout << "SetUpTestCase" << std::endl;
}

void CloudSyncCallbackStubTest::TearDownTestCase(void)
{
    std::cout << "TearDownTestCase" << std::endl;
}

void CloudSyncCallbackStubTest::SetUp(void)
{
    messageParcelMock_ = std::make_shared<MessageParcelMock>();
    DfsMessageParcel::messageParcel = messageParcelMock_;
    std::cout << "SetUp" << std::endl;
}

void CloudSyncCallbackStubTest::TearDown(void)
{
    DfsMessageParcel::messageParcel = nullptr;
    std::cout << "TearDown" << std::endl;
}

/**
 * @tc.name: HandleOnSyncStateChangedTest001
 * @tc.desc: Verify HandleOnSyncStateChanged function - ReadInt32(stateValue) failed
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnSyncStateChangedTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnSyncStateChangedTest001 Start";
    try {
        MockCallback callback;
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;

        EXPECT_CALL(*messageParcelMock_, ReadInt32(_)).WillOnce(Return(false));
        int ret = callback.HandleOnSyncStateChanged(data, reply);

        EXPECT_EQ(ret, E_INVAL_ARG);
    } catch(...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " HandleOnSyncStateChangedTest001 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnSyncStateChangedTest001 End";
}

/**
 * @tc.name: HandleOnSyncStateChangedTest002
 * @tc.desc: Verify HandleOnSyncStateChanged function - ReadInt32(errorValue) failed
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnSyncStateChangedTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnSyncStateChangedTest002 Start";
    try {
        MockCallback callback;
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;

        EXPECT_CALL(*messageParcelMock_, ReadInt32(_))
            .WillOnce(DoAll(SetArgReferee<0>(0), Return(true)))
            .WillOnce(Return(false));
        int ret = callback.HandleOnSyncStateChanged(data, reply);

        EXPECT_EQ(ret, E_INVAL_ARG);
    } catch(...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " HandleOnSyncStateChangedTest002 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnSyncStateChangedTest002 End";
}

/**
 * @tc.name: HandleOnSyncStateChangedTest003
 * @tc.desc: Verify HandleOnSyncStateChanged function - all reads succeed
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnSyncStateChangedTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnSyncStateChangedTest003 Start";
    try {
        MockCallback callback;
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;

        EXPECT_CALL(*messageParcelMock_, ReadInt32(_))
            .WillOnce(DoAll(SetArgReferee<0>(0), Return(true)))
            .WillOnce(DoAll(SetArgReferee<0>(0), Return(true)));
        EXPECT_CALL(callback, OnSyncStateChanged(_, _)).WillOnce(Return());

        int ret = callback.HandleOnSyncStateChanged(data, reply);
        EXPECT_EQ(ret, E_OK);
    } catch(...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " HandleOnSyncStateChangedTest003 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnSyncStateChangedTest003 End";
}

/**
 * @tc.name: HandleOnSyncStateChangedTest004
 * @tc.desc: Verify HandleOnSyncStateChanged function - stateValue < UPLOADING
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnSyncStateChangedTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnSyncStateChangedTest004 Start";
    try {
        MockCallback callback;
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;

        EXPECT_CALL(*messageParcelMock_, ReadInt32(_))
            .WillOnce(DoAll(SetArgReferee<0>(-1), Return(true)));

        int ret = callback.HandleOnSyncStateChanged(data, reply);
        EXPECT_EQ(ret, E_INVAL_ARG);
    } catch(...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " HandleOnSyncStateChangedTest004 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnSyncStateChangedTest004 End";
}

/**
 * @tc.name: HandleOnSyncStateChangedTest005
 * @tc.desc: Verify HandleOnSyncStateChanged function - stateValue > STOPPED
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnSyncStateChangedTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnSyncStateChangedTest005 Start";
    try {
        MockCallback callback;
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;
        EXPECT_CALL(*messageParcelMock_, ReadInt32(_))
            .WillOnce(DoAll(SetArgReferee<0>(6), Return(true)));

        int ret = callback.HandleOnSyncStateChanged(data, reply);
        EXPECT_EQ(ret, E_INVAL_ARG);
    } catch(...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " HandleOnSyncStateChangedTest005 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnSyncStateChangedTest005 End";
}

/**
 * @tc.name: HandleOnSyncStateChangedTest006
 * @tc.desc: Verify HandleOnSyncStateChanged function - errorValue < NO_ERROR
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnSyncStateChangedTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnSyncStateChangedTest006 Start";
    try {
        MockCallback callback;
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;
        EXPECT_CALL(*messageParcelMock_, ReadInt32(_))
            .WillOnce(DoAll(SetArgReferee<0>(0), Return(true)))
            .WillOnce(DoAll(SetArgReferee<0>(-1), Return(true)));
        
        int ret = callback.HandleOnSyncStateChanged(data, reply);
        EXPECT_EQ(ret, E_INVAL_ARG);
    } catch(...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " HandleOnSyncStateChangedTest006 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnSyncStateChangedTest006 End";
}

/**
 * @tc.name: HandleOnSyncStateChangedTest007
 * @tc.desc: Verify HandleOnSyncStateChanged function - errorValue > INNER_ERROR
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnSyncStateChangedTest007, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnSyncStateChangedTest007 Start";
    try {
        MockCallback callback;
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;
        EXPECT_CALL(*messageParcelMock_, ReadInt32(_))
            .WillOnce(DoAll(SetArgReferee<0>(0), Return(true)))
            .WillOnce(DoAll(SetArgReferee<0>(12), Return(true)));
        
        int ret = callback.HandleOnSyncStateChanged(data, reply);
        EXPECT_EQ(ret, E_INVAL_ARG);
    } catch(...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " HandleOnSyncStateChangedTest007 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnSyncStateChangedTest007 End";
}
/**
 * @tc.name: HandleOnCheckTriggeredTest001
 * @tc.desc: Verify HandleOnCheckTriggered fails when reason read fails.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnCheckTriggeredTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest001 Start";
    try {
        MockCheckTriggerCallback callback;
        MessageParcel data;
        MessageParcel reply;
        EXPECT_CALL(*messageParcelMock_, ReadInt32(_)).WillOnce(Return(false));
        int ret = callback.HandleOnCheckTriggered(data, reply);
        EXPECT_EQ(ret, E_INVAL_ARG);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest001 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest001 End";
}

/**
 * @tc.name: HandleOnCheckTriggeredTest002
 * @tc.desc: Verify HandleOnCheckTriggered rejects an invalid reason.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnCheckTriggeredTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest002 Start";
    try {
        MockCheckTriggerCallback callback;
        MessageParcel data;
        MessageParcel reply;
        EXPECT_CALL(*messageParcelMock_, ReadInt32(_))
            .WillOnce(DoAll(SetArgReferee<0>(-1), Return(true)));
        int ret = callback.HandleOnCheckTriggered(data, reply);
        EXPECT_EQ(ret, E_INVAL_ARG);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest002 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest002 End";
}

/**
 * @tc.name: HandleOnCheckTriggeredTest003
 * @tc.desc: Verify HandleOnCheckTriggered forwards a manual trigger callback.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnCheckTriggeredTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest003 Start";
    try {
        MockCheckTriggerCallback callback;
        MessageParcel data;
        MessageParcel reply;
        EXPECT_CALL(*messageParcelMock_, ReadInt32(_))
            .WillOnce(DoAll(SetArgReferee<0>(1), Return(true)));
        EXPECT_CALL(*messageParcelMock_, ReadString(_))
            .WillOnce(DoAll(SetArgReferee<0>("com.ohos.cloud"), Return(true)))
            .WillOnce(DoAll(SetArgReferee<0>("com.ohos.trigger"), Return(true)));
        EXPECT_CALL(callback,
            OnCheckTriggered(CheckTriggerReason::MANUAL, "com.ohos.cloud", "com.ohos.trigger"))
            .WillOnce(Return());
        int ret = callback.HandleOnCheckTriggered(data, reply);
        EXPECT_EQ(ret, E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest003 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest003 End";
}

/**
 * @tc.name: HandleOnCheckTriggeredTest004
 * @tc.desc: Verify HandleOnCheckTriggered forwards valid reason and trigger source bundleName.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnCheckTriggeredTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest004 Start";
    try {
        MockCheckTriggerCallback callback;
        MessageParcel data;
        MessageParcel reply;
        EXPECT_CALL(*messageParcelMock_, ReadInt32(_))
            .WillOnce(DoAll(SetArgReferee<0>(0), Return(true)));
        EXPECT_CALL(*messageParcelMock_, ReadString(_))
            .WillOnce(DoAll(SetArgReferee<0>("com.ohos.cloud"), Return(true)))
            .WillOnce(DoAll(SetArgReferee<0>("com.ohos.trigger"), Return(true)));
        EXPECT_CALL(callback,
            OnCheckTriggered(CheckTriggerReason::PERIODIC, "com.ohos.cloud", "com.ohos.trigger"))
            .WillOnce(Return());
        int ret = callback.HandleOnCheckTriggered(data, reply);
        EXPECT_EQ(ret, E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest004 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest004 End";
}

/**
 * @tc.name: HandleOnCheckTriggeredTest005
 * @tc.desc: Verify HandleOnCheckTriggered rejects an empty bundleName.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnCheckTriggeredTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest005 Start";
    try {
        MockCheckTriggerCallback callback;
        MessageParcel data;
        MessageParcel reply;
        EXPECT_CALL(*messageParcelMock_, ReadInt32(_))
            .WillOnce(DoAll(SetArgReferee<0>(0), Return(true)));
        EXPECT_CALL(*messageParcelMock_, ReadString(_))
            .WillOnce(DoAll(SetArgReferee<0>(""), Return(true)));
        int ret = callback.HandleOnCheckTriggered(data, reply);
        EXPECT_EQ(ret, E_INVAL_ARG);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest005 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest005 End";
}

/**
 * @tc.name: HandleOnCheckTriggeredTest006
 * @tc.desc: Verify HandleOnCheckTriggered rejects reason greater than MANUAL.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnCheckTriggeredTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest006 Start";
    try {
        MockCheckTriggerCallback callback;
        MessageParcel data;
        MessageParcel reply;
        EXPECT_CALL(*messageParcelMock_, ReadInt32(_))
            .WillOnce(DoAll(SetArgReferee<0>(2), Return(true)));
        int ret = callback.HandleOnCheckTriggered(data, reply);
        EXPECT_EQ(ret, E_INVAL_ARG);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest006 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest006 End";
}

/**
 * @tc.name: HandleOnCheckTriggeredTest007
 * @tc.desc: Verify HandleOnCheckTriggered fails when bundleName read fails.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnCheckTriggeredTest007, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest007 Start";
    try {
        MockCheckTriggerCallback callback;
        MessageParcel data;
        MessageParcel reply;
        EXPECT_CALL(*messageParcelMock_, ReadInt32(_))
            .WillOnce(DoAll(SetArgReferee<0>(0), Return(true)));
        EXPECT_CALL(*messageParcelMock_, ReadString(_)).WillOnce(Return(false));
        int ret = callback.HandleOnCheckTriggered(data, reply);
        EXPECT_EQ(ret, E_INVAL_ARG);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest007 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest007 End";
}

/**
 * @tc.name: HandleOnCheckTriggeredTest008
 * @tc.desc: Verify HandleOnCheckTriggered fails when triggerBundleName read fails.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudSyncCallbackStubTest, HandleOnCheckTriggeredTest008, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest008 Start";
    try {
        MockCheckTriggerCallback callback;
        MessageParcel data;
        MessageParcel reply;
        EXPECT_CALL(*messageParcelMock_, ReadInt32(_))
            .WillOnce(DoAll(SetArgReferee<0>(0), Return(true)));
        EXPECT_CALL(*messageParcelMock_, ReadString(_))
            .WillOnce(DoAll(SetArgReferee<0>("com.ohos.cloud"), Return(true)))
            .WillOnce(Return(false));
        int ret = callback.HandleOnCheckTriggered(data, reply);
        EXPECT_EQ(ret, E_INVAL_ARG);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest008 ERROR";
    }
    GTEST_LOG_(INFO) << "HandleOnCheckTriggeredTest008 End";
}

/**
 * @tc.name: OnRemoteRequestTest001
 * @tc.desc: Verify CheckTriggerCallbackStub handles invalid tokens, unknown transactions and null handlers.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudSyncCallbackStubTest, OnRemoteRequestTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnRemoteRequestTest001 Start";
    try {
        MockCheckTriggerCallback callback;
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;
        EXPECT_CALL(*messageParcelMock_, ReadInterfaceToken()).WillOnce(Return(u"invalid"));
        EXPECT_EQ(callback.OnRemoteRequest(CheckTriggerCallbackStub::SERVICE_CMD_ON_CHECK_TRIGGERED,
            data, reply, option), E_SERVICE_DESCRIPTOR_IS_EMPTY);

        Mock::VerifyAndClearExpectations(messageParcelMock_.get());
        EXPECT_CALL(*messageParcelMock_, ReadInterfaceToken()).WillOnce(Return(callback.GetDescriptor()));
        EXPECT_NE(callback.OnRemoteRequest(99, data, reply, option), E_OK);

        Mock::VerifyAndClearExpectations(messageParcelMock_.get());
        callback.opToInterfaceMap_[CheckTriggerCallbackStub::SERVICE_CMD_ON_CHECK_TRIGGERED] = nullptr;
        EXPECT_CALL(*messageParcelMock_, ReadInterfaceToken()).WillOnce(Return(callback.GetDescriptor()));
        EXPECT_NE(callback.OnRemoteRequest(CheckTriggerCallbackStub::SERVICE_CMD_ON_CHECK_TRIGGERED,
            data, reply, option), E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnRemoteRequestTest001 ERROR";
    }
    GTEST_LOG_(INFO) << "OnRemoteRequestTest001 End";
}

/**
 * @tc.name: OnRemoteRequestTest002
 * @tc.desc: Verify CheckTriggerCallbackStub dispatches a valid request.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudSyncCallbackStubTest, OnRemoteRequestTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnRemoteRequestTest002 Start";
    try {
        MockCheckTriggerCallback callback;
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;
        EXPECT_CALL(*messageParcelMock_, ReadInterfaceToken()).WillOnce(Return(callback.GetDescriptor()));
        EXPECT_CALL(*messageParcelMock_, ReadInt32(_))
            .WillOnce(DoAll(SetArgReferee<0>(0), Return(true)));
        EXPECT_CALL(*messageParcelMock_, ReadString(_))
            .WillOnce(DoAll(SetArgReferee<0>("com.ohos.cloud"), Return(true)))
            .WillOnce(DoAll(SetArgReferee<0>("com.ohos.trigger"), Return(true)));
        EXPECT_CALL(callback,
            OnCheckTriggered(CheckTriggerReason::PERIODIC, "com.ohos.cloud", "com.ohos.trigger"))
            .WillOnce(Return());
        EXPECT_EQ(callback.OnRemoteRequest(CheckTriggerCallbackStub::SERVICE_CMD_ON_CHECK_TRIGGERED,
            data, reply, option), E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnRemoteRequestTest002 ERROR";
    }
    GTEST_LOG_(INFO) << "OnRemoteRequestTest002 End";
}
} // namespace Test
} // namespace FileManagement::CloudSync {
} // namespace OHOS
