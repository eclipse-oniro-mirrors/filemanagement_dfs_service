/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "data_handler.h"

#include "dfs_error.h"

namespace OHOS {
namespace FileManagement {
namespace CloudSync {
using namespace std;

int32_t DataHandler::SetCursor()
{
    return E_OK;
}

int32_t DataHandler::GetCursor()
{
    return E_OK;
}

int32_t DataHandler::GetFileModifiedRecords(vector<DriveKit::DKRecord> &records)
{
    return E_OK;
}

int32_t DataHandler::OnModifyFdirtyRecords(const map<DriveKit::DKRecordId,
    DriveKit::DKRecordOperResult> &map)
{
    return E_OK;
}
} // namespace CloudSync
} // namespace FileManagement
} // namespace OHOS