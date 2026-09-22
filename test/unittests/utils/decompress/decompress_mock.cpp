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

#include "decompress_mock.h"

#ifdef DECOMPRESS_UNIT_TEST
#error "decompress_mock.cpp must not be compiled with DECOMPRESS_UNIT_TEST defined"
#endif

#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <securec.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/vfs.h>
#include <unistd.h>

extern "C" int __real_statfs(const char *path, struct statfs *buf);

namespace {
constexpr int IOCTL_MAX_RETRY_COUNT = 2;
}

namespace DecompressMock {

State g_mockState;

void Reset()
{
    g_mockState = State();
}

} // namespace DecompressMock

int MockOpen(const char *pathname, int flags, ...)
{
    auto &s = DecompressMock::g_mockState;
    if (s.openEnabled) {
        s.openCallCount++;
        if (s.openFailAfterCount >= 0 && s.openCallCount > s.openFailAfterCount) {
            errno = s.openErrno;
            return -1;
        }
        if (s.openReturnFd >= 0) {
            return s.openReturnFd;
        }
        errno = s.openErrno;
        return -1;
    }
    mode_t mode = 0;
    if (flags & (O_CREAT | O_TMPFILE)) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
    }
    return open(pathname, flags, mode);
}

int MockClose(int fd)
{
    auto &s = DecompressMock::g_mockState;
    if (s.closeEnabled) {
        return s.closeReturnVal;
    }
    return close(fd);
}

int MockIoctl(int fd, unsigned long request, ...)
{
    auto &s = DecompressMock::g_mockState;
    if (s.ioctlEnabled) {
        s.ioctlCallCount++;
        if (s.ioctlSuccessAfterCount >= 0 && s.ioctlCallCount > s.ioctlSuccessAfterCount) {
            return 0;
        }
        if (s.ioctlRetryErrno != 0 && s.ioctlCallCount <= IOCTL_MAX_RETRY_COUNT) {
            errno = s.ioctlRetryErrno;
        } else {
            errno = s.ioctlErrno;
        }
        return s.ioctlReturnVal;
    }
    va_list ap;
    va_start(ap, request);
    void *arg = va_arg(ap, void *);
    va_end(ap);
    return ioctl(fd, request, arg);
}

char *MockRealpath(const char *path, char *resolved)
{
    auto &s = DecompressMock::g_mockState;
    if (s.realpathEnabled) {
        if (s.realpathFail) {
            errno = ENOENT;
            return nullptr;
        }
        if (resolved) {
            errno_t ret = strncpy_s(resolved, PATH_MAX, path, PATH_MAX - 1);
            if (ret != EOK) {
                return nullptr;
            }
        }
        return resolved;
    }

    return realpath(path, resolved);
}

void *MockDlopen(const char *filename, int flags)
{
    auto &s = DecompressMock::g_mockState;
    if (s.dlopenEnabled) {
        return s.dlopenSucceed ? s.dlopenHandle : nullptr;
    }
    return dlopen(filename, flags);
}

void *MockDlsym(void *handle, const char *symbol)
{
    auto &s = DecompressMock::g_mockState;
    if (s.dlsymEnabled) {
        if (!s.dlsymSucceed) {
            return nullptr;
        }
        if (strcmp(symbol, "CloudSync_GetDecompressUnsupportedList") == 0) {
            static int32_t (*fn)(std::vector<std::string> *) = +[](std::vector<std::string> *list) -> int32_t {
                if (list) {
                    *list = DecompressMock::g_mockState.unsupportedList;
                }
                return DecompressMock::g_mockState.unsupportedListRet;
            };
            return reinterpret_cast<void *>(fn);
        }
        if (strcmp(symbol, "CloudSync_GetDecompressSystemFeature") == 0) {
            static int32_t (*fn)(bool *) = +[](bool *feature) -> int32_t {
                if (feature) {
                    *feature = DecompressMock::g_mockState.systemFeatureValue;
                }
                return DecompressMock::g_mockState.systemFeatureRet;
            };
            return reinterpret_cast<void *>(fn);
        }
        if (strcmp(symbol, "CloudSync_GetDecompressCrossFsConfig") == 0) {
            static int32_t (*fn)(bool *) = +[](bool *config) -> int32_t {
                if (config) {
                    *config = DecompressMock::g_mockState.crossFsConfigValue;
                }
                return DecompressMock::g_mockState.crossFsConfigRet;
            };
            return reinterpret_cast<void *>(fn);
        }
        return nullptr;
    }
    return dlsym(handle, symbol);
}

int MockStat(const char *pathname, struct stat *statbuf)
{
    auto &s = DecompressMock::g_mockState;
    if (s.statEnabled) {
        s.statCallCount++;
        if (s.statFail) {
            errno = ENOENT;
            return -1;
        }
        if (statbuf) {
            errno_t ret = memset_s(statbuf, sizeof(struct stat), 0, sizeof(struct stat));
            if (ret != EOK) {
                return -1;
            }
            statbuf->st_dev = (s.statCallCount == 1) ? s.statHapDev : s.statDstDev;
        }
        return 0;
    }
    return stat(pathname, statbuf);
}

