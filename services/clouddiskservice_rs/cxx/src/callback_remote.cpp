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

#include "callback_remote.h"

#include <cstdint>
#include <new>
#include <utility>

#include "i_cloud_disk_progress_callback.h"
#include "i_cloud_disk_service_callback_table.h"

namespace OHOS::FileManagement::CloudDiskService {
namespace {
class CallbackDeathRecipient final : public IRemoteObject::DeathRecipient {
public:
    explicit CallbackDeathRecipient(rust::Fn<void(std::size_t)> onDeath) : onDeath_(onDeath) {}

    void OnRemoteDied(const wptr<IRemoteObject> &object) override
    {
        auto remote = object.promote();
        if (remote != nullptr) {
            onDeath_(reinterpret_cast<std::uintptr_t>(remote.GetRefPtr()));
        }
    }

private:
    rust::Fn<void(std::size_t)> onDeath_;
};
template<typename Interface>
std::size_t ResolveCallbackKey(sptr<IRemoteObject> &remote)
{
    if (!remote->IsProxyObject()) {
        auto callback = iface_cast<Interface>(remote);
        auto object = callback == nullptr ? nullptr : callback->AsObject();
        if (object == nullptr) {
            return 0;
        }
        remote = std::move(object);
    }
    return reinterpret_cast<std::uintptr_t>(remote.GetRefPtr());
}
} // namespace

CallbackRemote::CallbackRemote(sptr<IRemoteObject> remote) : remote_(std::move(remote)) {}

CallbackRemote::~CallbackRemote()
{
    if (recipient_ != nullptr && !remote_->IsObjectDead()) {
        remote_->RemoveDeathRecipient(recipient_);
    }
}

std::size_t CallbackRemote::ResolveTableKey()
{
    return ResolveCallbackKey<ICloudDiskServiceCallbackTable>(remote_);
}

std::size_t CallbackRemote::ResolveProgressKey()
{
    return ResolveCallbackKey<ICloudDiskProgressCallback>(remote_);
}

std::unique_ptr<SptrIRemoteObject> CallbackRemote::CloneObject() const
{
    return std::make_unique<SptrIRemoteObject>(remote_);
}

bool CallbackRemote::WatchDeath(rust::Fn<void(std::size_t)> onDeath)
{
    if (recipient_ != nullptr) {
        return true;
    }
    sptr<IRemoteObject::DeathRecipient> recipient = new (std::nothrow) CallbackDeathRecipient(onDeath);
    if (recipient == nullptr || !remote_->AddDeathRecipient(recipient)) {
        return false;
    }
    recipient_ = std::move(recipient);
    return true;
}

std::unique_ptr<CallbackRemote> ReadCallbackRemote(MessageParcel &parcel)
{
    auto remote = parcel.ReadRemoteObject();
    if (remote == nullptr) {
        return nullptr;
    }
    return std::make_unique<CallbackRemote>(std::move(remote));
}
} // namespace OHOS::FileManagement::CloudDiskService
