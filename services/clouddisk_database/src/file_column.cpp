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

#include "file_column.h"

namespace OHOS {
namespace FileManagement {
namespace CloudDisk {
const std::string FileColumn::CLOUD_ID = "cloud_id";
const std::string FileColumn::IS_DIRECTORY = "isDirectory";
const std::string FileColumn::FILE_NAME = "file_name";
const std::string FileColumn::PARENT_CLOUD_ID = "parent_cloud_id";
const std::string FileColumn::FILE_SIZE = "file_size";
const std::string FileColumn::FILE_SHA256 = "sha256";
const std::string FileColumn::FILE_TIME_ADDED = "time_added";
const std::string FileColumn::FILE_TIME_EDITED = "time_edited";
const std::string FileColumn::FILE_TIME_RECYCLED = "time_recycled";
const std::string FileColumn::META_TIME_EDITED = "meta_time_edited";
const std::string FileColumn::FILE_TIME_VISIT = "time_visit";
const std::string FileColumn::DIRECTLY_RECYCLED = "directly_recycled";
const std::string FileColumn::VERSION = "version";
const std::string FileColumn::OPERATE_TYPE = "operateType";
const std::string FileColumn::SYNC_STATUS = "sync_status";
const std::string FileColumn::POSITION = "position";
const std::string FileColumn::THM_FLAG = "thm_flag";
const std::string FileColumn::LCD_FLAG = "lcd_flag";
const std::string FileColumn::NO_NEED_UPLOAD = "no_need_upload";
const std::string FileColumn::DIRTY_TYPE = "dirty_type";
const std::string FileColumn::MIME_TYPE = "mimetype";
const std::string FileColumn::FILE_TYPE = "file_type";
const std::string FileColumn::FILE_CATEGORY = "file_category";
const std::string FileColumn::IS_FAVORITE = "isFavorite";
const std::string FileColumn::FILE_STATUS = "file_status";
const std::string FileColumn::ROW_ID = "rowid";
const std::string FileColumn::CHECK_FLAG = "check_flag";
const std::string FileColumn::ROOT_DIRECTORY = "root_directory";
const std::string FileColumn::ATTRIBUTE = "attribute";
const std::string FileColumn::FILES_TABLE = "CloudDisk";
const std::string FileColumn::PARENT_CLOUD_ID_INDEX = "parentCloudId_index";
const std::string FileColumn::SRC_CLOUD_ID = "src_cloud_id";
const std::string FileColumn::THM_SIZE = "thm_size";
const std::string FileColumn::LCD_SIZE = "lcd_size";
const std::string FileColumn::SOURCE_CLOUD_ID = "source_cloud_id";
const std::string FileColumn::LOCAL_FLAG = "local_flag";

//  description for LOCAL_FLAG shift
const uint64_t FileColumn::LOCAL_FLAG_MASK_ISCONFLICT = (1 << 0);

const std::string FileColumn::CREATE_FILE_TABLE = "CREATE TABLE IF NOT EXISTS " +
    FILES_TABLE + " (" +
    CLOUD_ID + " TEXT PRIMARY KEY NOT NULL, " +
    IS_DIRECTORY + " INT, " +
    FILE_NAME + " TEXT NOT NULL, " +
    PARENT_CLOUD_ID + " TEXT NOT NULL, " +
    FILE_SIZE + " BIGINT, " +
    FILE_SHA256 + " TEXT, " +
    FILE_TIME_ADDED + " BIGINT, " +
    FILE_TIME_EDITED + " BIGINT, " +
    FILE_TIME_RECYCLED + " BIGINT DEFAULT 0, " +
    META_TIME_EDITED + " BIGINT, " +
    FILE_TIME_VISIT + " BIGINT DEFAULT 0, " +
    DIRECTLY_RECYCLED + " INT DEFAULT 0, " +
    VERSION + " BIGINT DEFAULT 0, " +
    OPERATE_TYPE + " BIGINT DEFAULT 0, " +
    SYNC_STATUS + " INT DEFAULT 0, " +
    POSITION + " INT DEFAULT 1, " +
    DIRTY_TYPE + " INT DEFAULT 1, " +
    MIME_TYPE + " TEXT, " +
    FILE_TYPE + " INT, " +
    FILE_CATEGORY + " TEXT, " +
    IS_FAVORITE + " INT DEFAULT 0, " +
    FILE_STATUS + " INT DEFAULT 4, " +
    CHECK_FLAG + " INT DEFAULT 0, " +
    ROOT_DIRECTORY + " TEXT, " +
    ATTRIBUTE + " TEXT, " +
    THM_FLAG + " INT DEFAULT 0, " +
    LCD_FLAG + " INT DEFAULT 0, " +
    NO_NEED_UPLOAD + " INT DEFAULT 0, " +
    SRC_CLOUD_ID + " TEXT, " +
    THM_SIZE + " INT DEFAULT 0, " +
    LCD_SIZE + " INT DEFAULT 0, " +
    SOURCE_CLOUD_ID + " TEXT, " +
    LOCAL_FLAG + " BIGINT DEFAULT 0)";

const std::string FileColumn::CREATE_PARENT_CLOUD_ID_INDEX = "CREATE INDEX IF NOT EXISTS " +
    PARENT_CLOUD_ID_INDEX + " ON " + FILES_TABLE +
    " (" + PARENT_CLOUD_ID + ")";

const std::string FileColumn::ADD_LOCAL_FLAG = "ALTER Table " + FILES_TABLE +
    " ADD COLUMN " + LOCAL_FLAG + " BIGINT DEFAULT 0";

const std::string FileColumn::ADD_IS_FAVORITE = "ALTER Table " + FILES_TABLE +
    " ADD COLUMN " + IS_FAVORITE + " INT DEFAULT 0";

const std::string FileColumn::ADD_FILE_STATUS = "ALTER Table " + FILES_TABLE +
    " ADD COLUMN " + FILE_STATUS + " INT";

const std::string FileColumn::SET_FILE_STATUS_DEFAULT = "UPDATE " + FILES_TABLE +
    " SET " + FILE_STATUS + " = 4 WHERE " + FILE_STATUS + " IS NULL";

const std::string FileColumn::ADD_CHECK_FLAG = "ALTER Table " + FILES_TABLE +
    " ADD COLUMN " + CHECK_FLAG + " INT DEFAULT 0";

const std::string FileColumn::ADD_ATTRIBUTE = "ALTER Table " + FILES_TABLE +
    " ADD COLUMN " + ATTRIBUTE + " TEXT";

const std::string FileColumn::ADD_THM_FLAG = "ALTER Table " + FILES_TABLE +
    " ADD COLUMN " + THM_FLAG + " INT DEFAULT 0";

const std::string FileColumn::ADD_LCD_FLAG = "ALTER Table " + FILES_TABLE +
    " ADD COLUMN " + LCD_FLAG + " INT DEFAULT 0";

const std::string FileColumn::ADD_UPLOAD_FLAG = "ALTER Table " + FILES_TABLE +
    " ADD COLUMN " + NO_NEED_UPLOAD + " INT DEFAULT 0";

const std::string FileColumn::ADD_SRC_CLOUD_ID = "ALTER Table " + FILES_TABLE +
    " ADD COLUMN " + SRC_CLOUD_ID + " TEXT";

const std::string FileColumn::ADD_THM_SIZE = "ALTER Table " + FILES_TABLE +
    " ADD COLUMN " + THM_SIZE + " INT DEFAULT 0";

const std::string FileColumn::ADD_LCD_SIZE = "ALTER Table " + FILES_TABLE +
    " ADD COLUMN " + LCD_SIZE + " INT DEFAULT 0";

