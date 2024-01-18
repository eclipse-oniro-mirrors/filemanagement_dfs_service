/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "file_operations_base.h"
#include "cloud_disk_inode.h"
#include "file_operations_helper.h"

#include <cerrno>

#include "utils_log.h"

namespace OHOS {
namespace FileManagement {
namespace CloudDisk {
using namespace std;
void FileOperationsBase::Lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    LOGE("Lookup operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

void FileOperationsBase::Access(fuse_req_t req, fuse_ino_t ino, int mask)
{
    LOGE("Access operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

void FileOperationsBase::GetAttr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    LOGE("GetAttr operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

void FileOperationsBase::Open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    LOGE("Open operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

void FileOperationsBase::Forget(fuse_req_t req, fuse_ino_t ino, uint64_t nLookup)
{
    LOGE("Forget operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

static int32_t ForgetCloudIno(struct CloudDiskInode *inoPtr, string &key)
{
    if (inoPtr == nullptr) {
        LOGE("Get an invalid inode!");
        return EINVAL;
    }
    if (inoPtr->layer == CLOUD_DISK_INODE_OTHER_LAYER) {
        auto parentPtr = reinterpret_cast<struct CloudDiskInode *>(inoPtr->parent);
        if (parentPtr == nullptr) {
            LOGE("ForgetMulti Function get an invalid parent inode!");
            return EINVAL;
        }
        key = parentPtr->cloudId + inoPtr->fileName;
    }
    return 0;
}

void FileOperationsBase::ForgetMulti(fuse_req_t req, size_t count, struct fuse_forget_data *forgets)
{
    auto data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    for (size_t i = 0; i < count; i++) {
        string key = FileOperationsHelper::GetCloudDiskRootPath(data->userId);
        if (forgets[i].ino != FUSE_ROOT_ID) {
            auto inoPtr = reinterpret_cast<struct CloudDiskInode *>(forgets[i].ino);
            if (inoPtr == nullptr) {
                LOGE("ForgetMulti Function get an invalid inode!");
                continue;
            }
            key = inoPtr->path;
            if (ForgetCloudIno(inoPtr, key) != 0) {
                LOGE("ForgetMulti Function failed to process a cloud ino!");
                continue;
            }
        }
        shared_ptr<CloudDiskInode> node = FileOperationsHelper::FindCloudDiskInode(data, key);
        FileOperationsHelper::PutCloudDiskInode(data, node, forgets[i].nlookup, key);
    }
    fuse_reply_none(req);
}

void FileOperationsBase::MkNod(fuse_req_t req, fuse_ino_t parent, const char *name,
                               mode_t mode, dev_t rdev)
{
    LOGE("MkNod operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

void FileOperationsBase::Create(fuse_req_t req, fuse_ino_t parent, const char *name,
                                mode_t mode, struct fuse_file_info *fi)
{
    LOGE("Create operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

void FileOperationsBase::ReadDir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                                 struct fuse_file_info *fi)
{
    LOGE("ReadDir operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

void FileOperationsBase::SetXattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                                  const char *value, size_t size, int flags)
{
    LOGE("SetXattr operation is not supported!");
    fuse_reply_err(req, EINVAL);
}

void FileOperationsBase::GetXattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                                  size_t size)
{
    LOGE("GetXattr operation is not supported!");
    fuse_reply_err(req, EINVAL);
}

void FileOperationsBase::MkDir(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode)
{
    LOGE("MkDir operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

void FileOperationsBase::RmDir(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    LOGE("RmDir operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

void FileOperationsBase::Unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    LOGE("Unlink operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

void FileOperationsBase::Rename(fuse_req_t req, fuse_ino_t parent, const char *name,
                                fuse_ino_t newParent, const char *newName, unsigned int flags)
{
    LOGE("Rename operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

void FileOperationsBase::Read(fuse_req_t req, fuse_ino_t ino, size_t size,
                              off_t offset, struct fuse_file_info *fi)
{
    LOGE("Read operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

void FileOperationsBase::WriteBuf(fuse_req_t req, fuse_ino_t ino, struct fuse_bufvec *bufv,
                                  off_t off, struct fuse_file_info *fi)
{
    LOGE("WriteBuf operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

void FileOperationsBase::Release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    LOGE("Release operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}
void FileOperationsBase::SetAttr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, int valid,
                                 struct fuse_file_info *fi)
{
    LOGE("SetAttr operation is not supported!");
    fuse_reply_err(req, EINVAL);
}
void FileOperationsBase::Lseek(fuse_req_t req, fuse_ino_t ino, off_t off, int whence,
                               struct fuse_file_info *fi)
{
    LOGE("Lseek operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}
} // namespace CloudDisk
} // namespace FileManagement
} // namespace OHOS
