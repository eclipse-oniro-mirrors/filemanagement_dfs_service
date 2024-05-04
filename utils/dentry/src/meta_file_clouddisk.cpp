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

#include "meta_file.h"

#include <ctime>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>

#include "dfs_error.h"
#include "directory_ex.h"
#include "file_utils.h"
#include "securec.h"
#include "string_ex.h"
#include "sys/xattr.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement {
constexpr uint32_t DENTRYGROUP_SIZE = 4096;
constexpr uint32_t DENTRY_NAME_LEN = 8;
constexpr uint32_t DENTRY_RESERVED_LENGTH = 4;
constexpr uint32_t DENTRY_PER_GROUP = 52;
constexpr uint32_t DENTRY_BITMAP_LENGTH = 7;
constexpr uint32_t DENTRY_GROUP_RESERVED = 32;
constexpr uint32_t CLOUD_RECORD_ID_LEN = 33;
constexpr uint32_t DENTRYGROUP_HEADER = 4096;
constexpr uint32_t MAX_BUCKET_LEVEL = 63;
constexpr uint32_t BUCKET_BLOCKS = 2;
constexpr uint32_t BITS_PER_BYTE = 8;
constexpr uint32_t HMDFS_SLOT_LEN_BITS = 3;
constexpr uint64_t DELTA = 0x9E3779B9; /* Hashing code copied from f2fs */
constexpr uint64_t HMDFS_HASH_COL_BIT = (0x1ULL) << 63;
constexpr uint32_t CLOUD_ID_BUCKET_MAX_SIZE = 32;
constexpr uint32_t CLOUD_ID_BUCKET_MID_TIMES = 2;
constexpr uint32_t CLOUD_ID_MIN_SIZE = 3;
constexpr uint32_t FILE_TYPE_OFFSET = 2;

#pragma pack(push, 1)
struct HmdfsDentry {
    uint32_t hash{0};
    uint16_t mode{0};
    uint16_t namelen{0};
    uint64_t size{0};
    uint64_t atime{0};
    uint64_t mtime{0};
    uint8_t recordId[CLOUD_RECORD_ID_LEN]{0};
    uint8_t flags{0};
    /* reserved bytes for long term extend, total 60 bytes */
    uint8_t reserved[DENTRY_RESERVED_LENGTH];
};

struct HmdfsDentryGroup {
    uint8_t dentryVersion;
    uint8_t bitmap[DENTRY_BITMAP_LENGTH];
    struct HmdfsDentry nsl[DENTRY_PER_GROUP];
    uint8_t fileName[DENTRY_PER_GROUP][DENTRY_NAME_LEN];
    uint8_t reserved[DENTRY_GROUP_RESERVED];
};
static_assert(sizeof(HmdfsDentryGroup) == DENTRYGROUP_SIZE);

struct HmdfsDcacheHeader {
    uint64_t dcacheCrtime{0};
    uint64_t dcacheCrtimeNsec{0};

    uint64_t dentryCtime{0};
    uint64_t dentryCtimeNsec{0};

    uint64_t dentryCount{0};
};
#pragma pack(pop)

void MetaHelper::SetFileType(struct HmdfsDentry *de, uint8_t fileType)
{
    de->flags &= 0x3;
    de->flags |= (fileType << FILE_TYPE_OFFSET);
}

void MetaHelper::SetPosition(struct HmdfsDentry *de, uint8_t position)
{
    de->flags &= 0xFC;
    de->flags |= position;
}

uint8_t MetaHelper::GetFileType(const struct HmdfsDentry *de)
{
    return (de->flags & 0xFC) >> FILE_TYPE_OFFSET;
}

uint8_t MetaHelper::GetPosition(const struct HmdfsDentry *de)
{
    return de->flags & 0x3;
}

static uint32_t GetBucketId(std::string cloudId)
{
    size_t size = cloudId.size();
    if (size < CLOUD_ID_MIN_SIZE) {
        return 0;
    }

    char first = cloudId[0];
    char last = cloudId[size - 1];
    char middle = cloudId[size / CLOUD_ID_BUCKET_MID_TIMES];
    return (first + last + middle) % CLOUD_ID_BUCKET_MAX_SIZE;
}

