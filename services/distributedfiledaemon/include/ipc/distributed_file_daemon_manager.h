/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTED_FILE_DAEMON_MANAGER_H
#define DISTRIBUTED_FILE_DAEMON_MANAGER_H

#include <memory>

#include "dm_device_info.h"
#include "hmdfs_info.h"
#include "i_file_trans_listener.h"
#include "i_file_dfs_listener.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
class DistributedFileDaemonManager {
public:
    static DistributedFileDaemonManager &GetInstance();

    virtual int32_t OpenP2PConnection(const DistributedHardware::DmDeviceInfo &deviceInfo) = 0;
    virtual int32_t CloseP2PConnection(const DistributedHardware::DmDeviceInfo &deviceInfo) = 0;
    virtual int32_t OpenP2PConnectionEx(const std::string &networkId, sptr<IFileDfsListener> remoteReverseObj) = 0;
    virtual int32_t CloseP2PConnectionEx(const std::string &networkId) = 0;
    virtual int32_t PrepareSession(const std::string &srcUri,
                                   const std::string &dstUri,
                                   const std::string &srcDeviceId,
                                   const sptr<IRemoteObject> &listener,
                                   HmdfsInfo &info) = 0;
    virtual int32_t CancelCopyTask(const std::string &sessionName) = 0;
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS

#endif // DISTRIBUTED_FILE_DAEMON_MANAGER_H