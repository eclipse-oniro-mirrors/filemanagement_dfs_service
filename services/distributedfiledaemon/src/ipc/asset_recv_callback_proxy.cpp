/*
* Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "asset_recv_callback_proxy.h"

#include "accesstoken_kit.h"
#include "asset/asset_callback_interface_code.h"
#include "dfs_error.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "utils_log.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
using namespace std;
using namespace OHOS::Storage;
using namespace OHOS::FileManagement;

int32_t AssetRecvCallbackProxy::OnStart(const std::string &srcNetworkId,
                                        const std::string &destNetworkId,
                                        const std::string &sessionId,
                                        const std::string &destBundleName)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOGE("Failed to write interface token");
        return E_BROKEN_IPC;
    }
    if (!data.WriteString(srcNetworkId)) {
        LOGE("Failed to send srcNetworkId");
        return E_INVAL_ARG;
    }
    if (!data.WriteString(destNetworkId)) {
        LOGE("Failed to send destNetworkId");
        return E_INVAL_ARG;
    }
    if (!data.WriteString(sessionId)) {
        LOGE("Failed to send sessionId");
        return E_INVAL_ARG;
    }
    if (!data.WriteString(destBundleName)) {
        LOGE("Failed to send destBundleName");
        return E_INVAL_ARG;
    }
    auto remote = Remote();
    if (remote == nullptr) {
        LOGE("remote is nullptr");
        return E_BROKEN_IPC;
    }
    int32_t ret = remote->SendRequest(
        static_cast<uint32_t>(AssetCallbackInterfaceCode::ASSET_CALLBACK_ON_START), data, reply, option);
    if (ret != E_OK) {
        LOGE("Failed to send out the request, errno:%{public}d", errno);
        return E_BROKEN_IPC;
    }
    return reply.ReadInt32();
}

int32_t AssetRecvCallbackProxy::OnRecvProgress(const std::string &srcNetworkId,
                                               const sptr<AssetObj> &assetObj,
                                               uint64_t totalBytes,
                                               uint64_t processBytes)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOGE("Failed to write interface token");
        return E_BROKEN_IPC;
    }
    if (!data.WriteString(srcNetworkId)) {
        LOGE("Failed to send srcNetworkId");
        return E_INVAL_ARG;
    }

    if (!data.WriteParcelable(assetObj)) {
        LOGE("Failed to send the assetInfoObj");
        return E_INVAL_ARG;
    }

    if (!data.WriteUint64(totalBytes)) {
        LOGE("Failed to send totalBytes");
        return E_INVAL_ARG;
    }

    if (!data.WriteUint64(processBytes)) {
        LOGE("Failed to send processBytes");
        return E_INVAL_ARG;
    }
    auto remote = Remote();
    if (remote == nullptr) {
        LOGE("remote is nullptr");
        return E_BROKEN_IPC;
    }
    int32_t ret = remote->SendRequest(
        static_cast<uint32_t>(AssetCallbackInterfaceCode::ASSET_CALLBACK_ON_PROGRESS), data, reply, option);
    if (ret != E_OK) {
        LOGE("Failed to send out the request, errno:%{public}d", errno);
        return E_BROKEN_IPC;
    }
    return reply.ReadInt32();
}

int32_t AssetRecvCallbackProxy::OnFinished(const std::string &srcNetworkId,
                                           const sptr<AssetObj> &assetObj,
                                           int32_t result)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOGE("Failed to write interface token");
        return E_BROKEN_IPC;
    }
    if (!data.WriteString(srcNetworkId)) {
        LOGE("Failed to send srcNetworkId");
        return E_INVAL_ARG;
    }

    if (!data.WriteParcelable(assetObj)) {
        LOGE("Failed to send the assetInfoObj");
        return E_INVAL_ARG;
    }

    if (!data.WriteInt32(result)) {
        LOGE("Failed to send result");
        return E_INVAL_ARG;
    }
    auto remote = Remote();
    if (remote == nullptr) {
        LOGE("remote is nullptr");
        return E_BROKEN_IPC;
    }
    int32_t ret = remote->SendRequest(
        static_cast<uint32_t>(AssetCallbackInterfaceCode::ASSET_CALLBACK_ON_FINISHED), data, reply, option);
    if (ret != E_OK) {
        LOGE("Failed to send out the request, errno:%{public}d", errno);
        return E_BROKEN_IPC;
    }
    return reply.ReadInt32();
}

} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS