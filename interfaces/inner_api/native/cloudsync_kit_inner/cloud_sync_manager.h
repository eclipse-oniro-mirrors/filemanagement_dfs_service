/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_FILEMGMT_CLOUD_SYNC_MANAGER_H
#define OHOS_FILEMGMT_CLOUD_SYNC_MANAGER_H

#include <memory>

#include "cloud_download_callback.h"
#include "cloud_optimize_callback.h"
#include "cloud_sync_callback.h"
#include "cloud_sync_common.h"
#include "downgrade_dl_callback.h"

namespace OHOS::FileManagement::CloudSync {
class CloudSyncManager {
public:
    static CloudSyncManager &GetInstance();
    /**
     * @brief 注册
     *
     * @param callback 注册同步回调
     * @return int32_t 同步返回执行结果
     */
    virtual int32_t RegisterCallback(const std::shared_ptr<CloudSyncCallback> callback,
                                     const std::string &bundleName = "") = 0;
    virtual int32_t RegisterFileSyncCallback(const std::shared_ptr<CloudSyncCallback> callback,
                                     const std::string &bundleName = "") = 0;
    /**
     * @brief 注册
     *
     * @param callback 卸载同步回调
     * @return int32_t 返回执行结果
     */
    virtual int32_t UnRegisterCallback(const std::string &bundleName = "") = 0;
    virtual int32_t UnRegisterFileSyncCallback(const std::string &bundleName = "") = 0;
    /**
     * @brief 启动同步
     *
     * @return int32_t 同步返回执行结果
     */
    virtual int32_t StartSync(const std::string &bundleName = "") = 0;
    virtual int32_t StartFileSync(const std::string &bundleName = "") = 0;
    /**
     * @brief 启动同步
     *
     * @param forceFlag 是否强制继续同步，当前仅支持低电量下暂停后继续上传
     * @param callback 注册同步回调
     * @return int32_t 同步返回执行结果
     */
    virtual int32_t StartSync(bool forceFlag, const std::shared_ptr<CloudSyncCallback> callback) = 0;
    /**
     * @brief 触发同步
     *
     * @param bundleName 应用包名
     * @param userId 用户ID
     * @return int32_t 同步返回执行结果
     */
    virtual int32_t TriggerSync(const std::string &bundleName, const int32_t &userId) = 0;
    /**
     * @brief 停止同步
     *
     * @return int32_t 同步返回执行结果
     */
    virtual int32_t StopSync(const std::string &bundleName = "", bool forceFlag = false) = 0;
    virtual int32_t StopFileSync(const std::string &bundleName = "", bool forceFlag = false) = 0;
    /**
     * @brief 清理水位
     *
     * @return int32_t 同步返回执行结果
     */
    virtual int32_t ResetCursor(const std::string &bundleName = "") = 0;
    /**
     * @brief 切换应用云同步开关
     *
     * @param accoutId 账号ID
     * @param bundleName 应用包名
     * @param status 应用开关状态
     * @return int32_t 同步返回执行结果
     */
    virtual int32_t ChangeAppSwitch(const std::string &accoutId, const std::string &bundleName, bool status) = 0;
    /**
     * @brief 云数据变化触发同步
     *
     * @param accoutId 账号ID
     * @param bundleName 应用包名
     * @return int32_t 同步返回执行结果
     */
    virtual int32_t NotifyDataChange(const std::string &accoutId, const std::string &bundleName) = 0;
    virtual int32_t NotifyEventChange(int32_t userId, const std::string &eventId, const std::string &extraData) = 0;
    virtual int32_t EnableCloud(const std::string &accoutId, const SwitchDataObj &switchData) = 0;
    virtual int32_t DisableCloud(const std::string &accoutId) = 0;
    /**
     * @brief 云账号退出选择保留或清除云数据
     *
     * @param accoutId 账号ID
     * @param cleanOptions 说明云文件如何处理的方式
     * @return int32_t 同步返回执行结果
     */
    virtual int32_t Clean(const std::string &accountId, const CleanOptions &cleanOptions) = 0;
    virtual int32_t OptimizeStorage(const OptimizeSpaceOptions &optimizeOptions,
        const std::shared_ptr<CloudOptimizeCallback> optimizeCallback = nullptr) = 0;
    virtual int32_t StopOptimizeStorage() = 0;
    virtual int32_t StartDownloadFile(const std::string &path) = 0;
    virtual int32_t StartFileCache(const std::string &path) = 0;
    virtual int32_t StartFileCache(const std::vector<std::string> &pathVec, int64_t &downloadId,
                                   int32_t fieldkey = FIELDKEY_CONTENT,
                                   const std::shared_ptr<CloudDownloadCallback> downloadCallback = nullptr,
                                   int32_t timeout = -1) = 0;
    virtual int32_t StopDownloadFile(const std::string &path, bool needClean = false) = 0;
    virtual int32_t StopFileCache(int64_t downloadId, bool needClean = false, int32_t timeout = -1) = 0;
    virtual int32_t DownloadThumb() = 0;
    virtual int32_t RegisterDownloadFileCallback(const std::shared_ptr<CloudDownloadCallback> downloadCallback) = 0;
    virtual int32_t RegisterFileCacheCallback(const std::shared_ptr<CloudDownloadCallback> downloadCallback) = 0;
    virtual int32_t UnregisterDownloadFileCallback() = 0;
    virtual int32_t UnregisterFileCacheCallback() = 0;
    virtual int32_t GetSyncTime(int64_t &syncTime, const std::string &bundleName = "") = 0;
    virtual int32_t CleanCache(const std::string &uri) = 0;
    virtual void CleanGalleryDentryFile() = 0;
    virtual void CleanGalleryDentryFile(const std::string path) = 0;
    virtual int32_t BatchCleanFile(const std::vector<CleanFileInfo> &fileInfo,
        std::vector<std::string> &failCloudId) = 0;
    virtual int32_t BatchDentryFileInsert(const std::vector<DentryFileInfo> &fileInfo,
         std::vector<std::string> &failCloudId) = 0;
    /**
     * @brief 开始降级下载
     *
     * @param bundleName 降级应用包名
     * @param downloadCallback 下载进度回调函数
     * @return int32_t 同步返回执行结果
     */
    virtual int32_t StartDowngrade(const std::string &bundleName,
                                   const std::shared_ptr<DowngradeDlCallback> downloadCallback) = 0;
    /**
     * @brief 停止降级下载
     *
     * @param bundleName 降级应用包名
     * @return int32_t 同步返回执行结果
     */
    virtual int32_t StopDowngrade(const std::string &bundleName) = 0;
    /**
     * @brief 获取应用文件信息
     *
     * @param bundleName 应用包名
     * @param cloudFileInfo 不同位置文件总大小信息
     * @return int32_t 同步返回执行结果
     */
    virtual int32_t GetCloudFileInfo(const std::string &bundleName, CloudFileInfo &cloudFileInfo) = 0;
    // file version
    virtual int32_t GetHistoryVersionList(const std::string &uri, const int32_t versionNumLimit,
                                          std::vector<CloudSync::HistoryVersion> &historyVersionList) = 0;
    virtual int32_t DownloadHistoryVersion(const std::string &uri, int64_t &downloadId, const uint64_t versionId,
                                           const std::shared_ptr<CloudDownloadCallback> downloadCallback,
                                           std::string &versionUri) = 0;
    virtual int32_t ReplaceFileWithHistoryVersion(const std::string &uri, const std::string &versionUri) = 0;
    virtual int32_t IsFileConflict(const std::string &uri, bool &isConflict) = 0;
    virtual int32_t ClearFileConflict(const std::string &uri) = 0;
};
} // namespace OHOS::FileManagement::CloudSync

#endif // OHOS_FILEMGMT_CLOUD_SYNC_MANAGER_H
