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
#ifndef CLOUD_FILE_DAEMON_CLOUD_FILE_UTILS_H
#define CLOUD_FILE_DAEMON_CLOUD_FILE_UTILS_H

#include <string>
#include <sys/stat.h>
namespace OHOS {
namespace FileManagement {
namespace CloudDisk {
struct CloudDiskFileInfo {
    std::string name;
    std::string cloudId;
    std::string parentCloudId;
    uint32_t location{1};
    uint32_t mode{S_IFREG};
    int64_t localId{0};
    unsigned long long rowId{0};
    unsigned long long size{0};
    unsigned long long atime{0};
    unsigned long long ctime{0};
    unsigned long long mtime{0};
    bool IsDirectory{false};
    off_t nextOff{0};
};

namespace {
    static const std::string CLOUD_CLOUD_ID_XATTR = "user.cloud.cloudid";
    static const std::string CLOUD_FILE_LOCATION = "user.cloud.location";
    static const std::string HMDFS_PERMISSION_XATTR = "user.hmdfs.perm";
    static const std::string CLOUD_CLOUD_RECYCLE_XATTR = "user.cloud.recycle";
    static const std::string IS_FAVORITE_XATTR = "user.cloud.favorite";
    static const std::string IS_FILE_STATUS_XATTR = "user.cloud.filestatus";
    static const std::string CLOUD_EXT_ATTR = "extended_attributes";
    static const std::string CLOUD_HAS_LCD = "user.cloud.hasLcdThumbnail";
    static const std::string CLOUD_HAS_THM = "user.cloud.hasThumbnail";
    static const std::string CLOUD_TIME_RECYCLED = "user.cloud.deletetime";
    static const std::string CLOUD_RECYCLE_PATH = "user.cloud.recyclePath";
}

class CloudFileUtils final {
public:
    static bool CheckIsCloud(const std::string &key);
    static bool CheckIsCloudLocation(const std::string &key);
    static bool CheckIsHasLCD(const std::string &key);
    static bool CheckIsHasTHM(const std::string &key);
    static bool CheckIsHmdfsPermission(const std::string &key);
    static bool CheckIsCloudRecycle(const std::string &key);
    static bool CheckIsFavorite(const std::string &key);
    static bool CheckFileStatus(const std::string &key);
    static bool CheckIsTimeRecycled(const std::string &key);
    static bool CheckIsRecyclePath(const std::string &key);
    static std::string GetLocalBaseDir(std::string bundleName, int32_t userId);
    static std::string GetLocalBucketPath(std::string cloudId, std::string bundleName,
                                          int32_t userId);
    static std::string GetLocalDKCachePath(std::string cloudId, std::string bundleName, int32_t userId);
    static std::string GetLocalFilePath(std::string cloudId, std::string bundleName,
                                        int32_t userId);
    static std::string GetPathWithoutTmp(const std::string &path);
    static std::string GetCloudId(const std::string &path);
    static std::string GetRealPath(const std::string &path);
    static uint32_t GetBucketId(std::string cloudId);
    static int64_t Timespec2Milliseconds(const struct timespec &time);
    static bool LocalWriteOpen(const std::string &dfsPath);
    static bool ClearCache(const std::string &dfsPath, const std::string &cloudPath);
    static uint32_t DentryHash(const std::string &cloudId);
    static void TeaTransform(uint32_t buf[4], uint32_t const in[]);
    static void Str2HashBuf(const char *msg, size_t len, uint32_t *buf, int num);
    static bool IsDotDotdot(const std::string &name);
    static void ChangeUid(int32_t userId, const std::string &bundleName, uint32_t mode, const std::string &path);
    static void ChangeUidByCloudId(int32_t userId, const std::string &bundleName,
        const std::string &cloudId, uint32_t mode, uid_t uid);
    static void ChangeUidByPath(const std::string &path, mode_t mode, uid_t uid);
    static const std::string TMP_SUFFIX;
private:
    static bool EndsWith(const std::string &fullString, const std::string &ending);
};
} // namespace CloudDisk
} // namespace FileManagement
} // namespace OHOS
#endif // CLOUD_FILE_DAEMON_CLOUD_FILE_UTILS_H
