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

#ifndef FILEMANAGEMENT_DFS_SERVICE_SDK_ASSET_CALLBACK_INTERFACE_CODE_H
#define FILEMANAGEMENT_DFS_SERVICE_SDK_ASSET_CALLBACK_INTERFACE_CODE_H

namespace OHOS {
namespace Storage {
namespace DistributedFile {
enum class AssetCallbackInterfaceCode {
    ASSET_CALLBACK_ON_START = 0,
    ASSET_CALLBACK_ON_FINISHED,
    ASSET_CALLBACK_ON_SEND_RESULT,
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS

#endif // FILEMANAGEMENT_DFS_SERVICE_SDK_ASSET_CALLBACK_INTERFACE_CODE_H
