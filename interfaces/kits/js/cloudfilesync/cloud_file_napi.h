/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_FILEMGMT_CLOUD_FILE_NAPI_H
#define OHOS_FILEMGMT_CLOUD_FILE_NAPI_H

#include <memory>

#include "cloud_download_callback.h"
#include "download_callback_middle_napi.h"
#include "download_progress_napi.h"
#include "filemgmt_libn.h"

namespace OHOS::FileManagement::CloudSync {
class CloudDownloadCallbackImpl;
class CloudFileNapi : public LibN::NExporter {
public:
    CloudFileNapi(napi_env env, napi_value exports) : NExporter(env, exports) {}
    ~CloudFileNapi() = default;

    bool Export() override;
    bool ToExport(std::vector<napi_property_descriptor> props);
    virtual void SetClassName(std::string classname);
    std::string GetClassName() override;
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static napi_value Start(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);
    static napi_value Off(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);

private:
    inline static std::mutex mtx_;
    inline static bool isDownloadCallbackRegistered_;
    static inline std::shared_ptr<CloudDownloadCallbackImpl> callback_;
    inline static std::string className_ = "CloudFileNapi";
};

class CloudDownloadCallbackImpl : public CloudDownloadCallback,
                                  public std::enable_shared_from_this<CloudDownloadCallbackImpl> {
public:
    CloudDownloadCallbackImpl(napi_env env, napi_value fun, bool isBatch = false);
    ~CloudDownloadCallbackImpl() = default;
    void OnDownloadProcess(const DownloadProgressObj &progress) override;
    void DeleteReference();

    class UvChangeMsg {
    public:
        UvChangeMsg(std::shared_ptr<CloudDownloadCallbackImpl> CloudDownloadCallbackIn,
                    DownloadProgressObj downloadProgress,
                    bool isBatch)
            : CloudDownloadCallback_(CloudDownloadCallbackIn), downloadProgress_(downloadProgress), isBatch_(isBatch)
        {
        }
        ~UvChangeMsg() {}
        std::weak_ptr<CloudDownloadCallbackImpl> CloudDownloadCallback_;
        DownloadProgressObj downloadProgress_;
        bool isBatch_;
    };

private:
    static void OnComplete(UvChangeMsg *msg);
    napi_env env_;
    napi_ref cbOnRef_ = nullptr;
    bool isBatch_;
};

class CloudDownloadCallbackImplNapi : public CloudDlCallbackMiddleNapi {
public:
    explicit CloudDownloadCallbackImplNapi(napi_env env) : CloudDlCallbackMiddleNapi(env) {}
    int32_t StartDownloadInner(const std::string &uri);
    int32_t StopDownloadInner(const std::string &uri);
};
} // namespace OHOS::FileManagement::CloudSync
#endif // OHOS_FILEMGMT_CLOUD_FILE_NAPI_H