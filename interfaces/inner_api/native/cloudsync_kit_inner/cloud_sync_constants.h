/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_FILEMGMT_CLOUD_SYNC_CONSTANTS_H
#define OHOS_FILEMGMT_CLOUD_SYNC_CONSTANTS_H

#include <unordered_map>

namespace OHOS::FileManagement::CloudSync {
enum class SyncType : int32_t {
    UPLOAD = 0,
    DOWNLOAD,
    ALL,
};

enum class SyncPromptState : int32_t {
    SYNC_STATE_DEFAULT = 0,
    SYNC_STATE_SYNCING,
    SYNC_STATE_FAILED,
    SYNC_STATE_PAUSED_NO_NETWORK,
    SYNC_STATE_PAUSED_FOR_WIFI,
    SYNC_STATE_PAUSED_FOR_BATTERY,
    SYNC_STATE_BATTERY_TOO_LOW,
    SYNC_STATE_PAUSED_FOR_SPACE_TOO_LOW,
};

enum CloudSyncState {
    UPLOADING = 0,
    UPLOAD_FAILED,
    DOWNLOADING,
    DOWNLOAD_FAILED,
    COMPLETED,
    STOPPED,
};

enum OptimizeState {
    OPTIMIZE_RUNNING = 0,
    OPTIMIZE_COMPLETED = 1,
    OPTIMIZE_FAILED = 2,
    OPTIMIZE_STOPPED = 3,
};

enum ErrorType {
    NO_ERROR = 0,
    NETWORK_UNAVAILABLE,
    WIFI_UNAVAILABLE,
    BATTERY_LEVEL_LOW,
    BATTERY_LEVEL_WARNING,
    CLOUD_STORAGE_FULL,
    LOCAL_STORAGE_FULL,
    DEVICE_TEMPERATURE_TOO_HIGH,
    BUSINESS_MODE_CHANGED,
    PERMISSION_NOT_ALLOW,
    INNER_ERROR,
};

enum State {
    CLOUD_FILE_DOWNLOAD_RUNNING = 0,
    CLOUD_FILE_DOWNLOAD_COMPLETED,
    CLOUD_FILE_DOWNLOAD_FAILED,
    CLOUD_FILE_DOWNLOAD_STOPPED,
};

enum FileSyncState {
    FILESYNCSTATE_UPLOADING = 0,
    FILESYNCSTATE_DOWNLOADING,
    FILESYNCSTATE_COMPLETED,
    FILESYNCSTATE_STOPPED,
    FILESYNCSTATE_TO_BE_UPLOADED,
    FILESYNCSTATE_UPLOAD_SUCCESS,
    FILESYNCSTATE_UPLOAD_FAILURE,
};

enum FileSync {
    FILESYNC_TO_BE_UPLOADED = 0,
    FILESYNC_UPLOADING,
    FILESYNC_UPLOAD_FAILURE,
    FILESYNC_UPLOAD_SUCCESS,
    FILESYNC_INITIAL_AFTER_DOWNLOAD,
    FILESYNC_STOPPED,
};

enum FileState {
    FILESTATE_INITIAL_AFTER_DOWNLOAD = 0,
    FILESTATE_UPLOADING,
    FILESTATE_STOPPED,
    FILESTATE_TO_BE_UPLOADED,
    FILESTATE_UPLOAD_SUCCESS,
    FILESTATE_UPLOAD_FAILURE,
};

static inline std::unordered_map<int, int> statusMap {
    {FileSync::FILESYNC_TO_BE_UPLOADED, FileSyncState::FILESYNCSTATE_TO_BE_UPLOADED},
    {FileSync::FILESYNC_UPLOADING, FileSyncState::FILESYNCSTATE_UPLOADING},
    {FileSync::FILESYNC_UPLOAD_SUCCESS, FileSyncState::FILESYNCSTATE_UPLOAD_SUCCESS},
    {FileSync::FILESYNC_UPLOAD_FAILURE, FileSyncState::FILESYNCSTATE_UPLOAD_FAILURE},
};

static inline std::unordered_map<int, int> publicStatusMap {
    {FileSync::FILESYNC_TO_BE_UPLOADED, FileState::FILESTATE_TO_BE_UPLOADED},
    {FileSync::FILESYNC_UPLOADING, FileState::FILESTATE_UPLOADING},
    {FileSync::FILESYNC_UPLOAD_SUCCESS, FileState::FILESTATE_UPLOAD_SUCCESS},
    {FileSync::FILESYNC_UPLOAD_FAILURE, FileState::FILESTATE_UPLOAD_FAILURE},
    {FileSync::FILESYNC_INITIAL_AFTER_DOWNLOAD, FileState::FILESTATE_INITIAL_AFTER_DOWNLOAD},
    {FileSync::FILESYNC_STOPPED, FileState::FILESTATE_STOPPED},
};

enum NotifyType {
    NOTIFY_ADDED = 0,
    NOTIFY_MODIFIED,
    NOTIFY_DELETED,
    NOTIFY_RENAMED,
    NOTIFY_NONE,
};
} // namespace OHOS::FileManagement::CloudSync

#endif // OHOS_FILEMGMT_CLOUD_SYNC_CONSTANTS_H