static std::string GetCloudDiskDentryFileByPath(uint32_t userId, const std::string &bundleName,
    const std::string &cloudId)
{
    std::string cacheDir =
        "/data/service/el2/" + std::to_string(userId) +
        "/hmdfs/cloud/data/" + bundleName + "/" + std::to_string(GetBucketId(cloudId)) + "/";
    std::string dentryFileName = MetaFileMgr::GetInstance().CloudIdToRecordId(cloudId);
    ForceCreateDirectory(cacheDir);
    return cacheDir + dentryFileName;
}

CloudDiskMetaFile::CloudDiskMetaFile(uint32_t userId, const std::string &bundleName, const std::string &cloudId)
{
    userId_ = userId;
    bundleName_ = bundleName;
    cloudId_ = cloudId;
    cacheFile_ = GetCloudDiskDentryFileByPath(userId_, bundleName_, cloudId_);
    fd_ = UniqueFd{open(cacheFile_.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)};
    LOGD("CloudDiskMetaFile cloudId=%{public}s, path=%{public}s", cloudId_.c_str(), cacheFile_.c_str());
    LOGD("CloudDiskMetaFile fd=%{public}d, errno :%{public}d", fd_.Get(), errno);

    HmdfsDcacheHeader header{};
    (void)FileUtils::ReadFile(fd_, 0, sizeof(header), &header);
    dentryCount_ = header.dentryCount;
}

int32_t CloudDiskMetaFile::DoLookupAndUpdate(const std::string &name, CloudDiskMetaFileCallBack callback)
{
    return E_OK;
}

int32_t CloudDiskMetaFile::DoLookupAndRemove(MetaBase &metaBase)
{
    return 0;
}

CloudDiskMetaFile::~CloudDiskMetaFile()
{
    HmdfsDcacheHeader header{.dentryCount = dentryCount_};
    (void)FileUtils::WriteFile(fd_, &header, 0, sizeof(header));
}

static bool IsDotDotdot(const std::string &name)
{
    return name == "." || name == "..";
}

static void Str2HashBuf(const char *msg, size_t len, uint32_t *buf, int num)
{
    uint32_t pad = static_cast<uint32_t>(len) | (static_cast<uint32_t>(len) << 8);
    pad |= pad << 16; /* hash pad length 16 */

    uint32_t val = pad;
    len = std::min(len, static_cast<size_t>(num * sizeof(int)));
    for (uint32_t i = 0; i < len; i++) {
        if ((i % sizeof(int)) == 0) {
            val = pad;
        }
        uint8_t c = static_cast<uint8_t>(tolower(msg[i]));
        val = c + (val << 8); /* hash shift size 8 */
        if ((i % 4) == 3) {   /* msg size 4, shift when 3 */
            *buf++ = val;
            val = pad;
            num--;
        }
    }
    if (--num >= 0) {
        *buf++ = val;
    }
    while (--num >= 0) {
        *buf++ = pad;
    }
}

static void TeaTransform(uint32_t buf[4], uint32_t const in[])
{
    int n = 16;           /* transform total rounds 16 */
    uint32_t a = in[0];   /* transform input pos 0 */
    uint32_t b = in[1];   /* transform input pos 1 */
    uint32_t c = in[2];   /* transform input pos 2 */
    uint32_t d = in[3];   /* transform input pos 3 */
    uint32_t b0 = buf[0]; /* buf pos 0 */
    uint32_t b1 = buf[1]; /* buf pos 1 */
    uint32_t sum = 0;

    do {
        sum += DELTA;
        b0 += ((b1 << 4) + a) ^ (b1 + sum) ^ ((b1 >> 5) + b); /* tea transform width 4 and 5 */
        b1 += ((b0 << 4) + c) ^ (b0 + sum) ^ ((b0 >> 5) + d); /* tea transform width 4 and 5 */
    } while (--n);

    buf[0] += b0;
    buf[1] += b1;
}

