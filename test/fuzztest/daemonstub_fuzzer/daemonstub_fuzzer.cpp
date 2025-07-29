/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "daemonstub_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "copy/ipc_wrapper.h"
#include "daemon_stub.h"
#include "distributed_file_daemon_ipc_interface_code.h"
#include "ipc_skeleton.h"
#include "message_option.h"
#include "message_parcel.h"
#include "securec.h"
#include "accesstoken_kit.h"
#include "utils_log.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

namespace OHOS {
constexpr pid_t DATA_UID = 3012;
constexpr pid_t DAEMON_UID = 1009;
const pid_t PASTEBOARDUSERID = 3816;
static pid_t UID = DAEMON_UID;
#ifdef CONFIG_IPC_SINGLE
using namespace IPC_SINGLE;
#endif
pid_t IPCSkeleton::GetCallingUid()
{
    return UID;
}
}

namespace OHOS {
using namespace OHOS::Storage::DistributedFile;

class DaemonStubImpl : public DaemonStub {
public:
    DaemonStubImpl() = default;
    ~DaemonStubImpl() override {}
    int32_t OpenP2PConnection(const DistributedHardware::DmDeviceInfo &deviceInfo) override
    {
        return 0;
    }

    int32_t CloseP2PConnection(const DistributedHardware::DmDeviceInfo &deviceInfo) override
    {
        return 0;
    }

    int32_t OpenP2PConnectionEx(const std::string &networkId, sptr<IFileDfsListener> remoteReverseObj) override
    {
        return 0;
    }

    int32_t CloseP2PConnectionEx(const std::string &networkId) override
    {
        return 0;
    }

    int32_t PrepareSession(const std::string &srcUri,
                           const std::string &dstUri,
                           const std::string &srcDeviceId,
                           const sptr<IRemoteObject> &listener,
                           HmdfsInfo &info) override
    {
        return 0;
    }

    int32_t CancelCopyTask(const std::string &sessionName) override
    {
        return 0;
    }

    int32_t CancelCopyTask(const std::string &srcUri, const std::string &dstUri) override
    {
        return 0;
    }

    int32_t RequestSendFile(const std::string &srcUri,
                            const std::string &dstPath,
                            const std::string &remoteDeviceId,
                            const std::string &sessionName) override
    {
        return 0;
    }

    int32_t GetRemoteCopyInfo(const std::string &srcUri, bool &isFile, bool &isDir) override
    {
        return 0;
    }

    int32_t PushAsset(int32_t userId,
                      const sptr<AssetObj> &assetObj,
                      const sptr<IAssetSendCallback> &sendCallback) override
    {
        return 0;
    }

    int32_t RegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback) override
    {
        return 0;
    }

    int32_t UnRegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback) override
    {
        return 0;
    }

    int32_t GetDfsUrisDirFromLocal(const std::vector<std::string> &uriList,
                                   const int32_t userId,
                                   std::unordered_map<std::string, AppFileService::ModuleRemoteFileShare::HmdfsUriInfo>
                                   &uriToDfsUriMaps) override
    {
        return 0;
    }

    int32_t GetDfsSwitchStatus(const std::string &networkId, int32_t &switchStatus) override
    {
        return 0;
    }

    int32_t UpdateDfsSwitchStatus(int32_t switchStatus) override
    {
        return 0;
    }

    int32_t GetConnectedDeviceList(std::vector<DfsDeviceInfo> &deviceList) override
    {
        return 0;
    }
};

void HandleOpenP2PConnectionFuzzTest(std::shared_ptr<DaemonStub> daemonStubPtr,
                                     const uint8_t *data,
                                     size_t size)
{
    uint32_t code = static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_OPEN_P2P_CONNECTION);
    MessageParcel datas;
    datas.WriteInterfaceToken(DaemonStub::GetDescriptor());
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    daemonStubPtr->OnRemoteRequest(code, datas, reply, option);
}

void HandleCloseP2PConnectionFuzzTest(std::shared_ptr<DaemonStub> daemonStubPtr,
                                      const uint8_t *data,
                                      size_t size)
{
    uint32_t code = static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_CLOSE_P2P_CONNECTION);
    MessageParcel datas;
    datas.WriteInterfaceToken(DaemonStub::GetDescriptor());
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    daemonStubPtr->OnRemoteRequest(code, datas, reply, option);
}

