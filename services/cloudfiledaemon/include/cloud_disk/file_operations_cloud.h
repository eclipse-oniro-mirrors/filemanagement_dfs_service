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
#ifndef CLOUD_FILE_DAEMON_FILE_OPERATIONS_CLOUD_H
#define CLOUD_FILE_DAEMON_FILE_OPERATIONS_CLOUD_H

#include "file_operations_base.h"

namespace OHOS {
namespace FileManagement {
namespace CloudDisk {
class FileOperationsCloud final : public FileOperationsBase {
public:
    void Lookup(fuse_req_t req, fuse_ino_t parent, const char *name) override;
    void Access(fuse_req_t req, fuse_ino_t ino, int mask) override;
    void GetAttr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) override;
    void Open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) override;
    void Forget(fuse_req_t req, fuse_ino_t ino, uint64_t nLookup) override;
    void ForgetMulti(fuse_req_t req, size_t count, struct fuse_forget_data *forgets) override;
    void MkNod(fuse_req_t req, fuse_ino_t parent, const char *name,
               mode_t mode, dev_t rdev) override;
    void Create(fuse_req_t req, fuse_ino_t parent, const char *name,
                mode_t mode, struct fuse_file_info *fi) override;
    void ReadDir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                 struct fuse_file_info *fi) override;
    void SetXattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                  const char *value, size_t size, int flags) override;
    void GetXattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                  size_t size) override;
};
} // namespace CloudDisk
} // namespace FileManagement
} // namespace OHOS
#endif // CLOUD_FILE_DAEMON_FILE_OPERATIONS_CLOUD_H