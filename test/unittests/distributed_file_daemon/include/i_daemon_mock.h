/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef MOCK_I_DAEMON_H
#define MOCK_I_DAEMON_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "dfs_error.h"
#include "ipc/i_daemon.h"
#include "iremote_broker.h"
#include "iremote_stub.h"

namespace OHOS::Storage::DistributedFile {
class DaemonServiceMock : public IRemoteStub<IDaemon> {
public:
    int code_;
    DaemonServiceMock() : code_(0) {}
    virtual ~DaemonServiceMock() {}

    MOCK_METHOD4(SendRequest, int(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option));

    int32_t InvokeSendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
    {
        code_ = code;
        reply.WriteInt32(FileManagement::E_OK);
        return FileManagement::E_OK;
    }

    int32_t OpenP2PConnection(const DistributedHardware::DmDeviceInfo &deviceInfo)
    {
        return FileManagement::E_OK;
    }

    int32_t CloseP2PConnection(const DistributedHardware::DmDeviceInfo &deviceInfo)
    {
        return FileManagement::E_OK;
    }

    int32_t OpenP2PConnectionEx(const std::string &networkId, sptr<IFileDfsListener> remoteReverseObj)
    {
        return FileManagement::E_OK;
    }

    int32_t CloseP2PConnectionEx(const std::string &networkId)
    {
        return FileManagement::E_OK;
    }

    int32_t PrepareSession(const std::string &srcUri,
                           const std::string &dstUri,
                           const std::string &srcDeviceId,
                           const sptr<IRemoteObject> &listener,
                           HmdfsInfo &fileInfo)
    {
        return FileManagement::E_OK;
    }

    int32_t RequestSendFile(const std::string &srcUri,
                            const std::string &dstPath,
                            const std::string &remoteDeviceId,
                            const std::string &sessionName)
    {
        return FileManagement::E_OK;
    }

    int32_t RequestSendFileACL(const std::string &srcUri, const std::string &dstPath,
                                   const std::string &remoteDeviceId, const std::string &sessionName,
                                   const AccountInfo &callerAccountInfo)
    {
        return FileManagement::E_OK;
    }

    int32_t GetRemoteCopyInfo(const std::string &srcUri, bool &isFile, bool &isDir)
    {
        return FileManagement::E_OK;
    }

    int32_t GetRemoteCopyInfoACL(const std::string &srcUri, bool &isFile, bool &isDir,
                                     const AccountInfo &callerAccountInfo)
    {
        return FileManagement::E_OK;
    }

    int32_t CancelCopyTask(const std::string &sessionName)
    {
        return FileManagement::E_OK;
    }

    int32_t CancelCopyTask(const std::string &srcUri, const std::string &dstUri)
    {
        return FileManagement::E_OK;
    }

    int32_t PushAsset(int32_t userId,
                      const sptr<AssetObj> &assetObj,
                      const sptr<IAssetSendCallback> &sendCallback)
    {
        return FileManagement::E_OK;
    }

    int32_t RegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback)
    {
        return FileManagement::E_OK;
    }

    int32_t UnRegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback)
    {
        return FileManagement::E_OK;
    }

    int32_t GetDfsUrisDirFromLocal(const std::vector<std::string> &uriList,
                                   const int32_t userId,
                                   std::unordered_map<std::string, AppFileService::ModuleRemoteFileShare::HmdfsUriInfo>
                                   &uriToDfsUriMaps)
    {
        return FileManagement::E_OK;
    }

    int32_t GetDfsSwitchStatus(const std::string &networkId, int32_t &switchStatus)
    {
        return FileManagement::E_OK;
    }

    int32_t UpdateDfsSwitchStatus(int32_t switchStatus)
    {
        return FileManagement::E_OK;
    }

    int32_t GetConnectedDeviceList(std::vector<DfsDeviceInfo> &deviceList)
    {
        return FileManagement::E_OK;
    }

    int32_t RegisterFileDfsListener(const std::string &instanceId, const sptr<IFileDfsListener> &listener)
    {
        return FileManagement::E_OK;
    }

    int32_t UnregisterFileDfsListener(const std::string &instanceId)
    {
        return FileManagement::E_OK;
    }

    int32_t IsSameAccountDevice(const std::string &networkId, bool &isSameAccount)
    {
        isSameAccount = true;
        return FileManagement::E_OK;
    }
};
} // namespace OHOS::Storage::DistributedFile

#endif // MOCK_I_DAEMON_H
