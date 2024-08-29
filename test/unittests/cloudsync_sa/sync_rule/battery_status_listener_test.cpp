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

#include "common_event_manager.h"
#include "common_event_support.h"
#include "battery_status.h"
#include "battery_status_listener.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync::Test {
using namespace testing;
using namespace testing::ext;
using namespace std;
using namespace CloudFile;
using Want = OHOS::AAFwk::Want;

class BatteryStatusListenerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};
void BatteryStatusListenerTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
}

void BatteryStatusListenerTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
}

void BatteryStatusListenerTest::SetUp(void)
{
    GTEST_LOG_(INFO) << "SetUp";
}

void BatteryStatusListenerTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "TearDown";
}

/**
 * @tc.name: StopTest
 * @tc.desc: Verify the Stop function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(BatteryStatusListenerTest, StopTest, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StopTest Start";
    try {
        auto dataSyncManager = std::make_shared<DataSyncManager>();
        auto batteryStatusListener = std::make_shared<BatteryStatusListener>(dataSyncManager);
        batteryStatusListener->Start();
        batteryStatusListener->Stop();
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " StopTest FAILED";
    }
    GTEST_LOG_(INFO) << "StopTest End";
}

/**
 * @tc.name: OnStatusAbnormalTest
 * @tc.desc: Verify the OnStatusAbnormal function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(BatteryStatusListenerTest, OnStatusAbnormalTest, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnStatusAbnormalTest Start";
    try {
        auto dataSyncManager = std::make_shared<DataSyncManager>();
        auto batteryStatusListener = std::make_shared<BatteryStatusListener>(dataSyncManager);
        batteryStatusListener->OnStatusAbnormal();
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " OnStatusAbnormalTest FAILED";
    }
    GTEST_LOG_(INFO) << "OnStatusAbnormalTest End";
}

/**
 * @tc.name: OnReceiveEventTest
 * @tc.desc: Verify the OnReceiveEvent function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(BatteryStatusListenerTest, OnReceiveEventTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnReceiveEventTest001 Start";
    try {
        Want want;
        want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_BATTERY_OKAY);
        EventFwk::CommonEventData eventData(want);
        auto dataSyncManager = std::make_shared<DataSyncManager>();
        auto batteryStatusListener = std::make_shared<BatteryStatusListener>(dataSyncManager);
        auto subscriber = std::make_shared<BatteryStatusSubscriber>(EventFwk::CommonEventSubscribeInfo(),
            batteryStatusListener);
        subscriber->OnReceiveEvent(eventData);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " OnReceiveEventTest001 FAILED";
    }
    GTEST_LOG_(INFO) << "OnReceiveEventTest001 End";
}

/**
 * @tc.name: OnReceiveEventTest
 * @tc.desc: Verify the OnReceiveEvent function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(BatteryStatusListenerTest, OnReceiveEventTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnReceiveEventTest002 Start";
    try {
        Want want;
        want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_CHARGING);
        EventFwk::CommonEventData eventData(want);
        auto dataSyncManager = std::make_shared<DataSyncManager>();
        auto batteryStatusListener = std::make_shared<BatteryStatusListener>(dataSyncManager);
        auto subscriber = std::make_shared<BatteryStatusSubscriber>(EventFwk::CommonEventSubscribeInfo(),
            batteryStatusListener);
        subscriber->OnReceiveEvent(eventData);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " OnReceiveEventTest002 FAILED";
    }
    GTEST_LOG_(INFO) << "OnReceiveEventTest002 End";
}

/**
 * @tc.name: OnReceiveEventTest
 * @tc.desc: Verify the OnReceiveEvent function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(BatteryStatusListenerTest, OnReceiveEventTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnReceiveEventTest003 Start";
    try {
        Want want;
        want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_DISCHARGING);
        EventFwk::CommonEventData eventData(want);
        auto dataSyncManager = std::make_shared<DataSyncManager>();
        auto batteryStatusListener = std::make_shared<BatteryStatusListener>(dataSyncManager);
        auto subscriber = std::make_shared<BatteryStatusSubscriber>(EventFwk::CommonEventSubscribeInfo(),
            batteryStatusListener);
        subscriber->OnReceiveEvent(eventData);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " OnReceiveEventTest003 FAILED";
    }
    GTEST_LOG_(INFO) << "OnReceiveEventTest003 End";
}

/**
 * @tc.name: OnReceiveEventTest
 * @tc.desc: Verify the OnReceiveEvent function
 * @tc.type: FUNC
 * @tc.require: I6JPKG
 */
HWTEST_F(BatteryStatusListenerTest, OnReceiveEventTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnReceiveEventTest004 Start";
    try {
        Want want;
        want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_POWER_CONNECTED);
        EventFwk::CommonEventData eventData(want);
        auto dataSyncManager = std::make_shared<DataSyncManager>();
        auto batteryStatusListener = std::make_shared<BatteryStatusListener>(dataSyncManager);
        auto subscriber = std::make_shared<BatteryStatusSubscriber>(EventFwk::CommonEventSubscribeInfo(),
            batteryStatusListener);
        subscriber->OnReceiveEvent(eventData);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " OnReceiveEventTest004 FAILED";
    }
    GTEST_LOG_(INFO) << "OnReceiveEventTest004 End";
}
} // namespace OHOS::FileManagement::CloudSync::Test