static uint32_t DentryHash(const std::string &name)
{
    if (IsDotDotdot(name)) {
        return 0;
    }

    constexpr int inLen = 8;      /* hash input buf size 8 */
    constexpr int bufLen = 4;     /* hash output buf size 4 */
    uint32_t in[inLen];
    uint32_t buf[bufLen];
    auto len = name.length();
    constexpr decltype(len) hashWidth = 16; /* hash operation width 4 */
    const char *p = name.c_str();

    buf[0] = 0x67452301; /* hash magic 1 */
    buf[1] = 0xefcdab89; /* hash magic 2 */
    buf[2] = 0x98badcfe; /* hash magic 3 */
    buf[3] = 0x10325476; /* hash magic 4 */

    bool loopFlag = true;
    while (loopFlag) {
        Str2HashBuf(p, len, in, bufLen);
        TeaTransform(buf, in);

        if (len <= hashWidth) {
            break;
        }

        p += hashWidth;
        len -= hashWidth;
    };
    uint32_t hash = buf[0];
    uint32_t hmdfsHash = hash & ~HMDFS_HASH_COL_BIT;

    return hmdfsHash;
}

static inline uint32_t GetDentrySlots(size_t nameLen)
{
    return static_cast<uint32_t>((nameLen + BITS_PER_BYTE - 1) >> HMDFS_SLOT_LEN_BITS);
}

static inline off_t GetDentryGroupPos(size_t bidx)
{
    return bidx * DENTRYGROUP_SIZE + DENTRYGROUP_HEADER;
}

static inline uint64_t GetDentryGroupCnt(uint64_t size)
{
    return (size >= DENTRYGROUP_HEADER) ? ((size - DENTRYGROUP_HEADER) / DENTRYGROUP_SIZE) : 0;
}

static uint32_t GetOverallBucket(uint32_t level)
{
    if (level >= MAX_BUCKET_LEVEL) {
        LOGD("level = %{public}d overflow", level);
        return 0;
    }
    uint64_t buckets = (1ULL << (level + 1)) - 1;
    return static_cast<uint32_t>(buckets);
}

static size_t GetDcacheFileSize(uint32_t level)
{
    size_t buckets = GetOverallBucket(level);
    return buckets * DENTRYGROUP_SIZE * BUCKET_BLOCKS + DENTRYGROUP_HEADER;
}

static uint32_t GetBucketaddr(uint32_t level, uint32_t buckoffset)
{
    if (level >= MAX_BUCKET_LEVEL) {
        return 0;
    }

    uint64_t curLevelMaxBucks = (1ULL << level);
    if (buckoffset >= curLevelMaxBucks) {
        return 0;
    }

    return static_cast<uint32_t>(curLevelMaxBucks) + buckoffset - 1;
}

static uint32_t GetBucketByLevel(uint32_t level)
{
    if (level >= MAX_BUCKET_LEVEL) {
        LOGD("level = %{public}d overflow", level);
        return 0;
    }

    uint64_t buckets = (1ULL << level);
    return static_cast<uint32_t>(buckets);
}

static uint32_t RoomForFilename(const uint8_t bitmap[], size_t slots, uint32_t maxSlots)
{
    uint32_t bitStart = 0;
    bool loopFlag = true;
    while (loopFlag) {
        uint32_t zeroStart = BitOps::FindNextZeroBit(bitmap, maxSlots, bitStart);
        if (zeroStart >= maxSlots) {
            return maxSlots;
        }

        uint32_t zeroEnd = BitOps::FindNextBit(bitmap, maxSlots, zeroStart);
        if (zeroEnd - zeroStart >= slots) {
            return zeroStart;
        }

        bitStart = zeroEnd + 1;
        if (zeroEnd + 1 >= maxSlots) {
            return maxSlots;
        }
    }
    return 0;
}

