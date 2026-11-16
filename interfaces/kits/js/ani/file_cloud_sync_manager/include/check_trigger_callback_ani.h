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

#ifndef OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_ANI_H
#define OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_ANI_H

#include <ani.h>

#include <memory>
#include <string>

#include "cloud_check_trigger_callback.h"

namespace OHOS::FileManagement::CloudSync {
class CheckTriggerCallbackAni : public CloudCheckTriggerCallback,
                                public std::enable_shared_from_this<CheckTriggerCallbackAni> {
public:
    CheckTriggerCallbackAni(ani_env *env, ani_ref fun);
    ~CheckTriggerCallbackAni() override;
    void OnCheckTriggered(CheckTriggerReason reason, const std::string &bundleName,
        const std::string &triggerBundleName) override;
    void DeleteReference() override;

private:
    ani_status GetCheckTriggerInfo(CheckTriggerReason reason, const std::string &bundleName,
        const std::string &triggerBundleName, const ani_class &cls, ani_object &info);
    ani_vm *vm_ = nullptr;
    ani_ref cbOnRef_ = nullptr;
};
} // namespace OHOS::FileManagement::CloudSync

#endif // OHOS_FILEMGMT_CHECK_TRIGGER_CALLBACK_ANI_H
