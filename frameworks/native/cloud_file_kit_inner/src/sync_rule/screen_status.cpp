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

#include "screen_status.h"

#include "common_event_manager.h"
#include "common_event_support.h"
#include "dfs_error.h"
#include "power_mgr_client.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement {
namespace CloudSync {
bool ScreenStatus::IsScreenOn()
{
    return screenState_ == ScreenState::SCREEN_ON;
}

void ScreenStatus::InitScreenStatus()
{
    bool isScreenOn = PowerMgr::PowerMgrClient::GetInstance().IsScreenOn();
    screenState_ =  isScreenOn? ScreenState::SCREEN_ON : ScreenState::SCREEN_OFF;
}

void ScreenStatus::SetScreenState(ScreenState screenState)
{
    screenState_ = screenState;
}

bool ScreenStatus::IsForceSleep()
{
    bool isForceSleep = PowerMgr::PowerMgrClient::GetInstance().IsForceSleeping();
    return isForceSleep;
}
} // namespace CloudSync
} // namespace FileManagement
} // namespace OHOS