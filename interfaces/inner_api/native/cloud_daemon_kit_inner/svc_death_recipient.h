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

#ifndef OHOS_STORAGE_SVC_DEATH_RECIPIENT_H
#define OHOS_STORAGE_SVC_DEATH_RECIPIENT_H

#include <functional>

#include "iremote_object.h"

namespace OHOS::FileManagement::CloudFile {
class SvcDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    explicit SvcDeathRecipient(std::function<void(const wptr<IRemoteObject> &)> functor) : functor_(functor){};
    void OnRemoteDied(const wptr<IRemoteObject> &object) override
    {
        if (object == nullptr) {
            return;
        }
        object->RemoveDeathRecipient(this);
        functor_(object);
    }

private:
    std::function<void(const wptr<IRemoteObject> &)> functor_;
};
} // namespace OHOS::FileManagement::CloudFile

#endif // OHOS_STORAGE_SVC_DEATH_RECIPIENT_H
