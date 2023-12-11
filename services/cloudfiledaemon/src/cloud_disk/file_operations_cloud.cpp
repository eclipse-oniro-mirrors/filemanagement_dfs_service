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
#include "file_operations_cloud.h"

#include <cerrno>
#include <sys/types.h>
#include <sys/xattr.h>

#include "cloud_disk_inode.h"
#include "cloud_file_utils.h"
#include "database_manager.h"
#include "directory_ex.h"
#include "dk_database.h"
#include "drive_kit.h"
#include "file_operations_helper.h"
#include "securec.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement {
namespace CloudDisk {
using namespace std;
using namespace DriveKit;
namespace {
    static const uint32_t STAT_NLINK_REG = 1;
    static const uint32_t STAT_NLINK_DIR = 2;
    static const uint32_t CLOUD_FILE_LAYER = 2;
    static const uint32_t STAT_MODE_REG = 0770;
    static const uint32_t STAT_MODE_DIR = 0771;
    static const uint32_t MILLISECOND_TO_SECONDS_TIMES = 1000;
}

static void InitInodeAttr(fuse_ino_t parent, struct CloudDiskInode *childInode,
                          const CloudDiskFileInfo &childInfo)
{
    struct CloudDiskInode *parentInode = reinterpret_cast<struct CloudDiskInode *>(parent);
    childInode->stat = parentInode->stat;
    childInode->stat.st_mtime = childInfo.mtime / MILLISECOND_TO_SECONDS_TIMES;
    childInode->stat.st_ctime = childInfo.ctime / MILLISECOND_TO_SECONDS_TIMES;
    childInode->stat.st_atime = childInfo.atime ? childInfo.atime / MILLISECOND_TO_SECONDS_TIMES :
                                childInode->stat.st_mtime;

    childInode->bundleName = parentInode->bundleName;
    childInode->layer = FileOperationsHelper::GetNextLayer(parent);
    childInode->parent = parent;
    childInode->cloudId = childInfo.cloudId;
    childInode->ops = make_shared<FileOperationsCloud>();
    childInode->stat.st_ino = reinterpret_cast<fuse_ino_t>(childInode);

    if (childInfo.IsDirectory) {
        childInode->stat.st_mode = S_IFDIR | STAT_MODE_DIR;
        childInode->stat.st_nlink = STAT_NLINK_DIR;
    } else {
        childInode->stat.st_mode = S_IFREG | STAT_MODE_REG;
        childInode->stat.st_nlink = STAT_NLINK_REG;
        childInode->stat.st_size = childInfo.size;
    }
}

static int32_t DoCloudLookup(fuse_req_t req, fuse_ino_t parent, const char *name,
                             struct fuse_entry_param *e)
{
    bool createFlag = false;
    auto data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    std::unique_lock<std::shared_mutex> wLock(data->cacheLock, std::defer_lock);

    if (parent == FUSE_ROOT_ID) {
        LOGE("cloud file operations should not get a fuse root inode");
        return ENOENT;
    }

    auto parentInode = reinterpret_cast<struct CloudDiskInode *>(parent);
    DatabaseManager &databaseManager = DatabaseManager::GetInstance();
    shared_ptr<CloudDiskRdbStore> rdbStore =
        databaseManager.GetRdbStore(parentInode->bundleName, data->userId);
    CloudDiskFileInfo childInfo;
    int32_t err = rdbStore->LookUp(parentInode->cloudId, name, childInfo);
    if (err != 0) {
        LOGE("lookup %{public}s error, err: %{public}d", name, err);
        return ENOENT;
    }

    shared_ptr<CloudDiskInode> child = FileOperationsHelper::FindCloudDiskInode(data, childInfo.cloudId);
    if (child == nullptr) {
        child = make_shared<CloudDiskInode>();
        createFlag = true;
    }

    child->refCount++;
    if (createFlag) {
        InitInodeAttr(parent, child.get(), childInfo);
        wLock.lock();
        data->inodeCache[childInfo.cloudId] = child;
        wLock.unlock();
    }
    e->ino = reinterpret_cast<fuse_ino_t>(child.get());
    FileOperationsHelper::GetInodeAttr(child, &e->attr);
    return 0;
}

void FileOperationsCloud::Lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    struct fuse_entry_param e;

