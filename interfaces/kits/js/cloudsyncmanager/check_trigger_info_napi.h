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

#ifndef OHOS_FILEMGMT_CHECK_TRIGGER_INFO_NAPI_H
#define OHOS_FILEMGMT_CHECK_TRIGGER_INFO_NAPI_H

#include <cstdint>
#include <string>

#include "filemgmt_libn.h"

namespace OHOS::FileManagement::CloudSync {

struct CheckTriggerInfoEntity {
    std::string bundleName;
    std::string triggerBundleName;
    int32_t reason{0};
};

class CheckTriggerInfoNapi final : public LibN::NExporter {
public:
    CheckTriggerInfoNapi(napi_env env, napi_value exports) : NExporter(env, exports) {}
    ~CheckTriggerInfoNapi() = default;

    bool Export() override;
    std::string GetClassName() override;
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static napi_value GetBundleName(napi_env env, napi_callback_info info);
    static napi_value GetTriggerBundleName(napi_env env, napi_callback_info info);
    static napi_value GetReason(napi_env env, napi_callback_info info);

    inline static const std::string className_ = "CheckTriggerInfoInner";
};

} // namespace OHOS::FileManagement::CloudSync
#endif // OHOS_FILEMGMT_CHECK_TRIGGER_INFO_NAPI_H