static void UpdateDentry(HmdfsDentryGroup &d, const MetaBase &base, uint32_t nameHash, uint32_t bitPos)
{
    HmdfsDentry *de;
    const std::string name = base.name;
    uint32_t slots = GetDentrySlots(name.length());

    de = &d.nsl[bitPos];
    de->hash = nameHash;
    de->namelen = name.length();
    auto ret = memcpy_s(d.fileName[bitPos], slots * DENTRY_NAME_LEN, name.c_str(), name.length());
    if (ret != 0) {
        LOGE("memcpy_s failed, dstLen = %{public}d, srcLen = %{public}zu", slots * DENTRY_NAME_LEN, name.length());
    }
    de->atime = base.atime;
    de->mtime = base.mtime;
    de->size = base.size;
    de->mode = base.mode;
    MetaHelper::SetPosition(de, base.position);
    MetaHelper::SetFileType(de, base.fileType);
    ret = memcpy_s(de->recordId, CLOUD_RECORD_ID_LEN, base.cloudId.c_str(), base.cloudId.length());
    if (ret != 0) {
        LOGE("memcpy_s failed, dstLen = %{public}d, srcLen = %{public}zu", CLOUD_RECORD_ID_LEN, base.cloudId.length());
    }

    for (uint32_t i = 0; i < slots; i++) {
        BitOps::SetBit(bitPos + i, d.bitmap);
        if (i) {
            (de + i)->namelen = 0;
        }
    }
}

int32_t CloudDiskMetaFile::HandleFileByFd(unsigned long &endBlock, uint32_t &level)
{
    struct stat fileStat;
    int err = fstat(fd_, &fileStat);
    if (err < 0) {
        return EINVAL;
    }
    if ((endBlock > GetDentryGroupCnt(fileStat.st_size)) &&
        ftruncate(fd_, GetDcacheFileSize(level))) {
        return ENOENT;
    }
    return E_OK;
}

static unsigned long GetBidxFromLevel(uint32_t level, uint32_t namehash)
{
    uint32_t bucket = GetBucketByLevel(level);
    if (bucket == 0) {
        return 0;
    }
    return BUCKET_BLOCKS * GetBucketaddr(level, namehash % bucket);
}

int32_t CloudDiskMetaFile::DoCreate(const MetaBase &base)
{
    if (fd_ < 0) {
        LOGE("bad metafile fd");
        return EINVAL;
    }
    off_t pos = 0;
    uint32_t level = 0;
    uint32_t bitPos = 0;
    uint32_t namehash = 0;
    unsigned long bidx = 0;
    HmdfsDentryGroup dentryBlk = {0};
    std::unique_lock<std::mutex> lock(mtx_);
    FileRangeLock fileLock(fd_, 0, 0);
    namehash = DentryHash(base.name);
    bool found = false;
    while (!found) {
        if (level == MAX_BUCKET_LEVEL) {
            return ENOSPC;
        }
        bidx = GetBidxFromLevel(level, namehash);
        unsigned long endBlock = bidx + BUCKET_BLOCKS;
        int32_t ret = HandleFileByFd(endBlock, level);
        if (ret != E_OK) {
            return ret;
        }
        for (; bidx < endBlock; bidx++) {
            pos = GetDentryGroupPos(bidx);
            int size = FileUtils::ReadFile(fd_, pos, DENTRYGROUP_SIZE, &dentryBlk);
            if (size != DENTRYGROUP_SIZE) {
                return ENOENT;
            }
            bitPos = RoomForFilename(dentryBlk.bitmap, GetDentrySlots(base.name.length()), DENTRY_PER_GROUP);
            if (bitPos < DENTRY_PER_GROUP) {
                found = true;
                break;
            }
        }
        ++level;
    }
    pos = GetDentryGroupPos(bidx);
    UpdateDentry(dentryBlk, base, namehash, bitPos);
    int size = FileUtils::WriteFile(fd_, &dentryBlk, pos, DENTRYGROUP_SIZE);
    if (size != DENTRYGROUP_SIZE) {
        LOGD("WriteFile failed, size %{public}d != %{public}d", size, DENTRYGROUP_SIZE);
        return EINVAL;
    }
    ++dentryCount_;
    return E_OK;
}

