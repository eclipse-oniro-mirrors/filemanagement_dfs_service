/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_FILEMGMT_CLOUD_SYNC_CALLBACK_ANI_H
#define OHOS_FILEMGMT_CLOUD_SYNC_CALLBACK_ANI_H

#include <ani.h>
#include <ani_signature_builder.h>

#include "cloud_sync_callback_middle.h"
#include "cloud_optimize_callback_middle.h"
#include "data_ability_observer_interface.h"
#include "data_ability_observer_stub.h"
#include "dataobs_mgr_client.h"
#include "securec.h"
#include "uri.h"

namespace OHOS::FileManagement::CloudSync {

class CloudNotifyObserver;

struct BundleEntity {
    std::string bundleName_;
    explicit BundleEntity(const std::string bundleName) : bundleName_(bundleName) {};
};

struct CloudChangeListener {
    NotifyType notifyType;
    AAFwk::ChangeInfo changeInfo;
    std::string strUri;
};

struct RegisterParams {
    std::string uri;
    bool recursion;
    ani_ref cbOnRef;
};

class ObserverImpl : public AAFwk::DataAbilityObserverStub {
public:
    explicit ObserverImpl(const std::shared_ptr<CloudNotifyObserver> cloudNotifyObserver)
        : cloudNotifyObserver_(cloudNotifyObserver) {};
    void OnChange();
    void OnChangeExt(const AAFwk::ChangeInfo &info);
    static sptr<ObserverImpl> GetObserver(const Uri &uri, const std::shared_ptr<CloudNotifyObserver> &observer);
    static bool FindObserver(const Uri &uri, const std::shared_ptr<CloudNotifyObserver> &observer);
    static bool DeleteObserver(const Uri &uri, const std::shared_ptr<CloudNotifyObserver> &observer);

private:
    struct ObserverParam {
        sptr<ObserverImpl> obs_;
        std::list<Uri> uris_;
    };
    std::shared_ptr<CloudNotifyObserver> cloudNotifyObserver_;
    static ConcurrentMap<CloudNotifyObserver *, ObserverParam> observers_;
};

class CloudSyncCallbackAniImpl : public CloudSyncCallbackMiddle,
                                 public std::enable_shared_from_this<CloudSyncCallbackAniImpl> {
public:
    CloudSyncCallbackAniImpl(ani_env *env, ani_ref fun);
    ~CloudSyncCallbackAniImpl() override = default;
    void OnSyncStateChanged(SyncType type, SyncPromptState state) override;
    void OnSyncStateChanged(CloudSyncState state, ErrorType error) override;
    void DeleteReference() override;

private:
    void GetSyncProgress(CloudSyncState state, ErrorType error, const ani_class &cls, ani_object &pg);
    ani_vm *vm_ = nullptr;
    ani_ref cbOnRef_ = nullptr;
};

class ChangeListenerAni {
public:
    explicit ChangeListenerAni(ani_env *env)
    {
        env_ = env;
    }

    ChangeListenerAni(const ChangeListenerAni &listener)
    {
        this->env_ = listener.env_;
        this->cbOnRef_ = listener.cbOnRef_;
        this->cbOffRef_ = listener.cbOffRef_;
        this->observers_ = listener.observers_;
    }

    ChangeListenerAni &operator=(const ChangeListenerAni &listener)
    {
        this->env_ = listener.env_;
        this->cbOnRef_ = listener.cbOnRef_;
        this->cbOffRef_ = listener.cbOffRef_;
        this->observers_ = listener.observers_;
        return *this;
    }

    ~ChangeListenerAni() {};
    void OnChange(CloudChangeListener &listener, const ani_ref cbRef);
    ani_status GetChangeDataObject(
        ani_env *env, CloudChangeListener &listener, ani_class cls, ani_object &changeData);
    ani_status SetIsDir(ani_env *env, const std::shared_ptr<MessageParcel> parcel, ani_object &isDirectory);
    ani_status SetValueArray(ani_env *env, const std::list<Uri> listValue, ani_object &uris);
    ani_ref cbOnRef_ = nullptr;
    ani_ref cbOffRef_ = nullptr;
    std::vector<std::shared_ptr<CloudNotifyObserver>> observers_;

private:
    ani_env *env_;
};

class CloudNotifyObserver {
public:
    CloudNotifyObserver(const ChangeListenerAni &listObj, std::string uri, ani_ref ref)
        : listObj_(listObj), uri_(uri), ref_(ref)
    {
    }
    ~CloudNotifyObserver() = default;

    void OnChange();
    void OnChangeExt(const AAFwk::ChangeInfo &changeInfo);

    ChangeListenerAni listObj_;
    std::string uri_;
    ani_ref ref_;
};

class CloudOptimizeCallbackAniImpl : public CloudOptimizeCallbackMiddle,
                                  public std::enable_shared_from_this<CloudOptimizeCallbackAniImpl> {
public:
    CloudOptimizeCallbackAniImpl(ani_env *env, ani_ref fun) : env_(env), cbOnRef_(fun) {}
    ~CloudOptimizeCallbackAniImpl() = default;
    void OnOptimizeProcess(const OptimizeState state, const int32_t progress) override;

private:
    ani_status GetOptimProgress(ani_env *env, OptimizeState state, int32_t progress, ani_class cls, ani_object &data);
    ani_env *env_;
    ani_ref cbOnRef_ = nullptr;
};
} // namespace OHOS::FileManagement::CloudSync
#endif // OHOS_FILEMGMT_CLOUD_SYNC_CALLBACK_ANI_H