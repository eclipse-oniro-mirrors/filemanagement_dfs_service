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
#ifndef OHOS_CLOUD_SYNC_SERVICE_DATA_SYNC_CONST_H
#define OHOS_CLOUD_SYNC_SERVICE_DATA_SYNC_CONST_H

#include <set>
#include <string>
namespace OHOS::FileManagement::CloudSync {

enum CleanAction {
    RETAIN_DATA = 0,
    CLEAR_DATA
};

enum AlbumSource {
    ALBUM_FROM_LOCAL = 1,
    ALBUM_FROM_CLOUD = 2
};

enum class SyncTriggerType : int32_t {
    APP_TRIGGER,
    CLOUD_TRIGGER,
    PENDING_TRIGGER,
    BATTERY_OK_TRIGGER,
    NETWORK_AVAIL_TRIGGER,
    TASK_TRIGGER,
    SYSTEM_LOAD_TRIGGER,
    POWER_CONNECT_TRIGGER,
    SCREEN_OFF_TRIGGER,
};

enum class ThumbState : int32_t {
    DOWNLOADED,
    LCD_TO_DOWNLOAD,
    THM_TO_DOWNLOAD,
    TO_DOWNLOAD,
};

struct LocalInfo {
    std::string parentCloudId;
    std::string fileName;
    int64_t mdirtyTime;
    int64_t fdirtyTime;
    int64_t recycledTime;
    int64_t rowId;
    int32_t thm_flag;
    int32_t lcd_flag;
    int32_t dirtyType;
};

struct BundleNameUserInfo {
    std::string bundleName;
    int32_t userId;
    int32_t pid;
};

const int32_t POSITION_LOCAL = 1;
const int32_t POSITION_CLOUD = 2;
const int32_t POSITION_BOTH = 3;

const int32_t DOWNLOADED_THM = 0;
const int32_t TO_DOWNLOAD_THM = 1;
const int32_t NO_THM_TO_DOWNLOAD = 2;
const int32_t THM_DENTRY_INSERTED = 3;

const int32_t FILE = 0;
const int32_t DIRECTORY = 1;

const int32_t MILLISECOND_TO_SECOND = 1000;
const int32_t SECOND_TO_MILLISECOND = 1000;
const int64_t MILLISECOND_TO_NANOSECOND = 1e6;
const uint64_t TWELVE_HOURS_MILLISECOND = 12 * 60 * 60 * SECOND_TO_MILLISECOND;

const int32_t NOT_IN_TRASH = 0;
const int32_t NOT_IN_PENDING = 0;
const int32_t NOT_HIDDEN = 0;
const int32_t NOT_TEMP_FILE = 0;
const int32_t IS_BURST_COVER = 1;

const std::string HDC_BUNDLE_NAME = "com.ohos.ailife";
const std::string GALLERY_BUNDLE_NAME = "com.ohos.photos";
const std::string MEDIALIBRARY_BUNDLE_NAME = "com.ohos.medialibrary.medialibrarydata";

static const std::string CLOUDSYNC_STATUS_KEY = "persist.kernel.cloudsync.status";
static const std::string CLOUDSYNC_STATUS_LOGOUT = "0";
static const std::string CLOUDSYNC_STATUS_START = "1";
static const std::string CLOUDSYNC_STATUS_INC_SYNC = "2";
static const std::string CLOUDSYNC_STATUS_FULL_SYNC = "3";
static const std::string CLOUDSYNC_STATUS_FINISH = "4";
static const std::string CLOUDSYNC_STATUS_SWITCHOFF = "5";
static const std::string CLOUDSYNC_STATUS_CLEANING = "6";

static const std::string CLOUDSYNC_SWITCH_STATUS = "persist.kernel.cloudsync.switch_status";

static inline uint64_t GetCurrentTimeStamp()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec * MILLISECOND_TO_NANOSECOND + t.tv_nsec / SECOND_TO_MILLISECOND;
}

static inline uint64_t GetCurrentTimeStampMs()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec * SECOND_TO_MILLISECOND + t.tv_nsec / MILLISECOND_TO_NANOSECOND;
}
} // namespace OHOS::FileManagement::CloudSync
#endif // OHOS_CLOUD_SYNC_SERVICE_DATA_SYNC_CONST_H
