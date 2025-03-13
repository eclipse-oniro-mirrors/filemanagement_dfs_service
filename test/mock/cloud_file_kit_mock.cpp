
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

#include <gmock/gmock.h>
#include "cloud_file_kit.h"

namespace OHOS::FileManagement::CloudSync {

class CloudFileKitMock : public CloudFile::CloudFileKit {
public:
    MOCK_METHOD2(GetCloudUserInfo, int32_t(const int32_t userId, CloudFile::CloudUserInfo &userInfo));
    MOCK_METHOD3(GetAppSwitchStatus, int32_t(const std::string &bundleName, const int32_t userId, bool &switchStatus));
    MOCK_METHOD3(GetAppConfigParams, int32_t(const int32_t userId,
                const std::string &bundleName, std::map<std::string, std::string> &param));
};

}