    int32_t err = DoCloudLookup(req, parent, name, &e);
    if (err) {
        fuse_reply_err(req, err);
    } else {
        fuse_reply_entry(req, &e);
    }
}

void FileOperationsCloud::Access(fuse_req_t req, fuse_ino_t ino, int mask)
{
    LOGI("Access operation is not supported!");
    fuse_reply_err(req, ENOSYS);
}

void FileOperationsCloud::GetAttr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    auto inoPtr = reinterpret_cast<struct CloudDiskInode *>(ino);
    fuse_reply_attr(req, &inoPtr->stat, 0);
}

static bool HandleDkError(fuse_req_t req, DriveKit::DKError dkError)
{
    if (!dkError.HasError()) {
        return false;
    }
    if ((dkError.serverErrorCode == static_cast<int>(DriveKit::DKServerErrorCode::NETWORK_ERROR))
        || dkError.dkErrorCode == DriveKit::DKLocalErrorCode::DOWNLOAD_REQUEST_ERROR) {
        LOGE("network error");
        fuse_reply_err(req, ENOTCONN);
    } else if (dkError.isServerError) {
        LOGE("server errorCode is: %{public}d", dkError.serverErrorCode);
        fuse_reply_err(req, EIO);
    } else if (dkError.isLocalError) {
        LOGE("local errorCode is: %{public}d", dkError.dkErrorCode);
        fuse_reply_err(req, EINVAL);
    }
    return true;
}

static shared_ptr<DriveKit::DKDatabase> GetDatabase(int32_t userId, string bundleName)
{
    auto driveKit = DriveKit::DriveKitNative::GetInstance(userId);
    if (driveKit == nullptr) {
        LOGE("sdk helper get drive kit instance fail");
        return nullptr;
    }

    auto container = driveKit->GetDefaultContainer(bundleName);
    if (container == nullptr) {
        LOGE("sdk helper get drive kit container fail");
        return nullptr;
    }

    shared_ptr<DriveKit::DKDatabase> database = container->GetPrivateDatabase();
    if (database == nullptr) {
        LOGE("sdk helper get drive kit database fail");
        return nullptr;
    }
    return database;
}

static void CloudOpen(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi, string path)
{
    auto *data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    auto inoPtr = reinterpret_cast<struct CloudDiskInode *>(ino);
    shared_ptr<DriveKit::DKDatabase> database = GetDatabase(data->userId, inoPtr->bundleName);
    std::unique_lock<std::shared_mutex> wSesLock(inoPtr->sessionLock, std::defer_lock);

    if (!database) {
        fuse_reply_err(req, EPERM);
        LOGE("database is null");
        return;
    }

    wSesLock.lock();
    if (inoPtr->readSession) {
        inoPtr->sessionRefCount++;
        LOGD("open success, sessionRefCount: %d", inoPtr->sessionRefCount.load());
        fuse_reply_open(req, fi);
        wSesLock.unlock();
        return;
    }

    string cloudId = inoPtr->cloudId;
    LOGD("cloudId: %s", cloudId.c_str());
    inoPtr->readSession = database->NewAssetReadSession("file", cloudId, "content", path);
    if (inoPtr->readSession) {
        DriveKit::DKError dkError = inoPtr->readSession->InitSession();
        if (!HandleDkError(req, dkError)) {
            inoPtr->sessionRefCount++;
            LOGD("open success, sessionRefCount: %d", inoPtr->sessionRefCount.load());
            fuse_reply_open(req, fi);
        } else {
            inoPtr->readSession = nullptr;
            LOGE("open fali");
        }
    } else {
        fuse_reply_err(req, EPERM);
        LOGE("readSession is null");
    }
    wSesLock.unlock();
    return;
}

void FileOperationsCloud::Open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    auto data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    auto inoPtr = reinterpret_cast<struct CloudDiskInode *>(ino);
    string path = CloudFileUtils::GetLocalFilePath(inoPtr->cloudId, inoPtr->bundleName, data->userId);
    if (access(path.c_str(), F_OK) == 0) {
        if ((fi->flags & O_ACCMODE) & O_WRONLY) {
            fi->flags &= ~O_WRONLY;
            fi->flags |= O_RDWR;
        }
        if (fi->flags & O_APPEND) {
            fi->flags &= ~O_APPEND;
        }
        int32_t fd = open(path.c_str(), fi->flags);
        if (fd < 0) {
            LOGE("open file failed path:%{public}s errno:%{public}d", path.c_str(), errno);
            return (void) fuse_reply_err(req, errno);
        }
        fi->fh = fd;
        fuse_reply_open(req, fi);
    } else {
        path = path.substr(0, path.find_last_of("/"));
        CloudOpen(req, ino, fi, path);
    }
}