struct DcacheLookupCtx {
    int fd{-1};
    std::string name{};
    uint32_t hash{0};
    uint32_t bidx{0};
    std::unique_ptr<HmdfsDentryGroup> page{nullptr};
};

static void InitDcacheLookupCtx(DcacheLookupCtx *ctx, const MetaBase &base, int fd)
{
    ctx->fd = fd;
    ctx->name = base.name;
    ctx->bidx = 0;
    ctx->page = nullptr;
    ctx->hash = DentryHash(ctx->name);
}

static std::unique_ptr<HmdfsDentryGroup> FindDentryPage(uint64_t index, DcacheLookupCtx *ctx)
{
    auto dentryBlk = std::make_unique<HmdfsDentryGroup>();

    off_t pos = GetDentryGroupPos(index);
    ssize_t size = FileUtils::ReadFile(ctx->fd, pos, DENTRYGROUP_SIZE, dentryBlk.get());
    if (size != DENTRYGROUP_SIZE) {
        return nullptr;
    }
    return dentryBlk;
}

static HmdfsDentry *FindInBlock(HmdfsDentryGroup &dentryBlk, uint32_t namehash, const std::string &name)
{
    int maxLen = 0;
    uint32_t bitPos = 0;
    HmdfsDentry *de = nullptr;

    while (bitPos < DENTRY_PER_GROUP) {
        if (!BitOps::TestBit(bitPos, dentryBlk.bitmap)) {
            bitPos++;
            maxLen++;
            continue;
        }
        de = &dentryBlk.nsl[bitPos];
        if (!de->namelen) {
            bitPos++;
            continue;
        }

        if (de->hash == namehash && de->namelen == name.length() &&
            !memcmp(name.c_str(), dentryBlk.fileName[bitPos], de->namelen)) {
            return de;
        }
        maxLen = 0;
        bitPos += GetDentrySlots(de->namelen);
    }

    return nullptr;
}

static HmdfsDentry *InLevel(uint32_t level, DcacheLookupCtx *ctx)
{
    HmdfsDentry *de = nullptr;

    uint32_t nbucket = GetBucketByLevel(level);
    if (nbucket == 0) {
        return de;
    }

    uint32_t bidx = GetBucketaddr(level, ctx->hash % nbucket) * BUCKET_BLOCKS;
    uint32_t endBlock = bidx + BUCKET_BLOCKS;

    for (; bidx < endBlock; bidx++) {
        auto dentryBlk = FindDentryPage(bidx, ctx);
        if (dentryBlk == nullptr) {
            break;
        }

        de = FindInBlock(*dentryBlk, ctx->hash, ctx->name);
        if (de != nullptr) {
            ctx->page = std::move(dentryBlk);
            break;
        }
    }
    ctx->bidx = bidx;
    return de;
}

static HmdfsDentry *FindDentry(DcacheLookupCtx *ctx)
{
    for (uint32_t level = 0; level < MAX_BUCKET_LEVEL; level++) {
        HmdfsDentry *de = InLevel(level, ctx);
        if (de != nullptr) {
            return de;
        }
    }
    return nullptr;
}

int32_t CloudDiskMetaFile::DoRemove(const MetaBase &base)
{
    if (fd_ < 0) {
        LOGE("bad metafile fd");
        return EINVAL;
    }

    std::unique_lock<std::mutex> lock(mtx_);
    FileRangeLock fileLock(fd_, 0, 0);
    DcacheLookupCtx ctx;
    InitDcacheLookupCtx(&ctx, base, fd_);
    HmdfsDentry *de = FindDentry(&ctx);
    if (de == nullptr) {
        LOGE("find dentry failed");
        return ENOENT;
    }

    uint32_t bitPos = (de - ctx.page->nsl);
    uint32_t slots = GetDentrySlots(de->namelen);
    for (uint32_t i = 0; i < slots; i++) {
        BitOps::ClearBit(bitPos + i, ctx.page->bitmap);
    }

    off_t ipos = GetDentryGroupPos(ctx.bidx);
    ssize_t size = FileUtils::WriteFile(fd_, ctx.page.get(), ipos, sizeof(HmdfsDentryGroup));
    if (size != sizeof(HmdfsDentryGroup)) {
        LOGE("WriteFile failed!, ret = %{public}zd", size);
        return EIO;
    }

    --dentryCount_;
    return E_OK;
}

