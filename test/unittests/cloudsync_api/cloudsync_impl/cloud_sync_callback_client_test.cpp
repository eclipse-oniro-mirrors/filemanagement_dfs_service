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

#include "cloud_sync_constants.h"
#include "dfs_error.h"
#include "check_trigger_callback_client.h"
#include "cloud_sync_callback_client.h"
#include "service_callback_mock.h"

namespace OHOS {
namespace FileManagement::CloudSync {
namespace Test {
using namespace testing::ext;
using namespace std;

class CloudSyncCallbackTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    sptr<CloudSyncCallbackClient> callbackClient_;
    void InitCloudSyncCallback();
};

class CloudSyncCallbackDerived : public CloudSyncCallback {
public:
    void OnSyncStateChanged(CloudSyncState state, ErrorType error) override
    {
        GTEST_LOG_(INFO) << "OnSyncStateChanged SUCCESS";
    }
};

class CheckTriggerCallbackDerived : public CloudCheckTriggerCallback {
public:
    void OnCheckTriggered(CheckTriggerReason reason, const std::string &bundleName,
        const std::string &triggerBundleName) override
    {
        reason_ = reason;
        bundleName_ = bundleName;
        triggerBundleName_ = triggerBundleName;
        GTEST_LOG_(INFO) << "OnCheckTriggered SUCCESS";
    }

    CheckTriggerReason reason_{CheckTriggerReason::MANUAL};
    std::string bundleName_;
    std::string triggerBundleName_;
};

void CloudSyncCallbackTest::InitCloudSyncCallback(void)
{
    shared_ptr<CloudSyncCallback> callback = make_shared<CloudSyncCallbackDerived>();
    callbackClient_ = new CloudSyncCallbackClient(callback);
}

void CloudSyncCallbackTest::SetUpTestCase(void)
{
    std::cout << "SetUpTestCase" << std::endl;
}

void CloudSyncCallbackTest::TearDownTestCase(void)
{
    std::cout << "TearDownTestCase" << std::endl;
}

void CloudSyncCallbackTest::SetUp(void)
{
    std::cout << "SetUp" << std::endl;
}

void CloudSyncCallbackTest::TearDown(void)
{
    std::cout << "TearDown" << std::endl;
}

/**
 * @tc.name: OnSyncStateChangedTest
 * @tc.desc: Verify the OnSyncStateChanged function.
 * @tc.type: FUNC
 * @tc.require: I6H5MH
 */
HWTEST_F(CloudSyncCallbackTest, OnSyncStateChangedTest, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnSyncStateChanged start";
    try {
        InitCloudSyncCallback();
        callbackClient_->OnSyncStateChanged(CloudSyncState::COMPLETED, ErrorType::NO_ERROR);
        EXPECT_TRUE(true);
        callbackClient_->callback_ = nullptr;
        callbackClient_->OnSyncStateChanged(CloudSyncState::COMPLETED, ErrorType::NO_ERROR);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnSyncStateChanged FAILED";
    }
    GTEST_LOG_(INFO) << "OnSyncStateChanged end";
}

/**
 * @tc.name: OnCheckTriggeredTest
 * @tc.desc: Verify the CheckTriggerCallbackClient OnCheckTriggered forwarding.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudSyncCallbackTest, OnCheckTriggeredTest, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest start";
    try {
        auto callback = make_shared<CheckTriggerCallbackDerived>();
        sptr<CheckTriggerCallbackClient> client = new CheckTriggerCallbackClient(callback);
        client->OnCheckTriggered(CheckTriggerReason::PERIODIC, "com.ohos.cloud", "com.ohos.trigger");
        EXPECT_EQ(callback->reason_, CheckTriggerReason::PERIODIC);
        EXPECT_EQ(callback->bundleName_, "com.ohos.cloud");
        EXPECT_EQ(callback->triggerBundleName_, "com.ohos.trigger");

        client->callback_ = nullptr;
        client->OnCheckTriggered(CheckTriggerReason::MANUAL, "com.ohos.other", "com.ohos.otherTrigger");
        EXPECT_EQ(callback->reason_, CheckTriggerReason::PERIODIC);
        EXPECT_EQ(callback->bundleName_, "com.ohos.cloud");
        EXPECT_EQ(callback->triggerBundleName_, "com.ohos.trigger");
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnCheckTriggeredTest FAILED";
    }
    GTEST_LOG_(INFO) << "OnCheckTriggeredTest end";
}

} // namespace Test
} // namespace FileManagement::CloudSync {
} // namespace OHOS
