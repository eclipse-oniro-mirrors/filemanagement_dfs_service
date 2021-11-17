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

#ifndef DISTRIBUTEDFILE_ABILITY_INFO_H
#define DISTRIBUTEDFILE_ABILITY_INFO_H

#include <string>

#include "parcel.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {

struct DistributedFileInfo : public Parcelable {
    std::string name;

    int32_t gid = 0;
    int32_t uid = 0;
    int32_t appId = 0;
    int32_t dmode = 0;
    int32_t mode = 0;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static DistributedFileInfo *Unmarshalling(Parcel &parcel);
    void Dump(std::string prefix, int fd);
};

}  // namespace DistributedFile
} // namespace Storage
}  // namespace OHOS
#endif  // DISTRIBUTEDFILE_ABILITY_INFO_H
