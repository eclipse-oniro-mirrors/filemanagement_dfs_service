/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef FUSE_IOCTL_H
#define FUSE_IOCTL_H

#include <cstdint>
#include <sys/ioctl.h>

namespace OHOS {
namespace FileManagement {
namespace CloudFile {
static const unsigned int HMDFS_IOC = 0xf2;
static const unsigned int HMDFS_DAEMON_IOC_MASK = 0x80;

#define HMFS_IOCTL_MAGIC 0xf5
#define HMFS_DEDUP_IOC_BASE 50

#define HMDFS_IOC_GET_WRITEOPEN_CNT _IOR(HMDFS_IOC, 0x02, uint32_t)
#define HMDFS_IOC_HAS_CACHE _IOW(HMDFS_IOC, 0x06, struct HmdfsHasCache)
#define HMDFS_IOC_CANCEL_READ _IO(HMDFS_IOC, 0x08)
#define HMDFS_IOC_RESET_READ _IO(HMDFS_IOC, 0x09)
#define HMDFS_IOC_SAVE_APPID _IOW(HMDFS_IOC, 0x0a, struct HmdfsSaveAppId)
#define HMDFS_IOC_SET_CLOUD_GENERATION _IOR(HMDFS_IOC, 0x0b, uint32_t)
#define HMDFS_IOC_COPY_FILE _IOW(HMDFS_IOC, 0x0c, struct CloudDiskCopy)

/* New daemon-specific ioctls must be defined with the HMDFS_DAEMON_IOC_MASK for proper handling */
#define HMDFS_IOC_CLEAN_CACHE_DAEMON _IO(HMDFS_IOC, HMDFS_DAEMON_IOC_MASK + 0x01)
#define HMDFS_IOC_SET_KEEP_CACHE _IOW(HMDFS_IOC, HMDFS_DAEMON_IOC_MASK + 0x02, int32_t)

/* HMFS pdedup (pyysical dedup / no-decompress) ioctl interface */
struct hmfs_pdedup_extent {
    uint64_t outer_off; /* offset in outer file */
    uint64_t inner_off; /* offset in inner file */
    uint64_t len;       /* map len */
} __attribute__((packed));

#define HMFS_PDEDUP_NAME_MAX 256
struct hmfs_pdedup_create_arg_common {
    int32_t version;                 /* pdedup version*/
    int32_t inner_fd;                /* fd of inner file*/
    uint64_t file_size;              /* expected size of outer file*/
    uint32_t nr_extents;             /* extents num */
    int32_t mode;                    /* expected mode of outher file */
    char name[HMFS_PDEDUP_NAME_MAX];  /* expected name of outer file */
};

struct hmfs_pdedup_create_arg_v1 {
    struct hmfs_pdedup_create_arg_common common;
    struct hmfs_pdedup_extent extents[]; /* map extends */
};

#define HMFS_PDEDUP_CREATE_FLAG_BUILD_HASH 0x1
#define HMFS_PDEDUP_CREATE_RESERVED_NUM 64
struct hmfs_pdedup_create_arg_v2 {
    struct hmfs_pdedup_create_arg_common common;
    uint64_t flags;
    uint64_t reserved[HMFS_PDEDUP_CREATE_RESERVED_NUM];
    struct hmfs_pdedup_extent extents[]; /* map extends */
};

#define HMFS_IOC_PDEDLUP_CREATE _IOW(HMFS_IOCTL_MAGIC, HMFS_DEDUP_IOC_BASE + 8, struct hmfs_pdedup_create_arg_v1)

} // namespace CloudFile
} // namespace FileManagement
} // namespace OHOS
#endif // FUSE_IOCTL_H
