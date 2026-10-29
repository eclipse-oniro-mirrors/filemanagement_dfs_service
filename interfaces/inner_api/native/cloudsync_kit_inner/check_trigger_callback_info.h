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

#ifndef OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_INFO_H
#define OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_INFO_H

#include <memory>
#include <string>

#include "cloud_check_trigger_callback.h"

namespace OHOS::FileManagement::CloudSync {
struct CheckTriggerCallbackInfo {
    std::string callbackId;
    std::shared_ptr<CloudCheckTriggerCallback> callback{nullptr};
    std::string bundleName;
};
} // namespace OHOS::FileManagement::CloudSync

#endif // OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_INFO_H
