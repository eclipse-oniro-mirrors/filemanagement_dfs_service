/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include "clouddisk_notify_utils_mock.h"

#include "clouddisk_notify_utils.h"
#include "clouddisk_rdb_utils.h"
#include "file_column.h"
#include "cloud_pref_impl.h"
#include "dfs_error.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudDisk {
const uint32_t MAX_QUERY_TIMES = 1024;
const string BUNDLENAME_FLAG = "<BundleName>";
const string CLOUDDISK_URI_PREFIX = "file://<BundleName>/data/storage/el2/cloud";
const string BACKFLASH = "/";
const int32_t MOCKUSERID = 10;
const int32_t CloudDiskNotifyUtils::maxCacheCnt_;
list<pair<string, CacheNode>> CloudDiskNotifyUtils::cacheList_;
unordered_map<string, list<pair<string, CacheNode>>::iterator> CloudDiskNotifyUtils::cacheMap_;
mutex CloudDiskNotifyUtils::cacheMutex_;

static bool isRoot(const fuse_ino_t &ino)
{
    return ino == FUSE_ROOT_ID;
}

static int32_t GetUriRecursively(CloudDiskFuseData* data, FindCloudDiskInodeFunc func,
    shared_ptr<CloudDiskInode> inoPtr, string &uri)
{
    string bundleName = inoPtr->bundleName;
    string realPrefix = CLOUDDISK_URI_PREFIX;
    realPrefix.replace(realPrefix.find(BUNDLENAME_FLAG), BUNDLENAME_FLAG.length(), bundleName);
    uint32_t queryTimes = 0;
    while (!isRoot(inoPtr->parent)) {
        inoPtr = func(data, inoPtr->parent);
        if (!inoPtr || inoPtr->fileName.empty()) {
            break;
        }
        uri = inoPtr->fileName + BACKFLASH + uri;
        queryTimes++;
        if (uri.length() > PATH_MAX || queryTimes > MAX_QUERY_TIMES) {
            return E_INVAL_ARG;
        }
    }
    uri = realPrefix + BACKFLASH + uri;
    LOGD("GetUriRecursively uri: %{public}s", GetAnonyString(uri).c_str());
    return E_OK;
}

int32_t CloudDiskNotifyUtils::GetNotifyData(CloudDiskFuseData* data, FindCloudDiskInodeFunc func,
    const fuse_ino_t &ino, NotifyData &notifyData)
{
    if (data->userId == MOCKUSERID) {
        LOGI("AAA");
        return E_INVAL_ARG;
    }
    return E_OK;
}

int32_t CloudDiskNotifyUtils::GetNotifyData(CloudDiskFuseData* data, FindCloudDiskInodeFunc func,
    const fuse_ino_t &parent, const string &name, NotifyData &notifyData)
{
    if (data->userId == MOCKUSERID || name == "mock") {
        return E_INVAL_ARG;
    }
    return E_OK;
}

int32_t CloudDiskNotifyUtils::GetNotifyData(CloudDiskFuseData* data, FindCloudDiskInodeFunc func,
    shared_ptr<CloudDiskInode> inoPtr, NotifyData &notifyData)
{
    if (data->userId == MOCKUSERID) {
        return E_INVAL_ARG;
    }
    return E_OK;
}

int32_t CloudDiskNotifyUtils::GetNotifyData(CloudDiskFuseData* data, FindCloudDiskInodeFunc func,
    shared_ptr<CloudDiskInode> pInoPtr, const string &name, NotifyData &notifyData)
{
    if (data->userId == MOCKUSERID || name == "mock") {
        return E_INVAL_ARG;
    }
    return E_OK;
}

int32_t CloudDiskNotifyUtils::GetCacheNode(const string &cloudId, CacheNode &cacheNode)
{
    lock_guard<mutex> lock(cacheMutex_);
    auto it = cacheMap_.find(cloudId);
    if (it == cacheMap_.end()) {
        LOGI("Not fount in cache, id: %{public}s", cloudId.c_str());
        return E_INVAL_ARG;
    }
    cacheList_.splice(cacheList_.begin(), cacheList_, it->second);
    cacheNode = it->second->second;
    return E_OK;
}

void CloudDiskNotifyUtils::PutCacheNode(const string &cloudId, const CacheNode &cacheNode)
{
    if (cacheNode.isDir != TYPE_DIR_STR) {
        return;
    }
    lock_guard<mutex> lock(cacheMutex_);
    auto it = cacheMap_.find(cloudId);
    if (it != cacheMap_.end()) {
        LOGD("update cache name: %{public}s", GetAnonyString(cacheNode.fileName).c_str());
        it->second->second = cacheNode;
        cacheList_.splice(cacheList_.begin(), cacheList_, it->second);
        return;
    }
    if (cacheMap_.size() == maxCacheCnt_) {
        LOGI("upto max, delete last one");
        string deleteCloudId = cacheList_.back().first;
        cacheList_.pop_back();
        cacheMap_.erase(deleteCloudId);
    }
    LOGD("insert to cache name: %{public}s", GetAnonyString(cacheNode.fileName).c_str());
    cacheList_.emplace_front(cloudId, cacheNode);
    cacheMap_[cloudId] = cacheList_.begin();
}

int32_t CloudDiskNotifyUtils::GetUriFromCache(const string &bundleName,
                                              const string &rootId,
                                              const CacheNode &cacheNode,
                                              string &uri)
{
    if (uri == "mock") {
        return E_RDB;
    }
    uri = ICloudDiskNotifyUtils::ins == nullptr ? uri : ICloudDiskNotifyUtils::ins->GetUriFromCache();
    return E_OK;
}
} // namespace OHOS::FileManagement::CloudDisk