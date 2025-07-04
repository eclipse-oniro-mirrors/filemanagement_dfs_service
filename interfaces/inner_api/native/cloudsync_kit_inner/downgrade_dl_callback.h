/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_FILEMGMT_DOWNGRADE_DL_CALLBACK_H
#define OHOS_FILEMGMT_DOWNGRADE_DL_CALLBACK_H

#include <functional>

#include "cloud_sync_common.h"

namespace OHOS::FileManagement::CloudSync {

class DowngradeDlCallback {
public:
    virtual ~DowngradeDlCallback() = default;
    virtual void OnDownloadProcess(const DowngradeProgress &progress) = 0;
};
} // namespace OHOS::FileManagement::CloudSync

#endif // OHOS_FILEMGMT_DOWNGRADE_DL_CALLBACK_H
