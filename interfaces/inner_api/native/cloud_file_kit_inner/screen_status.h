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

#ifndef OHOS_FILEMGMT_CLOUD_SYNC_SCREEN_STATUS_H
#define OHOS_FILEMGMT_CLOUD_SYNC_SCREEN_STATUS_H

#include "common_event_subscriber.h"

namespace OHOS {
namespace FileManagement {
namespace CloudSync {
class ScreenStatus {
public:
    enum ScreenState {
        SCREEN_ON,
        SCREEN_OFF,
    };
    static bool IsScreenOn();
    static void InitScreenStatus();
    static void SetScreenState(ScreenState screenState);
    static bool IsForceSleep();

private:
    static inline ScreenState screenState_{ScreenState::SCREEN_ON};
};
} // namespace CloudSync
} // namespace FileManagement
} // namespace OHOS

#endif // OHOS_FILEMGMT_CLOUD_SYNC_SCREEN_STATUS_H