void HandleOpenP2PConnectionExFuzzTest(std::shared_ptr<DaemonStub> daemonStubPtr,
                                       const uint8_t *data,
                                       size_t size)
{
    uint32_t code = static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_OPEN_P2P_CONNECTION_EX);
    MessageParcel datas;
    datas.WriteInterfaceToken(DaemonStub::GetDescriptor());
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    daemonStubPtr->OnRemoteRequest(code, datas, reply, option);
}

void HandleCloseP2PConnectionExFuzzTest(std::shared_ptr<DaemonStub> daemonStubPtr,
                                        const uint8_t *data,
                                        size_t size)
{
    uint32_t code = static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_CLOSE_P2P_CONNECTION_EX);
    MessageParcel datas;
    datas.WriteInterfaceToken(DaemonStub::GetDescriptor());
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    daemonStubPtr->OnRemoteRequest(code, datas, reply, option);
}

void HandlePrepareSessionFuzzTest(std::shared_ptr<DaemonStub> daemonStubPtr, const uint8_t *data, size_t size)
{
    uint32_t code = static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_PREPARE_SESSION);
    MessageParcel datas;
    datas.WriteInterfaceToken(DaemonStub::GetDescriptor());
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    daemonStubPtr->OnRemoteRequest(code, datas, reply, option);
}

void HandleCancelCopyTaskFuzzTest(std::shared_ptr<DaemonStub> daemonStubPtr, const uint8_t *data, size_t size)
{
    uint32_t code = static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_CANCEL_COPY_TASK);
    MessageParcel datas;
    datas.WriteInterfaceToken(DaemonStub::GetDescriptor());
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    daemonStubPtr->OnRemoteRequest(code, datas, reply, option);
}

void HandleRequestSendFileFuzzTest(std::shared_ptr<DaemonStub> daemonStubPtr,
                                   const uint8_t *data,
                                   size_t size)
{
    OHOS::UID = DAEMON_UID;
    uint32_t code = static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_REQUEST_SEND_FILE);
    MessageParcel datas;
    datas.WriteInterfaceToken(DaemonStub::GetDescriptor());
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    daemonStubPtr->OnRemoteRequest(code, datas, reply, option);
}

void HandleGetRemoteCopyInfoFuzzTest(std::shared_ptr<DaemonStub> daemonStubPtr,
                                     const uint8_t *data,
                                     size_t size)
{
    OHOS::UID = DAEMON_UID;
    uint32_t code = static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_GET_REMOTE_COPY_INFO);
    MessageParcel datas;
    datas.WriteInterfaceToken(DaemonStub::GetDescriptor());
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    daemonStubPtr->OnRemoteRequest(code, datas, reply, option);
}

void HandlePushAssetFuzzTest(std::shared_ptr<DaemonStub> daemonStubPtr, const uint8_t *data, size_t size)
{
    OHOS::UID = DATA_UID;
    uint32_t code = static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_PUSH_ASSET);
    MessageParcel datas;
    datas.WriteInterfaceToken(DaemonStub::GetDescriptor());
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    daemonStubPtr->OnRemoteRequest(code, datas, reply, option);
}

void HandleRegisterRecvCallbackFuzzTest(std::shared_ptr<DaemonStub> daemonStubPtr,
                                        const uint8_t *data,
                                        size_t size)
{
    OHOS::UID = DATA_UID;
    uint32_t code = static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_REGISTER_ASSET_CALLBACK);
    MessageParcel datas;
    datas.WriteInterfaceToken(DaemonStub::GetDescriptor());
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    daemonStubPtr->OnRemoteRequest(code, datas, reply, option);
}

void HandleUnRegisterRecvCallbackFuzzTest(std::shared_ptr<DaemonStub> daemonStubPtr,
                                          const uint8_t *data,
                                          size_t size)
{
    OHOS::UID = DATA_UID;
    uint32_t code =
        static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_UN_REGISTER_ASSET_CALLBACK);
    MessageParcel datas;
    datas.WriteInterfaceToken(DaemonStub::GetDescriptor());
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    daemonStubPtr->OnRemoteRequest(code, datas, reply, option);
}

void HandleGetDfsUrisDirFromLocalFuzzTest(std::shared_ptr<DaemonStub> daemonStubPtr,
                                          const uint8_t *data,
                                          size_t size)
{
    OHOS::UID = PASTEBOARDUSERID;
    uint32_t code =
        static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::GET_DFS_URI_IS_DIR_FROM_LOCAL);
    MessageParcel datas;
    datas.WriteInterfaceToken(DaemonStub::GetDescriptor());
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    daemonStubPtr->OnRemoteRequest(code, datas, reply, option);
}

