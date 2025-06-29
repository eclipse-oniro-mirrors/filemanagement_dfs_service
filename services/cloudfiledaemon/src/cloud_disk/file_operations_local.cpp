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
#include "file_operations_local.h"

#include <cerrno>
#include <dirent.h>

#include "cloud_file_fault_event.h"
#include "file_operations_cloud.h"
#include "file_operations_helper.h"
#include "fuse_ioctl.h"
#include "utils_log.h"
#include "xcollie_helper.h"

namespace OHOS {
namespace FileManagement {
namespace CloudDisk {
using namespace std;
using namespace CloudFile;
static const int32_t BUNDLE_NAME_OFFSET = 1000000000;
static const int32_t STAT_MODE_DIR = 0771;
static const float LOOKUP_TIMEOUT = 60.0;
#ifdef HICOLLIE_ENABLE
static const unsigned int LOOKUP_TIMEOUT_S = 1;
static const unsigned int GETATTR_TIMEOUT_S = 1;
#endif

static int32_t DoLocalLookup(fuse_req_t req, fuse_ino_t parent, const char *name,
                             struct fuse_entry_param *e)
{
    bool createFlag = false;
    struct CloudDiskFuseData *data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    string path = FileOperationsHelper::GetCloudDiskLocalPath(data->userId, name);
    std::unique_lock<std::shared_mutex> cWLock(data->cacheLock, std::defer_lock);
    string key = std::to_string(parent) + name;
    int64_t localId = FileOperationsHelper::FindLocalId(data, key);
    auto child = FileOperationsHelper::FindCloudDiskInode(data, localId);
    if (child == nullptr) {
        child = make_shared<CloudDiskInode>();
        createFlag = true;
        LOGD("new child %{public}s", name);
    }
    std::unique_lock<std::shared_mutex> lWLock(data->localIdLock, std::defer_lock);
    child->refCount++;
    if (createFlag) {
        if (stat(path.c_str(), &child->stat) != 0) {
            CLOUD_FILE_FAULT_REPORT(CloudFile::CloudFileFaultInfo{child->bundleName, CloudFile::FaultOperation::LOOKUP,
                CloudFile::FaultType::INODE_FILE, errno, "lookup " + GetAnonyString(path) + " error, err: " +
                std::to_string(errno)});
            return errno;
        }
        child->stat.st_mode |= STAT_MODE_DIR;
        child->parent = parent;
        child->path = path;
        auto parentInode = FileOperationsHelper::FindCloudDiskInode(data, static_cast<int64_t>(parent));
        child->layer = FileOperationsHelper::GetNextLayer(parentInode, parent);
        localId = FileOperationsHelper::GetFixedLayerRootId(child->layer);
        if (child->layer >= CLOUD_DISK_INODE_FIRST_LAYER) {
            std::lock_guard<std::shared_mutex> bWLock(data->bundleNameIdLock);
            data->bundleNameId++;
            localId = data->bundleNameId + BUNDLE_NAME_OFFSET;
        }
        child->stat.st_ino = static_cast<uint64_t>(localId);
        child->ops = make_shared<FileOperationsLocal>();
        cWLock.lock();
        data->inodeCache[localId] = child;
        cWLock.unlock();
        lWLock.lock();
        data->localIdCache[key] = localId;
        lWLock.unlock();
    }
    if (child->layer >= CLOUD_DISK_INODE_FIRST_LAYER) {
        child->bundleName = name;
        child->ops = make_shared<FileOperationsCloud>();
    }
    e->ino = static_cast<fuse_ino_t>(localId);
    FileOperationsHelper::GetInodeAttr(child, &e->attr);
    return 0;
}

void FileOperationsLocal::Lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    struct fuse_entry_param e;
    int32_t err;
    e.attr_timeout = LOOKUP_TIMEOUT;
    e.entry_timeout = LOOKUP_TIMEOUT;
#ifdef HICOLLIE_ENABLE
    auto xcollieId = XCollieHelper::SetTimer("CloudDisk_Lookup", LOOKUP_TIMEOUT_S, nullptr, nullptr, false);
#endif
    err = DoLocalLookup(req, parent, name, &e);
    if (err) {
        fuse_reply_err(req, err);
    } else {
        fuse_reply_entry(req, &e);
    }
#ifdef HICOLLIE_ENABLE
    XCollieHelper::CancelTimer(xcollieId);
#endif
}

void FileOperationsLocal::GetAttr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
#ifdef HICOLLIE_ENABLE
    auto xcollieId = XCollieHelper::SetTimer("CloudDisk_GetAttr", GETATTR_TIMEOUT_S, nullptr, nullptr, false);
#endif
    struct CloudDiskFuseData *data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    if (ino == FUSE_ROOT_ID) {
        string path = FileOperationsHelper::GetCloudDiskRootPath(data->userId);

