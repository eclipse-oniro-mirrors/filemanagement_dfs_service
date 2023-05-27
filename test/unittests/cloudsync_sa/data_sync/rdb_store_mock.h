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

#ifndef OHOS_CLOUD_SYNC_SA_RDB_STORE_MOCK_H
#define OHOS_CLOUD_SYNC_SA_RDB_STORE_MOCK_H

#include "abs_rdb_predicates.h"
#include "rdb_helper.h"
#include "result_set.h"
#include "values_bucket.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace OHOS::FileManagement::CloudSync::Test {
using namespace testing;
using namespace testing::ext;
using namespace std;
using namespace NativeRdb;
class RdbStoreMock final : public NativeRdb::RdbStore {
public:
    MOCK_METHOD3(Insert, int(int64_t &outRowId, const std::string &table, const ValuesBucket &initialValues));
    MOCK_METHOD3(BatchInsert,
                 int(int64_t &outInsertNum,
                     const std::string &table,
                     const std::vector<ValuesBucket> &initialBatchValues));
    MOCK_METHOD3(Replace, int(int64_t &outRowId, const std::string &table, const ValuesBucket &initialValues));
    MOCK_METHOD4(InsertWithConflictResolution,
                 int(int64_t &outRowId,
                     const std::string &table,
                     const ValuesBucket &initialValues,
                     ConflictResolution conflictResolution));
    MOCK_METHOD5(Update,
                 int(int &changedRows,
                     const std::string &table,
                     const ValuesBucket &values,
                     const std::string &whereClause,
                     const std::vector<std::string> &whereArgs));

    MOCK_METHOD6(UpdateWithConflictResolution,
                 int(int &changedRows,
                     const std::string &table,
                     const ValuesBucket &values,
                     const std::string &whereClause,
                     const std::vector<std::string> &whereArgs,
                     ConflictResolution conflictResolution));

    MOCK_METHOD4(Delete,
                 int(int &deletedRows,
                     const std::string &table,
                     const std::string &whereClause,
                     const std::vector<std::string> &whereArgs));

    MOCK_METHOD2(ExecuteSql, int(const std::string &sql, const std::vector<ValueObject> &bindArgs));

    MOCK_METHOD3(ExecuteAndGetLong,
                 int(int64_t &outValue, const std::string &sql, const std::vector<ValueObject> &bindArgs));

    MOCK_METHOD3(ExecuteAndGetString,
                 int(std::string &outValue, const std::string &sql, const std::vector<ValueObject> &bindArgs));

    MOCK_METHOD3(ExecuteForLastInsertedRowId,
                 int(int64_t &outValue, const std::string &sql, const std::vector<ValueObject> &bindArgs));

    MOCK_METHOD3(ExecuteForChangedRowCount,
                 int(int64_t &outValue, const std::string &sql, const std::vector<ValueObject> &bindArgs));

    MOCK_METHOD2(Backup, int(const std::string databasePath, const std::vector<uint8_t> destEncryptKey));

    MOCK_METHOD3(Attach,
                 int(const std::string &alias, const std::string &pathName, const std::vector<uint8_t> destEncryptKey));

    MOCK_METHOD2(Count, int(int64_t &outValue, const AbsRdbPredicates &predicates));
    MOCK_METHOD2(Query,
                 std::unique_ptr<AbsSharedResultSet>(const AbsRdbPredicates &predicates,
                                                     const std::vector<std::string> columns));
    MOCK_METHOD10(Query,
                  std::unique_ptr<AbsSharedResultSet>((int &errCode,
                                                       bool distinct,
                                                       const std::string &table,
                                                       const std::vector<std::string> &columns,
                                                       const std::string &selection,
                                                       const std::vector<std::string> &selectionArgs,
                                                       const std::string &groupBy,
                                                       const std::string &having,
                                                       const std::string &orderBy,
                                                       const std::string &limit)));
    MOCK_METHOD2(QuerySql,
                 std::unique_ptr<AbsSharedResultSet>(const std::string &sql,
                                                     const std::vector<std::string> &selectionArgs));
    MOCK_METHOD2(QueryByStep,
                 std::unique_ptr<ResultSet>(const std::string &sql, const std::vector<std::string> &selectionArgs));
    MOCK_METHOD2(QueryByStep,
                 std::unique_ptr<ResultSet>(const AbsRdbPredicates &predicates,
                                            const std::vector<std::string> columns));

    MOCK_METHOD4(RemoteQuery,
                 std::shared_ptr<ResultSet>(const std::string &device,
                                            const AbsRdbPredicates &predicates,
                                            const std::vector<std::string> &columns,
                                            int &errCode));

    MOCK_METHOD1(SetDistributedTables, int(const std::vector<std::string> &tables));
    MOCK_METHOD3(ObtainDistributedTableName,
                 std::string(const std::string &device, const std::string &table, int &errCode));
    MOCK_METHOD3(Sync, int(const SyncOption &option, const AbsRdbPredicates &predicate, const SyncCallback &callback));
    MOCK_METHOD2(Subscribe, int(const SubscribeOption &option, RdbStoreObserver *observer));
    MOCK_METHOD2(UnSubscribe, int(const SubscribeOption &option, RdbStoreObserver *observer));
    MOCK_METHOD2(DropDeviceData, bool(const std::vector<std::string> &devices, const DropOption &option));
    MOCK_METHOD3(Update, int(int &changedRows, const ValuesBucket &values, const AbsRdbPredicates &predicates));
    MOCK_METHOD2(Delete, int(int &deletedRows, const AbsRdbPredicates &predicates));
    MOCK_METHOD1(GetVersion, int(int &version));
    MOCK_METHOD1(SetVersion, int(int version));
    MOCK_METHOD0(BeginTransaction, int());
    MOCK_METHOD0(RollBack, int());
    MOCK_METHOD0(Commit, int());
    MOCK_METHOD0(IsInTransaction, bool());
    MOCK_METHOD0(GetPath, std::string());
    MOCK_METHOD0(IsHoldingConnection, bool());
    MOCK_CONST_METHOD0(IsOpen, bool());
    MOCK_CONST_METHOD0(IsReadOnly, bool());
    MOCK_CONST_METHOD0(IsMemoryRdb, bool());
    MOCK_METHOD2(Restore, int(const std::string backupPath, const std::vector<uint8_t> &newKey));
    MOCK_METHOD3(ChangeDbFileForRestore,
                 int(const std::string newPath, const std::string backupPath, const std::vector<uint8_t> &newKey));
};
} // namespace OHOS::FileManagement::CloudSync::Test
#endif