/*
* Copyright (c) 2021 Huawei Device Co., Ltd.
d * you may not use this file except in compliance with the License.
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

#pragma once

#include <functional>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#include "nocopyable.h"

namespace OHOS {
namespace DistributedFile {
namespace Utils {
enum Uid {
    UID_ROOT = 0,
    UID_SYSTEM = 1000,
    UID_MEDIA_RW = 1023,
};

void ForceCreateDirectory(const std::string &path);
void ForceCreateDirectory(const std::string &path, mode_t mode);
void ForceCreateDirectory(const std::string &path, mode_t mode, uid_t uid, gid_t gid);

void ForceRemoveDirectory(const std::string &path);
} // namespace Utils
} // namespace DistributedFile
} // namespace OHOS