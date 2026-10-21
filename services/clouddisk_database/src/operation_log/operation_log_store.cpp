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

#include "operation_log_store.h"

#include <sys/stat.h>
#include <unistd.h>

#include "dfs_error.h"
#include "rdb_errno.h"
#include "rdb_sql_utils.h"
#include "utils_directory.h"
#include "utils_log.h"

#include "operation_log_column.h"
#include "operation_log_const.h"

namespace OHOS {
namespace FileManagement {
namespace CloudDisk {

using namespace std;
using namespace OHOS::NativeRdb;

constexpr uid_t OID_DFS = 1009;

OperationLogStore& OperationLogStore::GetInstance()
{
    static OperationLogStore instance;
    return instance;
}

int32_t OperationLogStore::Init(int32_t userId, bool isAnon)
{
    lock_guard<mutex> lock(mutex_);
    bool& isInited = isAnon ? anonIsInited_ : isInited_;
    if (isInited) {
        return E_OK;
    }
    userId_ = userId;
    int32_t ret = RdbInit(userId, isAnon);
    if (ret != E_OK) {
        LOGE("OperationLogStore Init failed, isAnon=%{public}d, ret=%{public}d", isAnon, ret);
        return ret;
    }
    isInited = true;
    return E_OK;
}

int32_t OperationLogStore::RdbInit(int32_t userId, bool isAnon)
{
    const std::string& databaseName = isAnon ? OperationLogConst::ANON_DATABASE_NAME :
        OperationLogConst::DATABASE_NAME;
    auto& config = isAnon ? anonConfig_ : config_;
    string databaseDir = OperationLogConst::DATABASE_ROOT_DIR + to_string(userId) +
        OperationLogConst::DATABASE_RELATIVE_DIR;
    int32_t ret = CheckAndCreateDir(databaseDir);
    if (ret != E_OK) {
        LOGE("check and create operation log db dir failed, ret = %{public}d", ret);
        return ret;
    }

    string databasePath = RdbSqlUtils::GetDefaultDatabasePath(databaseDir, databaseName, ret);
    if (ret != NativeRdb::E_OK) {
        LOGE("create default database path failed, errCode = %{public}d", ret);
        return E_PATH;
    }

    config.SetName(databaseName);
    config.SetPath(databasePath);
    config.SetReadConSize(OperationLogConst::CONNECT_SIZE);
    config.SetWalLimitSize(OperationLogConst::RDB_WAL_LIMIT_SIZE);

    if (!TryOpenRdbStore(databaseDir, databasePath, ret, isAnon)) {
        LOGE("open operation log rdb store failed, isAnon = %{public}d, errCode = %{public}d", isAnon, ret);
        return ret;
    }
    LOGI("OperationLogStore RdbInit success, isAnon = %{public}d, path = %{public}s",
        isAnon, GetAnonyString(databasePath).c_str());
    return E_OK;
}

int32_t OperationLogStore::CheckAndCreateDir(const string& dirPath)
{
    struct stat dirStat;
    if (lstat(dirPath.c_str(), &dirStat) == 0) {
        if (S_ISLNK(dirStat.st_mode)) {
            LOGE("path is a symlink, reject fro security, dir = %{public}s", dirPath.c_str());
            return E_PATH;
        }
        return E_OK;
    }
    if (errno != ENOENT) {
        LOGE("lstat failed, dir = %{public}s, errno = %{public}d", dirPath.c_str(), errno);
        return E_PATH;
    }
    OHOS::Storage::DistributedFile::Utils::ForceCreateDirectory(dirPath, OperationLogConst::DIR_MODE);
    if (access(dirPath.c_str(), F_OK) != 0) {
        LOGE("create log db dir failed, dir = %{public}s, errno = %{public}d", dirPath.c_str(), errno);
        return E_PATH;
    }
    if (chown(dirPath.c_str(), OID_DFS, OID_DFS) != 0) {
        LOGE("chmod failed, dir = %{public}s, errno = %{public}d", dirPath.c_str(), errno);
        return E_PATH;
    }
    return E_OK;
}

bool OperationLogStore::TryOpenRdbStore(const string& customDir, const string& databasePath, int32_t& errCode,
    bool isAnon)
{
    auto& rdbStore = isAnon ? anonRdbStore_ : rdbStore_;
    auto& config = isAnon ? anonConfig_ : config_;
    OperationLogRdbCallBack callBack(isAnon);
    rdbStore = RdbHelper::GetRdbStore(config, OperationLogConst::DATABASE_VERSION, callBack, errCode);
    if (rdbStore != nullptr) {
        return true;
    }
    LOGE("get rdb store failed, errCode = %{public}d", errCode);
    if (errCode == NativeRdb::E_SQLITE_CANTOPEN || errCode == NativeRdb::E_SQLITE_PERM) {
        struct stat dirStat;
        if (lstat(customDir.c_str(), &dirStat) == 0 &&
            dirStat.st_uid == static_cast<uid_t>(OID_DFS) &&
            dirStat.st_gid == static_cast<gid_t>(OID_DFS)) {
            if (chmod(customDir.c_str(), OperationLogConst::DIR_MODE) == 0) {
                LOGE("recreate operation log db dir failed");
                rdbStore = RdbHelper::GetRdbStore(config, OperationLogConst::DATABASE_VERSION, callBack, errCode);
            }
        }
    }
    return rdbStore != nullptr;
}

shared_ptr<RdbStore> OperationLogStore::GetRaw()
{
    lock_guard<mutex> lock(mutex_);
    return rdbStore_;
}

shared_ptr<RdbStore> OperationLogStore::GetAnonRaw()
{
    lock_guard<mutex> lock(mutex_);
    return anonRdbStore_;
}

void OperationLogStore::Stop()
{
    lock_guard<mutex> lock(mutex_);
    rdbStore_ = nullptr;
    anonRdbStore_ = nullptr;
    isInited_ = false;
    anonIsInited_ = false;
}

OperationLogRdbCallBack::OperationLogRdbCallBack(bool isAnon) : isAnon_(isAnon) {}

int32_t OperationLogRdbCallBack::OnCreate(RdbStore& store)
{
    const string& tableSql = isAnon_ ? OperationLogColumn::CREATE_ANON_TABLE_SQL :
        OperationLogColumn::CREATE_TABLE_SQL;
    const string& indexSql = isAnon_ ? OperationLogColumn::CREATE_ANON_INDEX_SQL :
        OperationLogColumn::CREATE_INDEX_SQL;
    int32_t ret = store.ExecuteSql(tableSql);
    if (ret != NativeRdb::E_OK) {
        LOGE("create %s operation log table failed, ret = %{public}d", isAnon_ ? "anon" : "legacy", ret);
        return NativeRdb::E_ERROR;
    }

    ret = store.ExecuteSql(indexSql);
    if (ret != NativeRdb::E_OK) {
        LOGE("create %s operation log index failed, ret = %{public}d", isAnon_ ? "anon" : "legacy", ret);
        return NativeRdb::E_ERROR;
    }
    LOGI("create %s operation log table success", isAnon_ ? "anon" : "legacy");
    return NativeRdb::E_OK;
}

int32_t OperationLogRdbCallBack::OnUpgrade(RdbStore& store, int32_t oldVersion, int32_t newVersion)
{
    LOGI("operation log db upgrade, oldVersion = %{public}d, newVersion = %{public}d", oldVersion, newVersion);
    if (isAnon_) {
        return NativeRdb::E_OK;
    }
    int32_t ret = store.ExecuteSql("DROP TABLE IF EXISTS " + OperationLogColumn::TABLE_NAME);
    if (ret != NativeRdb::E_OK) {
        LOGE("drop operation log table failed, ret = %{public}d", ret);
        return NativeRdb::E_ERROR;
    }
    ret = store.ExecuteSql(OperationLogColumn::CREATE_TABLE_SQL);
    if (ret != NativeRdb::E_OK) {
        LOGE("recreate operation log table failed, ret = %{public}d", ret);
        return NativeRdb::E_ERROR;
    }
    ret = store.ExecuteSql(OperationLogColumn::CREATE_INDEX_SQL);
    if (ret != NativeRdb::E_OK) {
        LOGE("recreate operation log index failed, ret = %{public}d", ret);
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

} // namespace CloudDisk
} // namespace FileManagement
} // namespace OHOS
