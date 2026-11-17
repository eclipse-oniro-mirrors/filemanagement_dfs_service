/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "assistant.h"
#include "cloud_disk_sync_folder_system_mock.h"

#include <cerrno>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>

using namespace OHOS::FileManagement::CloudDiskService;

extern "C" {
int removexattr(const char *path, const char *name)
{
    if (Assistant::ins != nullptr) {
        errno = Assistant::mockErrno;
        return Assistant::ins->removexattr(path, name);
    }
    return 0;
}

ssize_t fgetxattr(int fd, const char *name, void *value, size_t size)
{
    if (Assistant::ins != nullptr) {
        return Assistant::ins->fgetxattr(fd, name, value, size);
    }
    static auto realFgetxattr = reinterpret_cast<ssize_t (*)(int, const char *, void *, size_t)>(
        dlsym(RTLD_NEXT, "fgetxattr"));
    if (realFgetxattr == nullptr) {
        errno = EIO;
        return -1;
    }
    return realFgetxattr(fd, name, value, size);
}

int fremovexattr(int fd, const char *name)
{
    if (Assistant::ins != nullptr) {
        return Assistant::ins->fremovexattr(fd, name);
    }
    static auto realFremovexattr =
        reinterpret_cast<int (*)(int, const char *)>(dlsym(RTLD_NEXT, "fremovexattr"));
    if (realFremovexattr == nullptr) {
        errno = EIO;
        return -1;
    }
    return realFremovexattr(fd, name);
}

int fstat(int fd, struct stat *buf)
{
    if (Assistant::ins != nullptr) {
        return Assistant::ins->fstat(fd, buf);
    }
    static auto realFstat = reinterpret_cast<int (*)(int, struct stat *)>(dlsym(RTLD_NEXT, "fstat"));
    if (realFstat == nullptr) {
        errno = EIO;
        return -1;
    }
    return realFstat(fd, buf);
}

struct dirent *readdir(DIR *dir)
{
    if (Assistant::ins != nullptr) {
        return Assistant::ins->readdir(dir);
    }
    static auto realReaddir = reinterpret_cast<struct dirent *(*)(DIR *)>(dlsym(RTLD_NEXT, "readdir"));
    if (realReaddir == nullptr) {
        errno = EIO;
        return nullptr;
    }
    return realReaddir(dir);
}

int closedir(DIR *dir)
{
    if (Assistant::ins != nullptr) {
        return Assistant::ins->CloseDir(dir);
    }
    static auto realClosedir = reinterpret_cast<int (*)(DIR *)>(dlsym(RTLD_NEXT, "closedir"));
    if (realClosedir == nullptr) {
        errno = EIO;
        return -1;
    }
    return realClosedir(dir);
}

int unlinkat(int dirfd, const char *path, int flags)
{
    if (SyncFolderAssistant::ins != nullptr) {
        return SyncFolderAssistant::ins->unlinkat(dirfd, path, flags);
    }
    static auto realUnlinkat = reinterpret_cast<int (*)(int, const char *, int)>(dlsym(RTLD_NEXT, "unlinkat"));
    if (realUnlinkat == nullptr) {
        errno = EIO;
        return -1;
    }
    return realUnlinkat(dirfd, path, flags);
}
}
