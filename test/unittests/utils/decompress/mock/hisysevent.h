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
#ifndef HI_SYS_EVENT_H
#define HI_SYS_EVENT_H

#include <cstdint>

namespace OHOS {
namespace HiviewDFX {

struct HiSysEventMockState {
    int32_t ret = 0;
    bool enabled = false;
    int32_t callCount = 0;
};

inline HiSysEventMockState &GetHiSysEventMockState()
{
    static HiSysEventMockState state;
    return state;
}

class HiSysEvent {
public:
    enum EventType { FAULT = 1, STATISTIC = 2, SECURITY = 3, BEHAVIOR = 4 };

    struct Domain {
        static constexpr const char *FILEMANAGEMENT = "FILEMANAGEMENT";
    };
};

#define HiSysEventWrite(domain, eventName, type, ...) \
    ({ \
        (void)domain; \
        (void)eventName; \
        (void)type; \
        OHOS::HiviewDFX::HiSysEventMockState &_mockState = OHOS::HiviewDFX::GetHiSysEventMockState(); \
        _mockState.callCount++; \
        (_mockState.enabled ? _mockState.ret : 0); \
    })

} // namespace HiviewDFX
} // namespace OHOS

#endif // HI_SYS_EVENT_H
