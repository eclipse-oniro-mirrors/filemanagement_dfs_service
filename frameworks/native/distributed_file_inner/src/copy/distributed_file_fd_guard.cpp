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

#include "copy/distributed_file_fd_guard.h"

#include <unistd.h>
#include "utils_log.h"

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0xD004315
#define LOG_TAG "distributedfile_daemon"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
FDGuard::FDGuard(int fd) : fd_(fd) {}

FDGuard::FDGuard(int fd, bool autoClose) : fd_(fd), autoClose_(autoClose) {}

FDGuard::FDGuard(FDGuard &&fdg) : fd_(fdg.fd_), autoClose_(fdg.autoClose_)
{
    fdg.fd_ = -1;
}

FDGuard &FDGuard::operator=(FDGuard &&fdg)
{
    if (this == &fdg) {
        return *this;
    }
    this->fd_ = fdg.fd_;
    this->autoClose_ = fdg.autoClose_;
    fdg.fd_ = -1;
    return *this;
}

FDGuard::~FDGuard()
{
    if (fd_ >= 0 && fd_ <= STDERR_FILENO) {
        LOGI("~FDGuard, fd_ = %{public}d", fd_);
    }
    if (fd_ >= 0 && autoClose_) {
        close(fd_);
    }
}

FDGuard::operator bool() const
{
    return fd_ >= 0;
}

int FDGuard::GetFD() const
{
    return fd_;
}

void FDGuard::SetFD(int fd, bool autoClose)
{
    fd_ = fd;
    autoClose_ = autoClose;
}

void FDGuard::ClearFD()
{
    fd_ = -1;
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
