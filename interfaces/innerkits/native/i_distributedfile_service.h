/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef I_DISTRIBUTEDFILE_SERVICE_H
#define I_DISTRIBUTEDFILE_SERVICE_H

#include "iremote_broker.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
class IDistributedFileService : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedFile.IDistributedFileService");
    // define the message code
    enum DistributedFileSurfaceCode {
        INTERFACE1 = 0,
        GET_BUNDLE_DISTRIBUTED_DIR,
        REMOVE_BUNDLE_DISTRIBUTED_DIRS,
    };
    // define the error code
    enum {
        DISTRIBUTEDFILE_SUCCESS = 0,
        GET_DISTRIBUTEDFILE_DISTRIBUTED_DIR_FAIL = 1,
        DISTRIBUTEDFILE_WRITE_DESCRIPTOR_TOKEN_FAIL,
        ERR_FLATTEN_OBJECT,
        DISTRIBUTEDFILE_NO_ERROR,
        DISTRIBUTEDFILE_CONNECT_SYSTEM_ABILITY_STUB_FAIL,
        REMOVE_DISTRIBUTEDFILE_DISTRIBUTEDDIRS_FAIL,
        DISTRIBUTEDFILE_BAD_TYPE,
        DISTRIBUTEDFILE_FAIL,
        DISTRIBUTEDFILE_REMOTE_ADDRESS_IS_NULL,
        DISTRIBUTEDFILE_WRITE_REPLY_FAIL,
        DISTRIBUTEDFILE_DIR_NAME_IS_EMPTY,
        DISTRIBUTEDFILE_NAME_NOT_FOUND,
        DISTRIBUTEDFILE_PERMISSION_DENIED,
        ROOT_UID,
        SYSTEM_SERVICE_UID,
    };
    // define business functions
    virtual int32_t GetBundleDistributedDir(const std::string &dirName) = 0;
    virtual int32_t RemoveBundleDistributedDirs(const std::string &dirName) = 0;
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS

#endif // I_YANGHU_TESt_SERVICE_H