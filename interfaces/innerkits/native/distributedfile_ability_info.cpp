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

#include "distributedfile_ability_info.h"

#include <string>

namespace OHOS {
namespace Storage {
namespace DistributedFile {

bool DistributedFileInfo::ReadFromParcel(Parcel &parcel)
{
    return true;
}

bool DistributedFileInfo::Marshalling(Parcel &parcel) const
{
    return true;
}

DistributedFileInfo *DistributedFileInfo::Unmarshalling(Parcel &parcel)
{
    return nullptr;
}

void DistributedFileInfo::Dump(std::string prefix, int fd) {}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
