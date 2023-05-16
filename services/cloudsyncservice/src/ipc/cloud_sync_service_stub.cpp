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
#include "ipc/cloud_sync_service_stub.h"
#include "dfs_error.h"
#include "dfsu_access_token_helper.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudSync {
using namespace std;

CloudSyncServiceStub::CloudSyncServiceStub()
{
    opToInterfaceMap_[SERVICE_CMD_REGISTER_CALLBACK] = &CloudSyncServiceStub::HandleRegisterCallbackInner;
    opToInterfaceMap_[SERVICE_CMD_START_SYNC] = &CloudSyncServiceStub::HandleStartSyncInner;
    opToInterfaceMap_[SERVICE_CMD_STOP_SYNC] = &CloudSyncServiceStub::HandleStopSyncInner;
    opToInterfaceMap_[SERVICE_CMD_CHANGE_APP_SWITCH] = &CloudSyncServiceStub::HandleChangeAppSwitch;
    opToInterfaceMap_[SERVICE_CMD_CLEAN] = &CloudSyncServiceStub::HandleClean;
    opToInterfaceMap_[SERVICE_CMD_NOTIFY_DATA_CHANGE] = &CloudSyncServiceStub::HandleNotifyDataChange;
    opToInterfaceMap_[SERVICE_CMD_ENABLE_CLOUD] = &CloudSyncServiceStub::HandleEnableCloud;
    opToInterfaceMap_[SERVICE_CMD_DISABLE_CLOUD] = &CloudSyncServiceStub::HandleDisableCloud;
    opToInterfaceMap_[SERVICE_CMD_DOWNLOAD_FILE] = &CloudSyncServiceStub::HandleDownloadFile;
}

int32_t CloudSyncServiceStub::OnRemoteRequest(uint32_t code,
                                              MessageParcel &data,
                                              MessageParcel &reply,
                                              MessageOption &option)
{
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        return E_SERVICE_DESCRIPTOR_IS_EMPTY;
    }
    auto interfaceIndex = opToInterfaceMap_.find(code);
    if (interfaceIndex == opToInterfaceMap_.end() || !interfaceIndex->second) {
        LOGE("Cannot response request %d: unknown tranction", code);
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    return (this->*(interfaceIndex->second))(data, reply);
}

int32_t CloudSyncServiceStub::HandleRegisterCallbackInner(MessageParcel &data, MessageParcel &reply)
{
    LOGI("Begin RegisterCallbackInner");
    if (!DfsuAccessTokenHelper::CheckCallerPermission(PERM_CLOUD_SYNC)) {
        LOGE("permission denied");
        return E_PERMISSION_DENIED;
    }
    if (!DfsuAccessTokenHelper::IsSystemApp()) {
        LOGE("caller hap is not system hap");
        return E_PERMISSION_SYSTEM;
    }
    auto remoteObj = data.ReadRemoteObject();
    int32_t res = RegisterCallbackInner(remoteObj);
    reply.WriteInt32(res);
    LOGI("End RegisterCallbackInner");
    return res;
}

int32_t CloudSyncServiceStub::HandleStartSyncInner(MessageParcel &data, MessageParcel &reply)
{
    LOGI("Begin StartSyncInner");
    if (!DfsuAccessTokenHelper::CheckCallerPermission(PERM_CLOUD_SYNC)) {
        LOGE("permission denied");
        return E_PERMISSION_DENIED;
    }
    if (!DfsuAccessTokenHelper::IsSystemApp()) {
        LOGE("caller hap is not system hap");
        return E_PERMISSION_SYSTEM;
    }
    auto forceFlag = data.ReadBool();
    int32_t res = StartSyncInner(forceFlag);
    reply.WriteInt32(res);
    LOGI("End StartSyncInner");
    return res;
}

int32_t CloudSyncServiceStub::HandleStopSyncInner(MessageParcel &data, MessageParcel &reply)
{
    LOGI("Begin StopSyncInner");
    if (!DfsuAccessTokenHelper::CheckCallerPermission(PERM_CLOUD_SYNC)) {
        LOGE("permission denied");
        return E_PERMISSION_DENIED;
    }
    if (!DfsuAccessTokenHelper::IsSystemApp()) {
        LOGE("caller hap is not system hap");
        return E_PERMISSION_SYSTEM;
    }
    int32_t res = StopSyncInner();
    reply.WriteInt32(res);
    LOGI("End StopSyncInner");
    return res;
}

