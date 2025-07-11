/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_FILEMGMT_DOWNLOAD_CALLBACK_MIDDLE_NAPI_H
#define OHOS_FILEMGMT_DOWNLOAD_CALLBACK_MIDDLE_NAPI_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "cloud_download_callback.h"
#include "download_progress_napi.h"
#include "register_callback_manager_napi.h"

namespace OHOS::FileManagement::CloudSync {
class CloudDlCallbackMiddleNapi : public CloudDownloadCallback,
                                  public RegisterCallbackManagerNapi,
                                  public std::enable_shared_from_this<CloudDlCallbackMiddleNapi> {
public:
    explicit CloudDlCallbackMiddleNapi(napi_env env) : RegisterCallbackManagerNapi(env) {}
    virtual ~CloudDlCallbackMiddleNapi() = default;
    void OnDownloadProcess(const DownloadProgressObj &progress) override;
    void RemoveDownloadInfo(int64_t downloadId);

protected:
    virtual void DownloadProgressInner(std::shared_ptr<DlProgressNapi> progress){};
    std::vector<int64_t> GetDownloadIdsByUri(const std::string &uri);
    std::shared_ptr<DlProgressNapi> GetDownloadInfo(int64_t downloadId);

protected:
    std::mutex downloadInfoMtx_;
    std::unordered_map<int64_t, std::shared_ptr<DlProgressNapi>> downloadInfos_;
};

} // namespace OHOS::FileManagement::CloudSync
#endif // OHOS_FILEMGMT_DOWNLOAD_CALLBACK_MIDDLE_NAPI_H