void FileOperationsCloud::Forget(fuse_req_t req, fuse_ino_t ino, uint64_t nLookup)
{
    auto data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    if (ino == FUSE_ROOT_ID) {
        LOGD("Cloud file operation should not get a root inode");
        return (void) fuse_reply_none(req);
    }
    auto inoPtr = reinterpret_cast<struct CloudDiskInode *>(ino);
    string key = inoPtr->cloudId;
    if (inoPtr->layer != CLOUD_DISK_INODE_OTHER_LAYER) {
        key = inoPtr->path;
    }
    shared_ptr<CloudDiskInode> node = FileOperationsHelper::FindCloudDiskInode(data, key);
    FileOperationsHelper::PutCloudDiskInode(data, node, nLookup);
    fuse_reply_none(req);
}

void FileOperationsCloud::ForgetMulti(fuse_req_t req, size_t count, struct fuse_forget_data *forgets)
{
    auto data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    for (size_t i = 0; i < count; i++) {
        if (forgets[i].ino == FUSE_ROOT_ID) {
            LOGD("Cloud file operation should not get a root inode");
            return (void) fuse_reply_none(req);
        }
        auto inoPtr = reinterpret_cast<struct CloudDiskInode *>(forgets[i].ino);
        string key = inoPtr->cloudId;
        if (inoPtr->layer != CLOUD_DISK_INODE_OTHER_LAYER) {
            key = inoPtr->path;
        }
        shared_ptr<CloudDiskInode> node = FileOperationsHelper::FindCloudDiskInode(data, key);
        FileOperationsHelper::PutCloudDiskInode(data, node, forgets[i].nlookup);
    }
    fuse_reply_none(req);
}

static int32_t CreateLocalFile(const string &cloudId, const string &bundleName, int32_t userId, mode_t mode)
{
    string bucketPath = CloudFileUtils::GetLocalBucketPath(cloudId, bundleName, userId);
    string path = CloudFileUtils::GetLocalFilePath(cloudId, bundleName, userId);
    if (access(bucketPath.c_str(), F_OK) != 0) {
        if (mkdir(bucketPath.c_str(), STAT_MODE_DIR) != 0) {
            LOGE("mkdir bucketpath failed :%{public}s err:%{public}d", bucketPath.c_str(), errno);
            return -errno;
        }
    }
    int32_t fd = open(path.c_str(), (mode & O_NOFOLLOW) | O_CREAT | O_RDWR, STAT_MODE_REG);
    if (fd < 0) {
        LOGE("create file failed :%{public}s err:%{public}d", path.c_str(), errno);
        return -errno;
    }
    return fd;
}

void RemoveLocalFile(const string &path)
{
    int32_t err = remove(path.c_str());
    if (err != 0) {
        LOGE("remove file %{public}s failed, error:%{public}d", path.c_str(), errno);
    }
}

int32_t GenerateCloudId(int32_t userId, const string &bundleName, string &cloudId)
{
    shared_ptr<DKDatabase> dkDatabasePtr = GetDatabase(userId, bundleName);
    if (dkDatabasePtr == nullptr) {
        LOGE("Failed to get database");
        return ENOSYS;
    }

    vector<DKRecordId> ids;
    DKError dkErr = dkDatabasePtr->GenerateIds(1, ids);
    if (dkErr.dkErrorCode != DKLocalErrorCode::NO_ERROR || ids.size() == 0) {
        return ENOSYS;
    }
    cloudId = ids[0];
    return 0;
}