int32_t CloudSyncServiceStub::HandleChangeAppSwitch(MessageParcel &data, MessageParcel &reply)
{
    LOGI("Begin ChangeAppSwitch");
    if (!DfsuAccessTokenHelper::CheckCallerPermission(PERM_CLOUD_SYNC_MANAGER)) {
        LOGE("permission denied");
        return E_PERMISSION_DENIED;
    }
    if (!DfsuAccessTokenHelper::IsSystemApp()) {
        LOGE("caller hap is not system hap");
        return E_PERMISSION_SYSTEM;
    }
    string accountId = data.ReadString();
    string bundleName = data.ReadString();
    bool status = data.ReadBool();
    int32_t res = ChangeAppSwitch(accountId, bundleName, status);
    reply.WriteInt32(res);
    LOGI("End ChangeAppSwitch");
    return res;
}

int32_t CloudSyncServiceStub::HandleClean(MessageParcel &data, MessageParcel &reply)
{
    LOGI("Begin Clean");
    if (!DfsuAccessTokenHelper::CheckCallerPermission(PERM_CLOUD_SYNC_MANAGER)) {
        LOGE("permission denied");
        return E_PERMISSION_DENIED;
    }
    if (!DfsuAccessTokenHelper::IsSystemApp()) {
        LOGE("caller hap is not system hap");
        return E_PERMISSION_SYSTEM;
    }
    string accountId = data.ReadString();
    sptr<CleanOptions> options = data.ReadParcelable<CleanOptions>();
    if (!options) {
        LOGE("object of CleanOptions is nullptr");
        return E_INVAL_ARG;
    }
    int32_t res = Clean(accountId, *options);
    reply.WriteInt32(res);
    LOGI("End Clean");
    return res;
}

int32_t CloudSyncServiceStub::HandleNotifyDataChange(MessageParcel &data, MessageParcel &reply)
{
    LOGI("Begin NotifyDataChange");
    if (!DfsuAccessTokenHelper::CheckCallerPermission(PERM_CLOUD_SYNC_MANAGER)) {
        LOGE("permission denied");
        return E_PERMISSION_DENIED;
    }
    if (!DfsuAccessTokenHelper::IsSystemApp()) {
        LOGE("caller hap is not system hap");
        return E_PERMISSION_SYSTEM;
    }
    string accountId = data.ReadString();
    string bundleName = data.ReadString();
    int32_t res = NotifyDataChange(accountId, bundleName);
    reply.WriteInt32(res);
    LOGI("End NotifyDataChange");
    return res;
}
int32_t CloudSyncServiceStub::HandleDisableCloud(MessageParcel &data, MessageParcel &reply)
{
    LOGI("Begin DisableCloud");
    if (!DfsuAccessTokenHelper::CheckCallerPermission(PERM_CLOUD_SYNC_MANAGER)) {
        LOGE("permission denied");
        return E_PERMISSION_DENIED;
    }
    if (!DfsuAccessTokenHelper::IsSystemApp()) {
        LOGE("caller hap is not system hap");
        return E_PERMISSION_SYSTEM;
    }
    string accountId = data.ReadString();
    int32_t res = DisableCloud(accountId);
    reply.WriteInt32(res);
    LOGI("End DisableCloud");
    return res;
}
int32_t CloudSyncServiceStub::HandleEnableCloud(MessageParcel &data, MessageParcel &reply)
{
    LOGI("Begin EnableCloud");
    if (!DfsuAccessTokenHelper::CheckCallerPermission(PERM_CLOUD_SYNC_MANAGER)) {
        LOGE("permission denied");
        return E_PERMISSION_DENIED;
    }
    if (!DfsuAccessTokenHelper::IsSystemApp()) {
        LOGE("caller hap is not system hap");
        return E_PERMISSION_SYSTEM;
    }
    string accountId = data.ReadString();
    sptr<SwitchDataObj> switchObj = data.ReadParcelable<SwitchDataObj>();
    int32_t res = EnableCloud(accountId, *switchObj);
    reply.WriteInt32(res);
    LOGI("End EnableCloud");
    return res;
}

int32_t CloudSyncServiceStub::HandleDownloadFile(MessageParcel &data, MessageParcel &reply)
{
    LOGI("Begin DownloadFile");
    if (!DfsuAccessTokenHelper::CheckCallerPermission(PERM_CLOUD_SYNC_MANAGER)) {
        LOGE("permission denied");
        return E_PERMISSION_DENIED;
    }
    if (!DfsuAccessTokenHelper::IsSystemApp()) {
        LOGE("caller hap is not system hap");
        return E_PERMISSION_SYSTEM;
    }
    string url = data.ReadString();
    auto processCallback = data.ReadRemoteObject();
    auto downloadedCallback = data.ReadRemoteObject();

    int32_t res = DownloadFile(url, processCallback, downloadedCallback);
    reply.WriteInt32(res);
    LOGI("End DownloadFile");
    return res;
}
} // namespace OHOS::FileManagement::CloudSync