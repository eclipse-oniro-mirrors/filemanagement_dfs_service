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

#include "file_operations_base.h"

#include "assistant.h"

using namespace OHOS::FileManagement::CloudDisk;
void *fuse_req_userdata(fuse_req_t req)
{
    return Assistant::ins->fuse_req_userdata(req);
}

int fuse_reply_err(fuse_req_t req, int err)
{
    return Assistant::ins->fuse_reply_err(req, err);
}

int fuse_reply_open(fuse_req_t req, const struct fuse_file_info *fi)
{
    return Assistant::ins->fuse_reply_open(req, fi);
}

void fuse_reply_none(fuse_req_t req)
{
    return Assistant::ins->fuse_reply_none(req);
}

int fuse_reply_data(fuse_req_t req, struct fuse_bufvec *bufv, enum fuse_buf_copy_flags flags)
{
    return Assistant::ins->fuse_reply_data(req, bufv, flags);
}

size_t fuse_buf_size(const struct fuse_bufvec *bufv)
{
    return Assistant::ins->fuse_buf_size(bufv);
}

ssize_t fuse_buf_copy(struct fuse_bufvec *dst, struct fuse_bufvec *src, enum fuse_buf_copy_flags flags)
{
    return Assistant::ins->fuse_buf_copy(dst, src, flags);
}

int fuse_reply_write(fuse_req_t req, size_t count)
{
    return Assistant::ins->fuse_reply_write(req, count);
}

int fuse_reply_lseek(fuse_req_t req, off_t off)
{
    return Assistant::ins->fuse_reply_lseek(req, off);
}

int fuse_reply_buf(fuse_req_t req, const char *buf, size_t size)
{
    return Assistant::ins->fuse_reply_buf(req, buf, size);
}

int fuse_reply_xattr(fuse_req_t req, size_t count)
{
    return Assistant::ins->fuse_reply_xattr(req, count);
}

int fuse_reply_attr(fuse_req_t req, const struct stat *attr, double attr_timeout)
{
    return Assistant::ins->fuse_reply_attr(req, attr, attr_timeout);
}

int fuse_reply_entry(fuse_req_t req, const struct fuse_entry_param *e)
{
    return Assistant::ins->fuse_reply_entry(req, e);
}

int fuse_reply_create(fuse_req_t req, const struct fuse_entry_param *e, const struct fuse_file_info *f)
{
    return Assistant::ins->fuse_reply_create(req, e, f);
}

int fuse_reply_ioctl(fuse_req_t req, int result, const void *buf, size_t size)
{
    return Assistant::ins->fuse_reply_ioctl(req, result, buf, size);
}

int fuse_lowlevel_notify_inval_entry(struct fuse_session *se, fuse_ino_t parent, const char *name, size_t namelen)
{
    return Assistant::ins->fuse_lowlevel_notify_inval_entry(se, parent, name, namelen);
}

int fuse_lowlevel_notify_inval_inode(struct fuse_session *se, fuse_ino_t ino, off_t off, off_t len)
{
    return Assistant::ins->fuse_lowlevel_notify_inval_inode(se, ino, off, len);
}