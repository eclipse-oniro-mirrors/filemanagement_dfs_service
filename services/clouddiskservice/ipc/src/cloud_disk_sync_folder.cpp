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

#include "cloud_disk_sync_folder.h"
#include "cloud_disk_service_error.h"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>
#include <utility>

#include "disk_monitor.h"
#include "placeholder_helper.h"
#include "unique_fd.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement {
namespace CloudDiskService {
using namespace std;

CloudDiskSyncFolder &CloudDiskSyncFolder::GetInstance()
{
    static CloudDiskSyncFolder instance;
    return instance;
}

void CloudDiskSyncFolder::AddSyncFolder(const uint32_t &syncFolderIndex, const SyncFolderValue &syncFolderValue)
{
    unique_lock<mutex> lock(mutex_);
    auto item = syncFolderMap.find(syncFolderIndex);
    if (item == syncFolderMap.end()) {
        syncFolderMap[syncFolderIndex] = syncFolderValue;
    }
}

void CloudDiskSyncFolder::DeleteSyncFolder(const uint32_t &syncFolderIndex)
{
    unique_lock<mutex> lock(mutex_);
    auto item = syncFolderMap.find(syncFolderIndex);
    if (item != syncFolderMap.end()) {
        syncFolderMap.erase(syncFolderIndex);
    }
}

int32_t CloudDiskSyncFolder::GetSyncFolderSize()
{
    unique_lock<mutex> lock(mutex_);
    return syncFolderMap.size();
}

bool CloudDiskSyncFolder::CheckSyncFolder(const uint32_t &syncFolderIndex)
{
    unique_lock<mutex> lock(mutex_);
    auto it = syncFolderMap.find(syncFolderIndex);
    if (it == syncFolderMap.end()) {
        return false;
    }
    return true;
}

bool CloudDiskSyncFolder::GetSyncFolderByIndex(const uint32_t syncFolderIndex, string &path)
{
    unique_lock<mutex> lock(mutex_);
    auto item = syncFolderMap.find(syncFolderIndex);
    if (item == syncFolderMap.end()) {
        return false;
    }
    path = item->second.path;
    return true;
}

bool CloudDiskSyncFolder::GetSyncFolderValueByIndex(const uint32_t syncFolderIndex, SyncFolderValue &syncFolderValue)
{
    unique_lock<mutex> lock(mutex_);
    auto item = syncFolderMap.find(syncFolderIndex);
    if (item == syncFolderMap.end()) {
        return false;
    }
    syncFolderValue = item->second;
    return true;
}

bool CloudDiskSyncFolder::GetIndexBySyncFolder(uint32_t &syncFolderIndex, const string &path)
{
    unique_lock<mutex> lock(mutex_);
    for (const auto &item : syncFolderMap) {
        if (item.second.path == path) {
            syncFolderIndex = item.first;
            return true;
        }
    }
    return false;
}

std::unordered_map<uint32_t, SyncFolderValue> CloudDiskSyncFolder::GetSyncFolderMap()
{
    unique_lock<mutex> lock(mutex_);
    return syncFolderMap;
}

namespace {
void RecordCleanupFailure(PlaceholderCleanupStats &stats, int32_t error)
{
    ++stats.failed;
    if (stats.firstError == 0) {
        stats.firstError = error;
    }
}

bool RemoveNodeXattr(int fd, const char *attrName, const std::string &path, PlaceholderCleanupStats &stats)
{
    if (fremovexattr(fd, attrName) == 0 || errno == ENODATA || errno == ENOENT) {
        return true;
    }
    int32_t error = errno;
    LOGE("remove xattr failed for path:%{public}s, errno:%{public}d", GetAnonyStringStrictly(path).c_str(), error);
    RecordCleanupFailure(stats, error);
    return false;
}

struct CleanupEntry {
    int dirFd = -1;
    std::string name;
    std::string path;
    struct stat stat {};
};

void CleanPlaceholderDirectory(int dirFd, const std::string &path, PlaceholderCleanupStats &stats);

// Walk from "/" one component at a time so no component of the cleanup root can be a symlink.
// The final component is returned as a name so callers can decide its type without opening it.
int32_t OpenParentNoFollow(const std::string &path, UniqueFd &parentFd, std::string &leaf)
{
    if (path.empty() || path.front() != '/') {
        LOGE("Cleanup path is not absolute");
        return EINVAL;
    }
    std::string normalized = path;
    while (normalized.size() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }
    size_t slash = normalized.find_last_of('/');
    if (slash == std::string::npos) {
        return EINVAL;
    }
    leaf = normalized.substr(slash + 1);
    if (leaf.empty() || leaf == "." || leaf == "..") {
        LOGE("Cleanup path has invalid leaf");
        return EINVAL;
    }
    std::string parent = slash == 0 ? "/" : normalized.substr(0, slash);

    UniqueFd current(open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC));
    if (current < 0) {
        int32_t error = errno;
        LOGE("Open cleanup anchor failed, errno:%{public}d", error);
        return error;
    }
    size_t start = 1;
    while (start < parent.size()) {
        size_t end = parent.find('/', start);
        std::string component = parent.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (component.empty() || component == "." || component == "..") {
            LOGE("Cleanup path has invalid component");
            return EINVAL;
        }
        UniqueFd next(openat(current.Get(), component.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
        if (next < 0) {
            int32_t error = errno;
            LOGE("Open cleanup path component failed, errno:%{public}d", error);
            return error;
        }
        current = std::move(next);
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    parentFd = std::move(current);
    return E_OK;
}

bool ValidateCleanupFile(const CleanupEntry &entry, int fileFd, struct stat &opened,
                         PlaceholderCleanupStats &stats)
{
    if (fstat(fileFd, &opened) != 0) {
        int32_t error = errno;
        LOGE("fstat failed for path:%{public}s, errno:%{public}d", GetAnonyStringStrictly(entry.path).c_str(), error);
        RecordCleanupFailure(stats, error);
        return false;
    }
    if (opened.st_dev != entry.stat.st_dev || opened.st_ino != entry.stat.st_ino) {
        ++stats.inconsistent;
        LOGW("skip replaced cleanup entry, path:%{public}s", GetAnonyStringStrictly(entry.path).c_str());
        return false;
    }
    if (!S_ISREG(opened.st_mode)) {
        ++stats.kept;
        return false;
    }
    if (opened.st_nlink > 1) {
        // Hard links share the inode with names outside the sync root; clearing xattr or
        // unlinking one name would affect the other names. Keep the entry untouched.
        ++stats.hardLinks;
        LOGW("skip hard-linked placeholder, path:%{public}s, nlink:%{public}lu",
             GetAnonyStringStrictly(entry.path).c_str(), static_cast<unsigned long>(opened.st_nlink));
        return false;
    }
    return true;
}

bool ReadCleanupFileState(const CleanupEntry &entry, int fileFd, uint8_t &state, PlaceholderCleanupStats &stats)
{
    // Missing state is not the same as state 0: a file inside a sync root that lost its xattr
    // must be reported instead of being treated as an unmanaged file.
    uint8_t stateByte = 0;
    int32_t ret = GetFileSyncStateByte(fileFd, stateByte, false);
    if (ret == ENODATA) {
        ++stats.inconsistent;
        LOGW("keep file with missing placeholder state, path:%{public}s",
             GetAnonyStringStrictly(entry.path).c_str());
        return false;
    }
    if (ret != 0) {
        LOGE("get placeholder state failed for path:%{public}s, errno:%{public}d",
             GetAnonyStringStrictly(entry.path).c_str(), ret);
        RecordCleanupFailure(stats, ret);
        return false;
    }
    state = GetPlaceholderStateFromFileSyncState(stateByte);
    if (!IsValidPlaceholderState(state)) {
        ++stats.inconsistent;
        LOGW("keep file with invalid placeholder state, path:%{public}s, state:%{public}u",
             GetAnonyStringStrictly(entry.path).c_str(), state);
        return false;
    }
    return true;
}

bool UnlinkCleanupPlaceholder(const CleanupEntry &entry, const struct stat &opened, PlaceholderCleanupStats &stats)
{
    struct stat current {};
    if (fstatat(entry.dirFd, entry.name.c_str(), &current, AT_SYMLINK_NOFOLLOW) != 0) {
        int32_t error = errno;
        if (error == ENOENT) {
            LOGW("cleanup entry disappeared before unlink, path:%{public}s",
                 GetAnonyStringStrictly(entry.path).c_str());
            return false;
        }
        LOGE("fstatat before unlink failed for path:%{public}s, errno:%{public}d",
             GetAnonyStringStrictly(entry.path).c_str(), error);
        RecordCleanupFailure(stats, error);
        return false;
    }
    if (S_ISLNK(current.st_mode)) {
        ++stats.skippedLinks;
        return false;
    }
    if (S_ISREG(current.st_mode) && current.st_nlink > 1) {
        ++stats.hardLinks;
        LOGW("skip hard-linked placeholder before unlink, path:%{public}s",
             GetAnonyStringStrictly(entry.path).c_str());
        return false;
    }
    if (!S_ISREG(current.st_mode) || current.st_dev != opened.st_dev || current.st_ino != opened.st_ino) {
        ++stats.inconsistent;
        LOGW("skip replaced cleanup entry before unlink, path:%{public}s",
             GetAnonyStringStrictly(entry.path).c_str());
        return false;
    }
    if (unlinkat(entry.dirFd, entry.name.c_str(), 0) != 0) {
        int32_t error = errno;
        if (error == ENOENT) {
            return false;
        }
        // Keep the state xattr: the file is still an incomplete placeholder and a later
        // retry must be able to identify it.
        LOGE("unlinkat failed for path:%{public}s, errno:%{public}d",
             GetAnonyStringStrictly(entry.path).c_str(), error);
        RecordCleanupFailure(stats, error);
        return false;
    }
    ++stats.removed;
    return true;
}

void CleanPlaceholderFile(const CleanupEntry &entry, int fileFd, PlaceholderCleanupStats &stats)
{
    struct stat opened {};
    if (!ValidateCleanupFile(entry, fileFd, opened, stats)) {
        return;
    }
    uint8_t state = 0;
    if (!ReadCleanupFileState(entry, fileFd, state, stats)) {
        return;
    }
    if (state == PLACEHOLDER_STATE_UNHYDRATED || state == PLACEHOLDER_STATE_PARTIALLY_HYDRATED) {
        (void)UnlinkCleanupPlaceholder(entry, opened, stats);
        return;
    }
    ++stats.kept;
    (void)RemoveNodeXattr(fileFd, CLOUD_DISK_FILE_SYNC_STATE_XATTR, entry.path, stats);
    (void)RemoveNodeXattr(fileFd, CLOUD_DISK_PLACEHOLDER_COUNT_XATTR, entry.path, stats);
}

void CleanDirectoryChildDirectory(int dirFd, const CleanupEntry &entry, PlaceholderCleanupStats &stats)
{
    UniqueFd childFd(openat(dirFd, entry.name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    if (childFd < 0) {
        int32_t error = errno;
        if (error == ELOOP) {
            ++stats.skippedLinks;
        } else if (error != ENOENT) {
            LOGE("open directory failed for path:%{public}s, errno:%{public}d",
                 GetAnonyStringStrictly(entry.path).c_str(), error);
            RecordCleanupFailure(stats, error);
        }
        return;
    }
    CleanPlaceholderDirectory(childFd.Release(), entry.path, stats);
}

void CleanDirectoryChildFile(int dirFd, const CleanupEntry &entry, PlaceholderCleanupStats &stats)
{
    UniqueFd fileFd(openat(dirFd, entry.name.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK));
    if (fileFd < 0) {
        int32_t error = errno;
        if (error == ELOOP) {
            ++stats.skippedLinks;
        } else if (error != ENOENT) {
            LOGE("open file failed for path:%{public}s, errno:%{public}d",
                 GetAnonyStringStrictly(entry.path).c_str(), error);
            RecordCleanupFailure(stats, error);
        }
        return;
    }
    CleanPlaceholderFile(entry, fileFd.Get(), stats);
}

void CleanDirectoryEntry(int dirFd, const CleanupEntry &entry, PlaceholderCleanupStats &stats)
{
    if (S_ISLNK(entry.stat.st_mode)) {
        ++stats.skippedLinks;
        LOGW("skip symlink during placeholder cleanup, path:%{public}s",
             GetAnonyStringStrictly(entry.path).c_str());
        return;
    }
    if (S_ISDIR(entry.stat.st_mode)) {
        CleanDirectoryChildDirectory(dirFd, entry, stats);
        return;
    }
    if (!S_ISREG(entry.stat.st_mode)) {
        ++stats.kept;
        return;
    }
    CleanDirectoryChildFile(dirFd, entry, stats);
}

struct dirent *ReadNextDirEntry(DIR *dir)
{
    errno = 0;
    return readdir(dir);
}

void CleanupDirectoryEntries(DIR *dir, int dirFd, const std::string &path, PlaceholderCleanupStats &stats)
{
    for (struct dirent *entry = ReadNextDirEntry(dir); entry != nullptr; entry = ReadNextDirEntry(dir)) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        CleanupEntry item {dirFd, entry->d_name, path + "/" + entry->d_name, {}};
        if (fstatat(dirFd, item.name.c_str(), &item.stat, AT_SYMLINK_NOFOLLOW) != 0) {
            int32_t error = errno;
            if (error == ENOENT) {
                continue;
            }
            LOGE("fstatat failed for path:%{public}s, errno:%{public}d",
                 GetAnonyStringStrictly(item.path).c_str(), error);
            RecordCleanupFailure(stats, error);
            continue;
        }
        CleanDirectoryEntry(dirFd, item, stats);
    }
    if (errno != 0) {
        int32_t error = errno;
        LOGE("read directory failed, path:%{public}s, errno:%{public}d",
             GetAnonyStringStrictly(path).c_str(), error);
        RecordCleanupFailure(stats, error);
    }
}

void CleanPlaceholderDirectory(int dirFd, const std::string &path, PlaceholderCleanupStats &stats)
{
    DIR *dir = fdopendir(dirFd);
    if (dir == nullptr) {
        int32_t error = errno;
        close(dirFd);
        LOGE("open directory failed, path:%{public}s, errno:%{public}d", GetAnonyStringStrictly(path).c_str(),
             error);
        RecordCleanupFailure(stats, error);
        return;
    }
    uint32_t failedBefore = stats.failed;
    uint32_t inconsistentBefore = stats.inconsistent;
    uint32_t hardLinksBefore = stats.hardLinks;
    CleanupDirectoryEntries(dir, dirFd, path, stats);

    bool subtreeClean = stats.failed == failedBefore && stats.inconsistent == inconsistentBefore &&
                        stats.hardLinks == hardLinksBefore;
    if (subtreeClean) {
        (void)RemoveNodeXattr(dirFd, CLOUD_DISK_PLACEHOLDER_COUNT_XATTR, path, stats);
        (void)RemoveNodeXattr(dirFd, CLOUD_DISK_FILE_SYNC_STATE_XATTR, path, stats);
    } else {
        // Directory xattrs describe the whole subtree; only drop them when every child was
        // handled successfully and consistently, so a retry can still walk the incomplete
        // placeholders and the count cannot silently diverge.
        LOGW("keep directory xattrs because child cleanup was incomplete, path:%{public}s",
             GetAnonyStringStrictly(path).c_str());
    }
    if (closedir(dir) != 0) {
        LOGW("close directory failed, path:%{public}s, errno:%{public}d", GetAnonyStringStrictly(path).c_str(),
             errno);
    }
}

void CleanCleanupRoot(int parentFd, const CleanupEntry &rootEntry, PlaceholderCleanupStats &stats)
{
    if (S_ISLNK(rootEntry.stat.st_mode)) {
        ++stats.skippedLinks;
        LOGW("skip symlink root during placeholder cleanup, path:%{public}s",
             GetAnonyStringStrictly(rootEntry.path).c_str());
        return;
    }
    if (S_ISDIR(rootEntry.stat.st_mode)) {
        UniqueFd dirFd(openat(parentFd, rootEntry.name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
        if (dirFd < 0) {
            int32_t error = errno;
            if (error == ELOOP) {
                ++stats.skippedLinks;
            } else {
                LOGE("open cleanup root failed, path:%{public}s, errno:%{public}d",
                     GetAnonyStringStrictly(rootEntry.path).c_str(), error);
                RecordCleanupFailure(stats, error);
            }
            return;
        }
        CleanPlaceholderDirectory(dirFd.Release(), rootEntry.path, stats);
        return;
    }
    if (!S_ISREG(rootEntry.stat.st_mode)) {
        ++stats.kept;
        return;
    }
    UniqueFd fileFd(openat(parentFd, rootEntry.name.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK));
    if (fileFd < 0) {
        int32_t error = errno;
        if (error == ELOOP) {
            ++stats.skippedLinks;
        } else {
            LOGE("open cleanup root file failed, path:%{public}s, errno:%{public}d",
                 GetAnonyStringStrictly(rootEntry.path).c_str(), error);
            RecordCleanupFailure(stats, error);
        }
        return;
    }
    CleanPlaceholderFile(rootEntry, fileFd.Get(), stats);
}

void RecordCleanupRootOpenFailure(const std::string &path, int32_t ret, PlaceholderCleanupStats &stats)
{
    if (ret == ELOOP) {
        ++stats.skippedLinks;
        LOGW("skip symlink in cleanup root path, path:%{public}s", GetAnonyStringStrictly(path).c_str());
        return;
    }
    LOGE("resolve cleanup root failed, path:%{public}s, errno:%{public}d", GetAnonyStringStrictly(path).c_str(), ret);
    RecordCleanupFailure(stats, ret);
}

} // namespace

PlaceholderCleanupStats CloudDiskSyncFolder::RemovePlaceholderFilesBatch(const string &path)
{
    PlaceholderCleanupStats stats;
    UniqueFd parentFd;
    std::string leaf;
    int32_t ret = OpenParentNoFollow(path, parentFd, leaf);
    if (ret != E_OK) {
        RecordCleanupRootOpenFailure(path, ret, stats);
        return stats;
    }

    CleanupEntry rootEntry {parentFd.Get(), leaf, path, {}};
    if (fstatat(parentFd.Get(), leaf.c_str(), &rootEntry.stat, AT_SYMLINK_NOFOLLOW) != 0) {
        int32_t error = errno;
        LOGE("stat cleanup root failed, path:%{public}s, errno:%{public}d", GetAnonyStringStrictly(path).c_str(),
             error);
        RecordCleanupFailure(stats, error);
        return stats;
    }
    CleanCleanupRoot(parentFd.Get(), rootEntry, stats);
    LOGI("Placeholder cleanup finished, removed:%{public}u, kept:%{public}u, skippedLinks:%{public}u, "
         "hardLinks:%{public}u, inconsistent:%{public}u, failed:%{public}u, firstError:%{public}d, path:%{private}s",
         stats.removed, stats.kept, stats.skippedLinks, stats.hardLinks, stats.inconsistent, stats.failed,
         stats.firstError, path.c_str());
    return stats;
}

/* Verify the path specified by inputPath, replace its prefix oldPrefix with the new prefix newPrefix. */
int32_t CloudDiskSyncFolder::ReplacePathPrefix(const string &oldPrefix,
                                               const string &newPrefix,
                                               const string &inputPath,
                                               string &outputPath)
{
    if ((inputPath.size() < oldPrefix.size()) || (inputPath.substr(0, oldPrefix.size()) != oldPrefix)) {
        LOGE("Invalid path prefix: %{public}s, input: %{public}s", GetAnonyStringStrictly(oldPrefix).c_str(),
             GetAnonyStringStrictly(inputPath).c_str());
        return E_INVALID_ARG;
    }

    string newPath = newPrefix + inputPath.substr(oldPrefix.size());

    char realPathBuffer[PATH_MAX + 1]{'\0'};
    if (realpath(newPath.c_str(), realPathBuffer) == nullptr) {
        LOGE("Realpath error: %{public}d, path=%{public}s", errno, GetAnonyStringStrictly(newPath).c_str());
        return (errno == ENOENT) ? E_SYNC_FOLDER_PATH_NOT_EXIST : E_INVALID_ARG;
    }

    string newRealPath = string(realPathBuffer);
    if (newRealPath.substr(0, newPrefix.size()) != newPrefix) {
        LOGE("Path traversal prefix: %{public}s, newRealPath: %{public}s", GetAnonyStringStrictly(newPrefix).c_str(),
             GetAnonyStringStrictly(newRealPath).c_str());
        return E_INVALID_ARG;
    }

    outputPath = newRealPath;
    return E_OK;
}

int32_t CloudDiskSyncFolder::PathToPhysicalPath(const string &path, const string &userId, string &realPath)
{
    string sandboxPath = "/storage/Users/currentUser/";
    string replacementPath = "/data/service/el2/" + userId + "/hmdfs/account/files/Docs/";

    return ReplacePathPrefix(sandboxPath, replacementPath, path, realPath);
}

int32_t CloudDiskSyncFolder::PathToMntPathBySandboxPath(const string &path, const string &userId, string &realPath)
{
    string sandboxPath = "/storage/Users/currentUser";
    string replacementPath = "/mnt/hmdfs/" + userId + "/account/device_view/local/files/Docs";

    return ReplacePathPrefix(sandboxPath, replacementPath, path, realPath);
}

bool CloudDiskSyncFolder::PathToMntPathByPhysicalPath(const string &path, const string &userId, string &realPath)
{
    string physicalPath = "/data/service/el2/" + userId + "/hmdfs/account/files/Docs";
    string replacementPath = "/mnt/hmdfs/" + userId + "/account/device_view/local/files/Docs";

    if (path.substr(0, physicalPath.length()) != physicalPath) {
        LOGE("replace path failed");
        return false;
    }
    realPath = replacementPath + path.substr(physicalPath.length());
    return true;
}

bool CloudDiskSyncFolder::PathToSandboxPathByPhysicalPath(const string &path, const string &userId, string &realPath)
{
    string physicalPath = "/data/service/el2/" + userId + "/hmdfs/account/files/Docs";
    string replacementPath = "/storage/Users/currentUser";

    if (path.substr(0, physicalPath.length()) != physicalPath) {
        LOGE("replace path failed");
        return false;
    }
    realPath = replacementPath + path.substr(physicalPath.length());
    return true;
}

void CloudDiskSyncFolder::ClearMap()
{
    unique_lock<mutex> lock(mutex_);
    syncFolderMap.clear();
}
} // namespace CloudDiskService
} // namespace FileManagement
} // namespace OHOS