int32_t DoCreatFile(fuse_req_t req, fuse_ino_t parent, const char *name,
                    mode_t mode, struct fuse_entry_param &e)
{
    struct CloudDiskFuseData *data =
        reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    struct CloudDiskInode *parentInode = reinterpret_cast<struct CloudDiskInode *>(parent);

    string cloudId;
    int32_t err = GenerateCloudId(data->userId, parentInode->bundleName, cloudId);
    if (err != 0) {
        LOGE("Failed to generate cloud id");
        return -err;
    }

    int32_t fd = CreateLocalFile(cloudId, parentInode->bundleName, data->userId, mode);
    if (fd < 0) {
        LOGD("Create local file failed error:%{public}d", fd);
        return fd;
    }

    DatabaseManager &databaseManager = DatabaseManager::GetInstance();
    string path = CloudFileUtils::GetLocalFilePath(cloudId, parentInode->bundleName, data->userId);
    shared_ptr<CloudDiskRdbStore> rdbStore =
        databaseManager.GetRdbStore(parentInode->bundleName, data->userId);
    err = rdbStore->Create(cloudId, parentInode->cloudId, name);
    if (err != 0) {
        close(fd);
        RemoveLocalFile(path);
        return -err;
    }

    err = DoCloudLookup(req, parent, name, &e);
    if (err != 0) {
        close(fd);
        RemoveLocalFile(path);
        return -err;
    }
    return fd;
}

void FileOperationsCloud::MkNod(fuse_req_t req, fuse_ino_t parent, const char *name,
                                mode_t mode, dev_t rdev)
{
    struct fuse_entry_param e;
    int32_t err = DoCreatFile(req, parent, name, mode, e);
    if (err < 0) {
        fuse_reply_err(req, -err);
        return;
    }
    close(err);
    fuse_reply_entry(req, &e);
}

void FileOperationsCloud::Create(fuse_req_t req, fuse_ino_t parent, const char *name,
                                 mode_t mode, struct fuse_file_info *fi)
{
    struct fuse_entry_param e;
    int32_t err = DoCreatFile(req, parent, name, mode, e);
    if (err < 0) {
        fuse_reply_err(req, -err);
        return;
    }
    fi->fh = err;
    fuse_reply_create(req, &e, fi);
}

static shared_ptr<CloudDiskInode> GenerateCloudDiskInode(struct CloudDiskFuseData *data, fuse_ino_t parent,
                                                         const CloudDiskFileInfo &childInfo)
{
    std::unique_lock<std::shared_mutex> wLock(data->cacheLock, std::defer_lock);
    shared_ptr<CloudDiskInode> child = make_shared<CloudDiskInode>();
    child->refCount++;
    InitInodeAttr(parent, child.get(), childInfo);
    wLock.lock();
    data->inodeCache[childInfo.cloudId] = child;
    wLock.unlock();
    return child;
}

void FileOperationsCloud::ReadDir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                                  struct fuse_file_info *fi)
{
    auto data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    auto inoPtr = reinterpret_cast<struct CloudDiskInode *>(ino);
    string parentCloudId = inoPtr->cloudId;

    vector<CloudDiskFileInfo> childInfos;
    DatabaseManager &databaseManager = DatabaseManager::GetInstance();
    shared_ptr<CloudDiskRdbStore> rdbStore = databaseManager.GetRdbStore(inoPtr->bundleName, data->userId);
    int32_t err = rdbStore->ReadDir(parentCloudId, childInfos);
    if (err != 0) {
        LOGE("Readdir failed cloudId:%{public}s err:%{public}d", parentCloudId.c_str(), err);
        return (void) fuse_reply_err(req, err);
    }

    string entryData;
    size_t len = 0;
    for (size_t i = 0; i < childInfos.size(); i++) {
        shared_ptr<CloudDiskInode> childPtr = FileOperationsHelper::FindCloudDiskInode(data, childInfos[i].cloudId);
        if (childPtr == nullptr) {
            childPtr = GenerateCloudDiskInode(data, ino, childInfos[i]);
        }
        FileOperationsHelper::AddDirEntry(req, entryData, len, childInfos[i].fileName.c_str(), childPtr);
    }
    FileOperationsHelper::FuseReplyLimited(req, entryData.c_str(), len, off, size);
    return;
}

void FileOperationsCloud::SetXattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                                   const char *value, size_t size, int flags)
{
    LOGD("Setxattr begin name:%{public}s", name);
    if (CloudFileUtils::CheckIsHmdfsPermission(name)) {
        fuse_reply_err(req, 0);
    } else if (CloudFileUtils::CheckIsCloudLocation(name)) {
        auto inoPtr = reinterpret_cast<struct CloudDiskInode *>(ino);
        DatabaseManager &databaseManager = DatabaseManager::GetInstance();
        auto data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
        auto rdbStore = databaseManager.GetRdbStore(inoPtr->bundleName, data->userId);
        int32_t err = rdbStore->SetXAttr(inoPtr->cloudId, CLOUD_FILE_LOCATION, value);
        if (err != 0) {
            LOGE("set cloud id fail %{public}d", err);
            fuse_reply_err(req, EINVAL);
            return;
        }
        fuse_reply_err(req, 0);
    } else {
        fuse_reply_err(req, EINVAL);
    }
}

