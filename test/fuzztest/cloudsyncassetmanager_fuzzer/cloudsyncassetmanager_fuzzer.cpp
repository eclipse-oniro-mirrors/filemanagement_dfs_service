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
#include "cloudsyncassetmanager_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include "cloud_fuzzer_helper.h"
#include "cloud_sync_asset_manager_impl.h"

namespace OHOS {
constexpr size_t U32_AT_SIZE = 4;
constexpr size_t BOOL_AT_SIZE = 1;
constexpr size_t SPLITE_SIZE = 6;
constexpr size_t NETWORK_ID_SIZE = 65;

using namespace OHOS::FileManagement::CloudSync;
using namespace std;

void UploadAssetFuzzTest(FuzzData &fuzzData, size_t size)
{
    fuzzData.ResetData(size);
    int32_t userId = fuzzData.GetData<int32_t>();
    std::string request = fuzzData.GetStringFromData(static_cast<int>(size - U32_AT_SIZE));
    std::string result;
    CloudSyncAssetManager::GetInstance().UploadAsset(userId, request, result);
}

void DownloadFileFuzzTest(FuzzData &fuzzData, size_t size)
{
    fuzzData.ResetData(size);
    int32_t userId = fuzzData.GetData<int32_t>();
    int len = static_cast<int>((size - U32_AT_SIZE) / SPLITE_SIZE);
    if (len <= 0) {
        return;
    }
    AssetInfo assetInfo = {
        .uri = fuzzData.GetStringFromData(len),
        .recordType = fuzzData.GetStringFromData(len),
        .recordId = fuzzData.GetStringFromData(len),
        .fieldKey = fuzzData.GetStringFromData(len),
        .assetName = fuzzData.GetStringFromData(len),
    };
    std::string bundleName = fuzzData.GetStringFromData(len);
    CloudSyncAssetManager::GetInstance().DownloadFile(userId, bundleName, assetInfo);
}

void DownloadFilesFuzzTest(FuzzData &fuzzData, size_t size)
{
    fuzzData.ResetData(size);
    int32_t userId = fuzzData.GetData<int32_t>();
    int len = static_cast<int>((size - U32_AT_SIZE) / SPLITE_SIZE);
    if (len <= 0) {
        return;
    }
    AssetInfo assetInfo = {
        .uri = fuzzData.GetStringFromData(len),
        .recordType = fuzzData.GetStringFromData(len),
        .recordId = fuzzData.GetStringFromData(len),
        .fieldKey = fuzzData.GetStringFromData(len),
        .assetName = fuzzData.GetStringFromData(len),
    };
    vector<AssetInfo> assetInfos;
    assetInfos.emplace_back(assetInfo);
    std::string bundleName = fuzzData.GetStringFromData(len);
    vector<bool> assetResultMap;
    CloudSyncAssetManager::GetInstance().DownloadFiles(userId, bundleName, assetInfos, assetResultMap);
}

void DownloadFileCallbackFuzzTest(FuzzData &fuzzData, size_t size)
{
    fuzzData.ResetData(size);
    int32_t userId = fuzzData.GetData<int32_t>();
    std::string networkId = fuzzData.GetStringFromData(static_cast<int>(NETWORK_ID_SIZE));
    auto remainSize = fuzzData.GetRemainSize();
    int len = static_cast<int>(remainSize / SPLITE_SIZE);
    if (len <= 0) {
        return;
    }
    std::string bundleName = fuzzData.GetStringFromData(len);
    AssetInfo assetInfo = {
        .uri = fuzzData.GetStringFromData(len),
        .recordType = fuzzData.GetStringFromData(len),
        .recordId = fuzzData.GetStringFromData(len),
        .fieldKey = fuzzData.GetStringFromData(len),
        .assetName = fuzzData.GetStringFromData(len),
    };
    auto callback = [](const std::string &, int32_t) {};
    CloudSyncAssetManager::GetInstance().DownloadFile(userId, bundleName, networkId, assetInfo, callback);
}

void DeleteAssetFuzzTest(FuzzData &fuzzData, size_t size)
{
    fuzzData.ResetData(size);
    int32_t userId = fuzzData.GetData<int32_t>();
    std::string uri = fuzzData.GetStringFromData(static_cast<int>(size - U32_AT_SIZE));
    CloudSyncAssetManager::GetInstance().DeleteAsset(userId, uri);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr ||
        size <= (OHOS::U32_AT_SIZE + static_cast<size_t>(OHOS::NETWORK_ID_SIZE + OHOS::SPLITE_SIZE))) {
        return 0;
    }

    OHOS::FuzzData fuzzData(data, size);
    OHOS::UploadAssetFuzzTest(fuzzData, size);
    OHOS::DownloadFileFuzzTest(fuzzData, size);
    OHOS::DownloadFilesFuzzTest(fuzzData, size);
    OHOS::DownloadFileCallbackFuzzTest(fuzzData, size);
    OHOS::DeleteAssetFuzzTest(fuzzData, size);
    return 0;
}
