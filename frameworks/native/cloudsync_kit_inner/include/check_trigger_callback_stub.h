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

#ifndef OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_STUB_H
#define OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_STUB_H

#include <functional>
#include <map>

#include "i_check_trigger_callback.h"
#include "iremote_stub.h"

namespace OHOS::FileManagement::CloudSync {
using CheckTriggerServiceInterface = std::function<int32_t(MessageParcel &data, MessageParcel &reply)>;

class CheckTriggerCallbackStub : public IRemoteStub<ICheckTriggerCallback> {
public:
    CheckTriggerCallbackStub();
    virtual ~CheckTriggerCallbackStub() = default;
    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    std::map<uint32_t, CheckTriggerServiceInterface> opToInterfaceMap_;

    int32_t HandleOnCheckTriggered(MessageParcel &data, MessageParcel &reply);
};
} // namespace OHOS::FileManagement::CloudSync

#endif // OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_STUB_H
