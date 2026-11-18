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

#ifndef CLOUD_DISK_SERVICE_RS_CALLBACK_REMOTE_H
#define CLOUD_DISK_SERVICE_RS_CALLBACK_REMOTE_H

#include <cstddef>
#include <memory>

#include "cxx.h"
#include "iremote_object.h"
#include "message_parcel.h"

namespace OHOS {
using SptrIRemoteObject = sptr<IRemoteObject>;
}

namespace OHOS::FileManagement::CloudDiskService {
class CallbackRemote final {
public:
    explicit CallbackRemote(sptr<IRemoteObject> remote);
    ~CallbackRemote();
    CallbackRemote(const CallbackRemote &) = delete;
    CallbackRemote &operator=(const CallbackRemote &) = delete;

    std::size_t ResolveTableKey();
    std::size_t ResolveProgressKey();
    std::unique_ptr<SptrIRemoteObject> CloneObject() const;
    bool WatchDeath(rust::Fn<void(std::size_t)> onDeath);

private:
    sptr<IRemoteObject> remote_;
    sptr<IRemoteObject::DeathRecipient> recipient_;
};

std::unique_ptr<CallbackRemote> ReadCallbackRemote(MessageParcel &parcel);
} // namespace OHOS::FileManagement::CloudDiskService

#endif // CLOUD_DISK_SERVICE_RS_CALLBACK_REMOTE_H