    const std::string FileColumn::ADD_SOURCE_CLOUD_ID = "ALTER Table " + FILES_TABLE +
    " ADD COLUMN " + SOURCE_CLOUD_ID + " TEXT";

const std::vector<std::string> FileColumn::FILE_SYSTEM_QUERY_COLUMNS = {
    FILE_NAME,
    CLOUD_ID,
    PARENT_CLOUD_ID,
    POSITION,
    FILE_SIZE,
    META_TIME_EDITED,
    FILE_TIME_ADDED,
    FILE_TIME_EDITED,
    IS_DIRECTORY,
    ROW_ID
};

const std::vector<std::string> FileColumn::DISK_CLOUD_SYNC_COLUMNS = {
    CLOUD_ID,
    IS_DIRECTORY,
    FILE_NAME,
    PARENT_CLOUD_ID,
    FILE_SIZE,
    FILE_SHA256,
    FILE_TIME_ADDED,
    FILE_TIME_EDITED,
    FILE_TIME_RECYCLED,
    META_TIME_EDITED,
    DIRECTLY_RECYCLED,
    VERSION,
    OPERATE_TYPE,
    ROOT_DIRECTORY,
    ATTRIBUTE,
    THM_FLAG,
    LCD_FLAG,
    ROW_ID,
    THM_SIZE,
    LCD_SIZE,
    LOCAL_FLAG
};

//File copy requires all fields in the database, and one is added here when adding a new column ...
const std::vector<std::string> FileColumn::DISK_CLOUD_FOR_COPY = {
    CLOUD_ID,
    IS_DIRECTORY,
    FILE_NAME,
    PARENT_CLOUD_ID,
    FILE_SIZE,
    FILE_SHA256,
    FILE_TIME_ADDED,
    FILE_TIME_EDITED,
    FILE_TIME_RECYCLED,
    META_TIME_EDITED,
    FILE_TIME_VISIT,
    DIRECTLY_RECYCLED,
    VERSION,
    OPERATE_TYPE,
    SYNC_STATUS,
    POSITION,
    DIRTY_TYPE,
    MIME_TYPE,
    FILE_TYPE,
    FILE_CATEGORY,
    IS_FAVORITE,
    FILE_STATUS,
    CHECK_FLAG,
    ROOT_DIRECTORY,
    ATTRIBUTE,
    THM_FLAG,
    LCD_FLAG,
    NO_NEED_UPLOAD,
    SRC_CLOUD_ID,
};

const std::vector<std::string> FileColumn::LOCAL_COLUMNS = {
    FILE_TIME_VISIT, SYNC_STATUS, POSITION, DIRTY_TYPE,
    MIME_TYPE, IS_DIRECTORY, FILE_TYPE, FILE_CATEGORY,
};

const std::vector<std::string> FileColumn::PULL_QUERY_COLUMNS = {
    CLOUD_ID, FILE_TIME_RECYCLED, VERSION, DIRTY_TYPE, POSITION,
    FILE_TIME_EDITED, FILE_SHA256, FILE_SIZE, FILE_NAME, PARENT_CLOUD_ID, ROW_ID,
    IS_DIRECTORY, FILE_TIME_ADDED, FILE_TYPE, ROOT_DIRECTORY, DIRECTLY_RECYCLED,
    THM_FLAG, LCD_FLAG,
};

const std::vector<std::string> FileColumn::DISK_ON_UPLOAD_COLUMNS = {
    CLOUD_ID,
    PARENT_CLOUD_ID,
    FILE_NAME,
    FILE_TIME_EDITED,
    META_TIME_EDITED,
    FILE_TIME_RECYCLED,
    ROW_ID,
    THM_FLAG,
    LCD_FLAG,
};

const std::vector<std::string> FileColumn::DISK_CLOUD_FOR_THM_INSERT = {
    THM_SIZE,
    LCD_SIZE,
    THM_FLAG,
    LCD_FLAG,
    CLOUD_ID,
    FILE_TIME_RECYCLED,
    FILE_NAME,
    ROW_ID,
};

const std::vector<std::string> FileColumn::EXT_ATTR_QUERY_COLUMNS = {
    ATTRIBUTE,
    POSITION
};
} // namespace CloudDisk
} // namespace FileManagement
} // namespace OHOS