int32_t CloudDiskMetaFile::DoLookup(MetaBase &base)
{
    if (fd_ < 0) {
        LOGE("bad metafile fd");
        return EINVAL;
    }

    std::unique_lock<std::mutex> lock(mtx_);
    FileRangeLock fileLock(fd_, 0, 0);
    struct DcacheLookupCtx ctx;
    InitDcacheLookupCtx(&ctx, base, fd_);
    struct HmdfsDentry *de = FindDentry(&ctx);
    if (de == nullptr) {
        LOGD("find dentry failed");
        return ENOENT;
    }

    base.size = de->size;
    base.atime = de->atime;
    base.mtime = de->mtime;
    base.mode = de->mode;
    base.position = MetaHelper::GetPosition(de);
    base.fileType = MetaHelper::GetFileType(de);
    base.cloudId = std::string(reinterpret_cast<const char *>(de->recordId), CLOUD_RECORD_ID_LEN);
    return E_OK;
}

int32_t CloudDiskMetaFile::DoUpdate(const MetaBase &base)
{
    if (fd_ < 0) {
        LOGE("bad metafile fd");
        return EINVAL;
    }

    std::unique_lock<std::mutex> lock(mtx_);
    FileRangeLock fileLock(fd_, 0, 0);
    struct DcacheLookupCtx ctx;
    InitDcacheLookupCtx(&ctx, base, fd_);
    struct HmdfsDentry *de = FindDentry(&ctx);
    if (de == nullptr) {
        LOGD("find dentry failed");
        return ENOENT;
    }

    de->atime = base.atime;
    de->mtime = base.mtime;
    de->size = base.size;
    de->mode = base.mode;
    MetaHelper::SetPosition(de, base.position);
    MetaHelper::SetFileType(de, base.fileType);
    auto ret = memcpy_s(de->recordId, CLOUD_RECORD_ID_LEN, base.cloudId.c_str(), base.cloudId.length());
    if (ret != 0) {
        LOGE("memcpy_s failed, dstLen = %{public}d, srcLen = %{public}zu", CLOUD_RECORD_ID_LEN, base.cloudId.length());
    }

    off_t ipos = GetDentryGroupPos(ctx.bidx);
    ssize_t size = FileUtils::WriteFile(fd_, ctx.page.get(), ipos, sizeof(struct HmdfsDentryGroup));
    if (size != sizeof(struct HmdfsDentryGroup)) {
        LOGE("write failed, ret = %{public}zd", size);
        return EIO;
    }
    return E_OK;
}

int32_t CloudDiskMetaFile::DoRename(MetaBase &metaBase, const std::string &newName,
    std::shared_ptr<CloudDiskMetaFile> newMetaFile)
{
    int32_t ret = DoRemove(metaBase);
    if (ret != E_OK) {
        LOGE("remove dentry failed, ret = %{public}d", ret);
        return ret;
    }
    metaBase.name = newName;
    ret = newMetaFile->DoCreate(metaBase);
    if (ret != E_OK) {
        LOGE("create dentry failed, ret = %{public}d", ret);
        return ret;
    }
    return E_OK;
}

std::shared_ptr<CloudDiskMetaFile> MetaFileMgr::GetCloudDiskMetaFile(uint32_t userId, const std::string &bundleName,
    const std::string &cloudId)
{
    return nullptr;
}

int32_t MetaFileMgr::MoveIntoRecycleDentryfile(uint32_t userId, const std::string &bundleName, const std::string &name,
    const std::string &parentCloudId, int64_t rowId)
{
    return E_OK;
}

int32_t MetaFileMgr::RemoveFromRecycleDentryfile(uint32_t userId, const std::string &bundleName,
    const std::string &name, const std::string &parentCloudId, int64_t rowId)
{
    return E_OK;
}
} // namespace FileManagement
} // namespace OHOS