FILE *MockFopen(const char *pathname, const char *mode)
{
    auto &s = DecompressMock::g_mockState;
    if (s.fopenEnabled) {
        return s.fopenSucceed ? s.fopenHandle : nullptr;
    }
    return fopen(pathname, mode);
}

int MockFclose(FILE *stream)
{
    auto &s = DecompressMock::g_mockState;
    if (s.fopenEnabled) {
        if (s.fcloseFail) {
            errno = EIO;
            return EOF;
        }
        return 0;
    }
    return fclose(stream);
}

ssize_t MockGetline(char **lineptr, size_t *n, FILE *stream)
{
    auto &s = DecompressMock::g_mockState;
    if (s.getlineEnabled) {
        if (s.getlineSucceed) {
            if (lineptr) {
                *lineptr = strdup(s.getlineContent);
            }
            if (n) {
                *n = strlen(s.getlineContent) + 1;
            }
            return s.getlineRetVal;
        }
        return -1;
    }
    return getline(lineptr, n, stream);
}

char *MockFgets(char *buf, int size, FILE *stream)
{
    auto &s = DecompressMock::g_mockState;
    if (s.fgetsEnabled) {
        if (s.fgetsSucceed && buf && size > 0) {
            strncpy_s(buf, static_cast<size_t>(size), s.fgetsContent, strlen(s.fgetsContent) + 1);
            return buf;
        }
        return nullptr;
    }
    return fgets(buf, size, stream);
}

int MockStatfs(const char *path, struct statfs *buf)
{
    auto &s = DecompressMock::g_mockState;
    if (s.statfsEnabled) {
        if (s.statfsFail) {
            errno = ENOENT;
            return -1;
        }
        if (buf) {
            errno_t ret = memset_s(buf, sizeof(struct statfs), 0, sizeof(struct statfs));
            if (ret != EOK) {
                return -1;
            }
            buf->f_type = static_cast<unsigned long>(s.statfsType);
            buf->f_frsize = static_cast<unsigned long>(s.statfsFrsize);
            buf->f_bsize = static_cast<unsigned long>(s.statfsBsize);
            buf->f_blocks = static_cast<uint64_t>(s.statfsBlocks);
            buf->f_bavail = static_cast<uint64_t>(s.statfsBavail);
        }
        return 0;
    }
    return __real_statfs(path, buf);
}

extern "C" int __wrap_statfs(const char *path, struct statfs *buf)
{
    return MockStatfs(path, buf);
}

int MockAccess(const char *path, int mode)
{
    auto &s = DecompressMock::g_mockState;
    if (s.accessEnabled) {
        if (s.accessFail) {
            errno = ENOENT;
            return -1;
        }
        return 0;
    }
    return access(path, mode);
}

int MockMkdir(const char *path, mode_t mode)
{
    auto &s = DecompressMock::g_mockState;
    if (s.mkdirEnabled) {
        if (s.mkdirFail) {
            errno = EACCES;
            return -1;
        }
        return 0;
    }
    return mkdir(path, mode);
}

int MockChown(const char *path, uid_t owner, gid_t group)
{
    auto &s = DecompressMock::g_mockState;
    if (s.chownEnabled) {
        if (s.chownFail) {
            errno = EPERM;
            return -1;
        }
        return 0;
    }
    return chown(path, owner, group);
}

int MockFchown(int fd, uid_t owner, gid_t group)
{
    auto &s = DecompressMock::g_mockState;
    if (s.fchownEnabled) {
        if (s.fchownFail) {
            errno = EPERM;
            return -1;
        }
        return 0;
    }
    return fchown(fd, owner, group);
}

int MockFlock(int fd, int operation)
{
    auto &s = DecompressMock::g_mockState;
    if (s.flockEnabled) {
        if (s.flockFail) {
            errno = EWOULDBLOCK;
            return -1;
        }
        return 0;
    }
    return flock(fd, operation);
}

off_t MockLseek(int fd, off_t offset, int whence)
{
    auto &s = DecompressMock::g_mockState;
    if (s.lseekEnabled) {
        if (s.lseekFail) {
            errno = EBADF;
            return -1;
        }
        return 0;
    }
    return lseek(fd, offset, whence);
}

int MockFtruncate(int fd, off_t length)
{
    auto &s = DecompressMock::g_mockState;
    if (s.ftruncateEnabled) {
        if (s.ftruncateFail) {
            errno = EBADF;
            return -1;
        }
        return 0;
    }
    return ftruncate(fd, length);
}

ssize_t MockWrite(int fd, const void *buf, size_t count)
{
    auto &s = DecompressMock::g_mockState;
    if (s.writeEnabled) {
        s.writeCallCount++;
        if (s.writeIntrBeforeCount >= 0 && s.writeCallCount <= s.writeIntrBeforeCount) {
            errno = EINTR;
            return -1;
        }
        if (s.writeFailAfterCount >= 0 && s.writeCallCount > s.writeFailAfterCount) {
            errno = EIO;
            return -1;
        }
        return write(fd, buf, count);
    }
    return write(fd, buf, count);
}