void HandleInnerCancelCopyTaskFuzzTest(std::shared_ptr<DaemonStub> daemonStubPtr,  const uint8_t *data, size_t size)
{
    OHOS::UID = DATA_UID;
    uint32_t code =
    static_cast<uint32_t>(DistributedFileDaemonInterfaceCode::DISTRIBUTED_FILE_CANCEL_INNER_COPY_TASK);
    MessageParcel datas;
    datas.WriteInterfaceToken(DaemonStub::GetDescriptor());
    int len = size >> 1;
    datas.WriteString(std::string(reinterpret_cast<const char *>(data), len));
    datas.WriteString(std::string(reinterpret_cast<const char *>(data + len), len));
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    daemonStubPtr->OnRemoteRequest(code, datas, reply, option);
}

void WriteUriByRawDataFuzzTest(const uint8_t *data, size_t size)
{
    MessageParcel datas;
    std::vector<std::string> uriVec { std::string(reinterpret_cast<const char *>(data), size) };

    IpcWrapper::WriteUriByRawData(datas, uriVec);
    IpcWrapper::WriteBatchUris(datas, uriVec);

    void* buf = nullptr;
    if (IpcWrapper::GetData(buf, size, data)) {
        free(buf);
        buf = nullptr;
    }
}

void ReadBatchUriByRawDataFuzzTest(const uint8_t *data, size_t size)
{
    MessageParcel datas;
    std::vector<std::string> uriVec;

    datas.WriteInt32(0);
    IpcWrapper::ReadBatchUriByRawData(datas, uriVec);

    datas.WriteInt32(1);
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);

    IpcWrapper::ReadBatchUriByRawData(datas, uriVec);
}

void ReadBatchUrisFuzzTest(const uint8_t *data, size_t size)
{
    MessageParcel datas;
    std::vector<std::string> uriVec;

    datas.WriteUint32(0);
    IpcWrapper::ReadBatchUris(datas, uriVec);

    datas.WriteUint32(1);
    datas.WriteBuffer(data, size);
    datas.RewindRead(0);

    IpcWrapper::ReadBatchUris(datas, uriVec);
}

void SetAccessTokenPermission()
{
    uint64_t tokenId;
    const char *perms[1];
    perms[0] = "ohos.permission.DISTRIBUTED_DATASYNC";

    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 1,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "distributdFileDaemonstubFuzzer",
        .aplStr = "system_basic",
    };
    tokenId = GetAccessTokenId(&infoInstance);
    if (tokenId == 0) {
        LOGE("Get Acess Token Id Failed");
        return;
    }
    int ret = SetSelfTokenID(tokenId);
    if (ret != 0) {
        LOGE("Set Acess Token Id Failed");
        return;
    }
    ret = Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    if (ret < 0) {
        LOGE("Reload Native Token Info Failed");
        return;
    }
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::SetAccessTokenPermission();
    auto daemonStubPtr = std::make_shared<OHOS::DaemonStubImpl>();
    OHOS::HandleOpenP2PConnectionFuzzTest(daemonStubPtr, data, size);
    OHOS::HandleCloseP2PConnectionFuzzTest(daemonStubPtr, data, size);
    OHOS::HandlePrepareSessionFuzzTest(daemonStubPtr, data, size);
    OHOS::HandleRequestSendFileFuzzTest(daemonStubPtr, data, size);
    OHOS::HandleOpenP2PConnectionExFuzzTest(daemonStubPtr, data, size);
    OHOS::HandleCloseP2PConnectionExFuzzTest(daemonStubPtr, data, size);
    OHOS::HandleCancelCopyTaskFuzzTest(daemonStubPtr, data, size);
    OHOS::HandleGetRemoteCopyInfoFuzzTest(daemonStubPtr, data, size);
    OHOS::HandlePushAssetFuzzTest(daemonStubPtr, data, size);
    OHOS::HandleRegisterRecvCallbackFuzzTest(daemonStubPtr, data, size);
    OHOS::HandleUnRegisterRecvCallbackFuzzTest(daemonStubPtr, data, size);
    OHOS::HandleGetDfsUrisDirFromLocalFuzzTest(daemonStubPtr, data, size);
    OHOS::HandleInnerCancelCopyTaskFuzzTest(daemonStubPtr, data, size);

    OHOS::WriteUriByRawDataFuzzTest(data, size);
    OHOS::ReadBatchUriByRawDataFuzzTest(data, size);
    OHOS::ReadBatchUrisFuzzTest(data, size);
    return 0;
}
