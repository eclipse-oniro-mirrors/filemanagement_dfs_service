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

#ifndef BASE_SESSION_H
#define BASE_SESSION_H

#include <array>
#include <string>

namespace OHOS {
namespace Storage {
namespace DistributedFile {
constexpr int KEY_SIZE_MAX = 32;

class BaseSession {
public:
    virtual ~BaseSession() = default;
    virtual bool IsFromServer() const = 0;
    virtual std::string GetCid() const = 0;
    virtual int32_t GetHandle() const = 0;
    virtual int GetSessionId() const = 0;
    virtual std::array<char, KEY_SIZE_MAX> GetKey() const = 0;
    virtual void Release() const = 0;
    virtual void DisableSessionListener() const = 0;
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
#endif // BASE_SESSION_H