void FileOperationsCloud::GetXattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                                   size_t size)
{
    auto inoPtr = reinterpret_cast<struct CloudDiskInode *>(ino);
    string buf;
    if (CloudFileUtils::CheckIsHmdfsPermission(name)) {
        buf = to_string(inoPtr->layer + CLOUD_FILE_LAYER);
    } else if (CloudFileUtils::CheckIsCloud(name)) {
        buf = inoPtr->cloudId;
    } else if (CloudFileUtils::CheckIsCloudLocation(name)) {
        DatabaseManager &databaseManager = DatabaseManager::GetInstance();
        auto data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
        auto rdbStore = databaseManager.GetRdbStore(inoPtr->bundleName, data->userId);
        int32_t err = rdbStore->GetXAttr(inoPtr->cloudId, CLOUD_FILE_LOCATION, buf);
        if (err!=0) {
            LOGE("get cloud id error %{public}d", err);
            fuse_reply_err(req, EINVAL);
            return;
        }
    } else {
        fuse_reply_err(req, EINVAL);
        return;
    }
    if (size == 0) {
        fuse_reply_xattr(req, buf.size());
        return;
    }
    if (buf.size() > size) {
        fuse_reply_err(req, ERANGE);
        return;
    }
    if (buf.size() > 0) {
        fuse_reply_buf(req, buf.c_str(), buf.size());
    } else {
        fuse_reply_err(req, 0);
    }
}

void FileOperationsCloud::MkDir(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode)
{
    auto data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    auto parentInode = reinterpret_cast<struct CloudDiskInode *>(parent);
    string cloudId;
    int32_t err = GenerateCloudId(data->userId, parentInode->bundleName, cloudId);
    if (err != 0) {
        LOGE("Failed to generate cloud id");
        return (void) fuse_reply_err(req, err);
    }

    DatabaseManager &databaseManager = DatabaseManager::GetInstance();
    shared_ptr<CloudDiskRdbStore> rdbStore = databaseManager.GetRdbStore(parentInode->bundleName,
                                                                         data->userId);
    err = rdbStore->MkDir(cloudId, parentInode->cloudId, name);
    if (err != 0) {
        LOGE("Failed to mkdir to DB err:%{public}d", err);
        return (void) fuse_reply_err(req, ENOSYS);
    }

    struct fuse_entry_param e;
    err = DoCloudLookup(req, parent, name, &e);
    if (err != 0) {
        LOGE("Faile to find dir %{private}s", name);
        return (void) fuse_reply_err(req, err);
    }
    fuse_reply_entry(req, &e);
    fuse_reply_err(req, 0);
}

int32_t DoCloudUnlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    struct fuse_entry_param e;
    int32_t err = DoCloudLookup(req, parent, name, &e);
    if (err != 0) {
        return ENOENT;
    }

    auto data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    auto parentInode = reinterpret_cast<struct CloudDiskInode *>(parent);
    DatabaseManager &databaseManager = DatabaseManager::GetInstance();
    shared_ptr<CloudDiskRdbStore> rdbStore = databaseManager.GetRdbStore(parentInode->bundleName,
                                                                         data->userId);
    err = rdbStore->Unlink(parentInode->cloudId, name);
    if (err != 0) {
        LOGE("Failed to unlink DB name:%{private}s err:%{public}d", name, err);
        return ENOSYS;
    }
    return 0;
}

void FileOperationsCloud::RmDir(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    int32_t err = DoCloudUnlink(req, parent, name);
    if (err != 0) {
        fuse_reply_err(req, err);
        return;
    }
    return (void) fuse_reply_err(req, 0);
}

void FileOperationsCloud::Unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    int32_t err = DoCloudUnlink(req, parent, name);
    if (err != 0) {
        fuse_reply_err(req, err);
        return;
    }
    return (void) fuse_reply_err(req, 0);
}

