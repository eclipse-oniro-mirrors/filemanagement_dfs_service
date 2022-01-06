/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <memory>
#include <string>
#include <vector>
#include "bundle_mgr_interface.h"
#include "bundle_mgr_proxy.h"
#include "service_proxy.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "utils_log.h"

using namespace OHOS;
using namespace OHOS::Storage::DistributedFile;

static void TestTutorial()
{
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    LOGE("xhl Get samgr %{public}p", samgr.GetRefPtr());

    auto remote = samgr->GetSystemAbility(STORAGE_DISTRIBUTED_FILE_SERVICE_SA_ID);
    LOGE("xhl Get remote %{public}p", remote.GetRefPtr());

    auto proxy = iface_cast<IDistributedFileService>(remote);
    LOGE("xhl Get proxy %{public}p", proxy.GetRefPtr());

    std::string cid{"123"};
    std::vector<std::string> srcList;
    std::vector<std::string> dstList;
    uint32_t fileCount = 2;
    LOGE("xhl prepare to sendfile");
    int ret = proxy->SendFile(cid, srcList, dstList, fileCount);
    LOGE("xhl sendfile finish ret %{public}d", ret);
    ret = proxy->sendTest();
    LOGE("xhl sendTest finish ret %{public}d", ret);
}

int main(int argc, char const *argv[])
{
    TestTutorial();
    LOGE("xhl main finish");
    return 0;
}