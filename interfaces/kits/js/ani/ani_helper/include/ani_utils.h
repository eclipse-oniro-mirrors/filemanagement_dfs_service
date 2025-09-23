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

#ifndef OHOS_FILEMGMT_ANI_UTILS_H
#define OHOS_FILEMGMT_ANI_UTILS_H

#include <ani.h>
#include <functional>

#include "event_handler.h"
#include "event_runner.h"

namespace OHOS::FileManagement::CloudSync {

const std::string EVENT_TYPE = "progress";
static thread_local std::shared_ptr<OHOS::AppExecFwk::EventHandler> mainHandler;

class ANIUtils {
public:
    static ani_status AniString2String(ani_env *env, ani_string str, std::string &res);
    static bool SendEventToMainThread(const std::function<void()> func);

private:
    ANIUtils() = default;
    ~ANIUtils() = default;
};
} // OHOS::FileManagement::CloudSync
#endif // OHOS_FILEMGMT_ANI_UTILS_H