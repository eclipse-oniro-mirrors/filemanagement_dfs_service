/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_FILEMGMT_FILE_VERSION_ANI_H
#define OHOS_FILEMGMT_FILE_VERSION_ANI_H

#include "file_version_core.h"

namespace OHOS::FileManagement::CloudSync {
class FileVersionAni final {
public:
    static void FileVersionConstructor(ani_env *env, ani_object object);
    static ani_ref FileVersionGetHistoryVersionList(ani_env *env, ani_object object,
        ani_string uri, ani_int numLimit);
    static ani_string FileVersionDownloadHistoryVersion(ani_env *env, ani_object object,
        ani_string uri, ani_string versionId, ani_object fun);
    static void FileVersionReplaceFileWithHistoryVersion(ani_env *env, ani_object object,
        ani_string originalUri, ani_string versionUri);
    static ani_boolean FileVersionIsFileConflict(ani_env *env, ani_object object, ani_string uri);
    static void FileVersionClearFileConflict(ani_env *env, ani_object object, ani_string uri);
};
} // namespace OHOS::FileManagement::CloudSync
#endif // OHOS_FILEMGMT_FILE_VERSION_ANI_H
