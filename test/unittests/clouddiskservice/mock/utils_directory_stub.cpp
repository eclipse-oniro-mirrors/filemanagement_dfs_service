/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <cerrno>
#include <functional>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils_directory.h"
#include "utils_log.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
namespace Utils {
namespace {
constexpr mode_t STAT_MODE_DIR = 0771;

// Test double of utils/system/src/utils_directory.cpp ForceCreateDirectory: the same
// access/mkdir prefix walk, without the HiSysEvent reporting the real translation
// unit pulls in. It keeps targets compiling the real placeholder task manager free
// of the libhisysevent dependency while the mock seam (access) drives the branches.
void ForceCreateDirectoryWithCallback(const std::string &path,
                                      const std::function<void(const std::string &)> &onSubDirCreated)
{
    std::string::size_type index = 0;
    do {
        std::string subPath;
        index = path.find('/', index + 1);
        if (index == std::string::npos) {
            subPath = path;
        } else {
            subPath = path.substr(0, index);
        }

        if (access(subPath.c_str(), F_OK) != 0) {
            if (mkdir(subPath.c_str(), STAT_MODE_DIR) != 0) {
                LOGE("failed to mkdir, errno:%{public}d", errno);
                return;
            }
            if (onSubDirCreated != nullptr) {
                onSubDirCreated(subPath);
            }
        }
    } while (index != std::string::npos);
}
} // namespace

void ForceCreateDirectory(const std::string &path)
{
    ForceCreateDirectoryWithCallback(path, nullptr);
}

void ForceCreateDirectory(const std::string &path, mode_t mode)
{
    ForceCreateDirectoryWithCallback(path, [mode](const std::string &subPath) {
        if (chmod(subPath.c_str(), mode) == -1) {
            LOGE("failed to chmod, errno:%{public}d", errno);
        }
    });
}

void ForceCreateDirectory(const std::string &path, mode_t mode, uid_t uid, gid_t gid)
{
    ForceCreateDirectoryWithCallback(path, [mode, uid, gid](const std::string &subPath) {
        if (chmod(subPath.c_str(), mode) == -1 || chown(subPath.c_str(), uid, gid) == -1) {
            LOGE("failed to chmod or chown, errno:%{public}d", errno);
        }
    });
}
} // namespace Utils
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
