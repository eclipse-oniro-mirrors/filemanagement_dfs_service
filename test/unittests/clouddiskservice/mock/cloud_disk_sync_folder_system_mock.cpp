/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "cloud_disk_sync_folder_system_mock.h"

#include <cerrno>
#include <dlfcn.h>

using namespace OHOS::FileManagement::CloudDiskService;

extern "C" {
char *realpath(const char *path, char *resolvedPath)
{
    return SyncFolderAssistant::ins->realpath(path, resolvedPath);
}

ssize_t getxattr(const char *path, const char *name, void *value, size_t size)
{
    if (SyncFolderAssistant::ins != nullptr) {
        return SyncFolderAssistant::ins->getxattr(path, name, value, size);
    }
    return -1;
}

// libc symbol interposition for tests; the name must match <sys/xattr.h>.
// G.NAM.03-CPP: keep the C symbol name.
int lremovexattr(const char *path, const char *name)
{
    if (SyncFolderAssistant::ins != nullptr) {
        return SyncFolderAssistant::ins->lremovexattr(path, name);
    }
    static auto realLremovexattr =
        reinterpret_cast<int (*)(const char *, const char *)>(dlsym(RTLD_NEXT, "lremovexattr"));
    if (realLremovexattr == nullptr) {
        errno = ENOSYS;
        return -1;
    }
    return realLremovexattr(path, name);
}

int unlink(const char *pathname)
{
    if (SyncFolderAssistant::ins != nullptr) {
        return SyncFolderAssistant::ins->unlink(pathname);
    }
    return -1;
}

int lstat(const char *path, struct stat *buf)
{
    if (SyncFolderAssistant::ins != nullptr) {
        return SyncFolderAssistant::ins->lstat(path, buf);
    }
    auto func = (int (*)(const char *, struct stat *))dlsym(RTLD_NEXT, "lstat");
    if (func != nullptr) {
        return func(path, buf);
    }
    return -1;
}
}