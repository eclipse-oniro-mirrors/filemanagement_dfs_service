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

#ifndef OHOS_CLOUD_DISK_SERVICE_RDB_UTILS_H
#define OHOS_CLOUD_DISK_SERVICE_RDB_UTILS_H

#include <vector>

#include "cloud_file_utils.h"
#include "clouddisk_db_const.h"
#include "rdb_store.h"

namespace OHOS::FileManagement::CloudDisk {
class CloudDiskRdbUtils final {
public:
    CloudDiskRdbUtils() = delete;
    ~CloudDiskRdbUtils() = delete;
    static int32_t GetInt(const std::string &key, int32_t &val,
                          const std::shared_ptr<NativeRdb::ResultSet> resultSet);
    static int32_t GetLong(const std::string &key, int64_t &val,
                           const std::shared_ptr<NativeRdb::ResultSet> resultSet);
    static int32_t GetString(const std::string &key, std::string &val,
                             const std::shared_ptr<NativeRdb::ResultSet> resultSet);
    static int32_t FillInfoFileName(CloudDiskFileInfo &info, const std::shared_ptr<NativeRdb::ResultSet> resultSet);
    static int32_t FillInfoCloudId(CloudDiskFileInfo &info, const std::shared_ptr<NativeRdb::ResultSet> resultSet);
    static int32_t FillInfoParentId(CloudDiskFileInfo &info, const std::shared_ptr<NativeRdb::ResultSet> resultSet);
    static int32_t FillInfoLocation(CloudDiskFileInfo &info, const std::shared_ptr<NativeRdb::ResultSet> resultSet);
    static int32_t FillInfoFileSize(CloudDiskFileInfo &info, const std::shared_ptr<NativeRdb::ResultSet> resultSet);
    static int32_t FillInfoFileATime(CloudDiskFileInfo &info, const std::shared_ptr<NativeRdb::ResultSet> resultSet);
    static int32_t FillInfoFileCTime(CloudDiskFileInfo &info, const std::shared_ptr<NativeRdb::ResultSet> resultSet);
    static int32_t FillInfoFileMTime(CloudDiskFileInfo &info, const std::shared_ptr<NativeRdb::ResultSet> resultSet);
    static int32_t FillInfoFileType(CloudDiskFileInfo &info, const std::shared_ptr<NativeRdb::ResultSet> resultSet);
    static int32_t ResultSetToFileInfo(const std::shared_ptr<NativeRdb::ResultSet> resultSet,
        std::vector<CloudDiskFileInfo> &infos);
};

#define RETURN_ON_ERR(ret)    \
    do {    \
        if ((ret) != E_OK) {    \
            return ret;    \
        }    \
    } while (0)
} // namespace OHOS::FileManagement::CloudDisk

#endif // OHOS_CLOUD_DISK_SERVICE_RDB_UTILS_H