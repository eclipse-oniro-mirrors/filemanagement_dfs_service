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
#include "battersrvclient_mock.h"
#include "battery_status.h"
#include "battery_status_listener.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync::Test {
using namespace testing;
using namespace testing::ext;
using namespace std;
using namespace CloudFile;
using Want = OHOS::AAFwk::Want;
using ChargeState = PowerMgr::BatteryChargeState;
using PluggedType = PowerMgr::BatteryPluggedType;

class BatteryStatusMock final : public BatteryStatus {
public:
    BatteryStatusMock() : BatteryStatus() {}
};

class BatteryStatusListenerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    shared_ptr<BatteryStatusMock> batteryStatus_ = nullptr;
    static inline shared_ptr<BatterySrvClientMock> dfsBatterySrvClient_ = nullptr;
};
void BatteryStatusListenerTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
    dfsBatterySrvClient_ = make_shared<BatterySrvClientMock>();
    BatterySrvClientMock::dfsBatterySrvClient = dfsBatterySrvClient_;
}

void BatteryStatusListenerTest::TearDownTestCase(void)
{
    BatterySrvClientMock::dfsBatterySrvClient = nullptr;
    dfsBatterySrvClient_ = nullptr;
    GTEST_LOG_(INFO) << "TearDownTestCase";
}

void BatteryStatusListenerTest::SetUp(void)
{
    GTEST_LOG_(INFO) << "SetUp";
    batteryStatus_ = make_shared<BatteryStatusMock>();
}

void BatteryStatusListenerTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "TearDown";
    batteryStatus_ = nullptr;
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
        EXPECT_CALL(*dfsBatterySrvClient_, GetChargingStatus())
            .WillOnce(Return(ChargeState::CHARGE_STATE_ENABLE));
        subscriber->OnReceiveEvent(eventData);
        EXPECT_TRUE(batteryStatus_->IsCharging());
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
        want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_DISCHARGING);
        EventFwk::CommonEventData eventData(want);
        auto dataSyncManager = std::make_shared<DataSyncManager>();
        auto batteryStatusListener = std::make_shared<BatteryStatusListener>(dataSyncManager);
        auto subscriber = std::make_shared<BatteryStatusSubscriber>(EventFwk::CommonEventSubscribeInfo(),
            batteryStatusListener);
        EXPECT_CALL(*dfsBatterySrvClient_, GetChargingStatus())
            .WillOnce(Return(ChargeState::CHARGE_STATE_DISABLE));
        EXPECT_CALL(*dfsBatterySrvClient_, GetPluggedType())
            .WillOnce(Return(PluggedType::PLUGGED_TYPE_USB));
        subscriber->OnReceiveEvent(eventData);
        EXPECT_TRUE(batteryStatus_->IsCharging());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " OnReceiveEventTest004 FAILED";
    }
    GTEST_LOG_(INFO) << "OnReceiveEventTest004 End";
}

/**
 * @tc.name: OnReceiveEventTest
 * @tc.desc: Verify the OnReceiveEvent function
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStatusListenerTest, OnReceiveEventTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnReceiveEventTest005 Start";
    try {
        Want want;
        want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_DISCHARGING);
        EventFwk::CommonEventData eventData(want);
        auto dataSyncManager = std::make_shared<DataSyncManager>();
        auto batteryStatusListener = std::make_shared<BatteryStatusListener>(dataSyncManager);
        auto subscriber = std::make_shared<BatteryStatusSubscriber>(EventFwk::CommonEventSubscribeInfo(),
            batteryStatusListener);
        EXPECT_CALL(*dfsBatterySrvClient_, GetChargingStatus())
            .WillOnce(Return(ChargeState::CHARGE_STATE_DISABLE));
        EXPECT_CALL(*dfsBatterySrvClient_, GetPluggedType())
            .WillOnce(Return(PluggedType::PLUGGED_TYPE_NONE));
        subscriber->OnReceiveEvent(eventData);
        EXPECT_FALSE(batteryStatus_->IsCharging());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " OnReceiveEventTest005 FAILED";
    }
    GTEST_LOG_(INFO) << "OnReceiveEventTest005 End";
}

/**
 * @tc.name: OnReceiveEventTest
 * @tc.desc: Verify the OnReceiveEvent function
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStatusListenerTest, OnReceiveEventTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnReceiveEventTest006 Start";
    try {
        Want want;
        want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_POWER_CONNECTED);
        EventFwk::CommonEventData eventData(want);
        auto dataSyncManager = std::make_shared<DataSyncManager>();
        auto batteryStatusListener = std::make_shared<BatteryStatusListener>(dataSyncManager);
        auto subscriber = std::make_shared<BatteryStatusSubscriber>(EventFwk::CommonEventSubscribeInfo(),
            batteryStatusListener);
        batteryStatus_->SetChargingStatus(true);
        subscriber->OnReceiveEvent(eventData);
        EXPECT_TRUE(batteryStatus_->IsCharging());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " OnReceiveEventTest006 FAILED";
    }
    GTEST_LOG_(INFO) << "OnReceiveEventTest006 End";
}

/**
 * @tc.name: OnPowerConnectedTest001
 * @tc.desc: Verify the OnPowerConnected function
 * @tc.type: FUNC
 * @tc.require: IB3SWZ
 */
HWTEST_F(BatteryStatusListenerTest, OnPowerConnectedTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnPowerConnectedTest001 start";
    try {
        auto dataSyncManager = std::make_shared<DataSyncManager>();
        auto batteryStatusListener = std::make_shared<BatteryStatusListener>(dataSyncManager);
        batteryStatusListener->OnPowerConnected();
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << " OnPowerConnectedTest001 failed";
    }
    GTEST_LOG_(INFO) << "OnPowerConnectedTest001 end";
}
} // namespace OHOS::FileManagement::CloudSync::Test