        struct stat statBuf;
        int err = stat(path.c_str(), &statBuf);
        if (err != 0) {
            CLOUD_FILE_FAULT_REPORT(CloudFile::CloudFileFaultInfo{"", CloudFile::FaultOperation::GETATTR,
                CloudFile::FaultType::FILE, errno, "lookup " + GetAnonyString(path) + " error, err: " +
                std::to_string(errno)});
            fuse_reply_err(req, errno);
#ifdef HICOLLIE_ENABLE
            XCollieHelper::CancelTimer(xcollieId);
#endif
            return;
        }
        fuse_reply_attr(req, &statBuf, 0);
#ifdef HICOLLIE_ENABLE
        XCollieHelper::CancelTimer(xcollieId);
#endif
        return;
    }
    auto inoPtr = FileOperationsHelper::FindCloudDiskInode(data, static_cast<int64_t>(ino));
    if (inoPtr == nullptr) {
        CLOUD_FILE_FAULT_REPORT(CloudFile::CloudFileFaultInfo{"", CloudFile::FaultOperation::GETATTR,
            CloudFile::FaultType::INODE_FILE, EINVAL, "inode not found"});
        fuse_reply_err(req, EINVAL);
#ifdef HICOLLIE_ENABLE
        XCollieHelper::CancelTimer(xcollieId);
#endif
        return;
    }
    fuse_reply_attr(req, &inoPtr->stat, 0);
#ifdef HICOLLIE_ENABLE
    XCollieHelper::CancelTimer(xcollieId);
#endif
}

void FileOperationsLocal::ReadDir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                                  struct fuse_file_info *fi)
{
    (void) fi;
    string path;
    struct CloudDiskFuseData *data =
        reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    if (ino == FUSE_ROOT_ID) {
        path = FileOperationsHelper::GetCloudDiskRootPath(data->userId);
    } else {
        auto inoPtr = FileOperationsHelper::FindCloudDiskInode(data, static_cast<int64_t>(ino));
        if (inoPtr == nullptr) {
            CLOUD_FILE_FAULT_REPORT(CloudFile::CloudFileFaultInfo{"", CloudFile::FaultOperation::READDIR,
                CloudFile::FaultType::INODE_FILE, EINVAL, "inode not found"});
            fuse_reply_err(req, EINVAL);
            return;
        }
        path = inoPtr->path;
    }
    DIR* dir = opendir(path.c_str());
    if (dir == NULL) {
        CLOUD_FILE_FAULT_REPORT(CloudFile::CloudFileFaultInfo{"", CloudFile::FaultOperation::READDIR,
            CloudFile::FaultType::FILE, errno, "opendir error " + std::to_string(errno) + ", path: " +
            GetAnonyString(path)});
        return;
    }

    struct dirent *entry;
    string entryData;
    size_t len = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        string childPath = FileOperationsHelper::GetCloudDiskLocalPath(data->userId,
            entry->d_name);
        int64_t key = FileOperationsHelper::FindLocalId(data, std::to_string(ino) +
            entry->d_name);
        auto childPtr = FileOperationsHelper::FindCloudDiskInode(data, key);
        if (childPtr == nullptr) {
            childPtr = FileOperationsHelper::GenerateCloudDiskInode(data, ino,
                entry->d_name, childPath.c_str());
        }
        if (childPtr == nullptr) {
            continue;
        }
        FileOperationsHelper::AddDirEntry(req, entryData, len, entry->d_name, childPtr);
    }
    FileOperationsHelper::FuseReplyLimited(req, entryData.c_str(), len, off, size);
    closedir(dir);
    return;
}

static void CleanCache(fuse_req_t req)
{
    LOGI("begin clean clouddisk metafile");
    MetaFileMgr::GetInstance().CloudDiskClearAll();
    fuse_reply_ioctl(req, 0, NULL, 0);
}

void FileOperationsLocal::Ioctl(fuse_req_t req, fuse_ino_t ino, int cmd, void *arg, struct fuse_file_info *fi,
                                unsigned flags, const void *inBuf, size_t inBufsz, size_t outBufsz)
{
    if (static_cast<unsigned int>(cmd) == HMDFS_IOC_CLEAN_CACHE_DAEMON) {
        CleanCache(req);
    } else {
        LOGE("Invalid argument, cmd is not supported");
        fuse_reply_err(req, EINVAL);
        return;
    }
}
} // namespace CloudDisk
} // namespace FileManagement
} // namespace OHOS