void FileOperationsCloud::Rename(fuse_req_t req, fuse_ino_t parent, const char *name,
                                 fuse_ino_t newParent, const char *newName, unsigned int flags)
{
    auto data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    auto parentInode = reinterpret_cast<struct CloudDiskInode *>(parent);
    auto newParentInode = reinterpret_cast<struct CloudDiskInode *>(newParent);
    if (flags) {
        LOGE("Fuse failed to support flag");
        fuse_reply_err(req, EINVAL);
        return;
    }

    DatabaseManager &databaseManager = DatabaseManager::GetInstance();
    shared_ptr<CloudDiskRdbStore> rdbStore = databaseManager.GetRdbStore(parentInode->bundleName,
                                                                         data->userId);
    int32_t err = rdbStore->Rename(parentInode->cloudId, name, newParentInode->cloudId, newName);
    if (err != 0) {
        fuse_reply_err(req, err);
        LOGE("Failed to Rename DB name:%{private}s err:%{public}d", name, err);
        return;
    }
    return (void) fuse_reply_err(req, 0);
}

void FileOperationsCloud::Read(fuse_req_t req, fuse_ino_t ino, size_t size,
                               off_t offset, struct fuse_file_info *fi)
{
    struct fuse_bufvec buf = FUSE_BUFVEC_INIT(size);

    buf.buf[0].flags = static_cast<fuse_buf_flags> (FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK);
    buf.buf[0].fd = fi->fh;
    buf.buf[0].pos = offset;

    fuse_reply_data(req, &buf, static_cast<fuse_buf_copy_flags> (0));
}

static void UpdateCloudDiskInode(shared_ptr<CloudDiskRdbStore> rdbStore, struct CloudDiskInode *inoPtr)
{
    CloudDiskFileInfo childInfo;
    int32_t err = rdbStore->GetAttr(inoPtr->cloudId, childInfo);
    if (err != 0) {
        LOGE("update file fail");
        return;
    }
    inoPtr->stat.st_size = childInfo.size;
    inoPtr->stat.st_mtime = childInfo.mtime / MILLISECOND_TO_SECONDS_TIMES;
}

void FileOperationsCloud::Write(fuse_req_t req, fuse_ino_t ino,  const char *buf, size_t size,
                                off_t off, struct fuse_file_info *fi)
{
    int res = pwrite(fi->fh, buf, size, off);
    if (res < 0) {
        fuse_reply_err(req, errno);
    } else {
        fuse_reply_write(req, (size_t) res);
    }
    auto data = reinterpret_cast<struct CloudDiskFuseData *>(fuse_req_userdata(req));
    auto inoPtr = reinterpret_cast<struct CloudDiskInode *>(ino);
    DatabaseManager &databaseManager = DatabaseManager::GetInstance();
    shared_ptr<CloudDiskRdbStore> rdbStore =
        databaseManager.GetRdbStore(inoPtr->bundleName, data->userId);
    res = rdbStore->Write(inoPtr->cloudId);
    if (res != 0) {
        LOGE("write file fail");
    }
    UpdateCloudDiskInode(rdbStore, inoPtr);
    return;
}

void FileOperationsCloud::Release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    auto inoPtr = reinterpret_cast<struct CloudDiskInode *>(ino);
    if (!inoPtr->readSession) {
        close(fi->fh);
        fuse_reply_err(req, 0);
    } else {
        std::unique_lock<std::shared_mutex> wSesLock(inoPtr->sessionLock, std::defer_lock);

        wSesLock.lock();
        inoPtr->sessionRefCount--;
        if (inoPtr->sessionRefCount == 0) {
            bool res = inoPtr->readSession->Close(false);
            if (!res) {
                LOGE("close error");
            }
            inoPtr->readSession = nullptr;
            LOGD("readSession released");
        }
        wSesLock.unlock();
    }
}

void FileOperationsCloud::SetAttr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
                                  int valid, struct fuse_file_info *fi)
{
    LOGI("SetAttr operation begin");
    fuse_reply_err(req, 0);
}

void FileOperationsCloud::Lseek(fuse_req_t req, fuse_ino_t ino, off_t off, int whence,
                                struct fuse_file_info *fi)
{
    off_t res = lseek(fi->fh, off, whence);
    if (res != -1)
        fuse_reply_lseek(req, res);
    else
        fuse_reply_err(req, errno);
}
} // namespace CloudDisk
} // namespace FileManagement
} // namespace OHOS
