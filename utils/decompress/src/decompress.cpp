/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "decompress.h"
#include "decompress_gains_reporter.h"

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <mutex>
#include <securec.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_set>

#include "dfs_error.h"
#include "fuse_ioctl.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement {
namespace Decompress {
using namespace CloudFile;

namespace {
constexpr const char *PDEDUP_FEATURE_NODE_PATH = "/sys/fs/hmfs/userdata/support_pdedup";
constexpr size_t MAX_PATH_LENGTH = PATH_MAX;
constexpr const char *CLOUD_SYNC_LIB_PATH = "libcloudsync_kit_inner.z.so";
constexpr int MAX_RETRY = 2;
constexpr int RETRY_DELAY_US = 100000;
constexpr const char *DST_PATH = "/data/app/el1/bundle/public";
constexpr int32_t PDEDUP_CREATE_VERSION = 2;
} // namespace

using GetUnsupportedListFunc = int32_t (*)(std::vector<std::string> *);
using GetSystemFeatureFunc = int32_t (*)(bool *);
using GetCrossFsConfigFunc = int32_t (*)(bool *);

class CloudSyncLibraryManager {
public:
    static CloudSyncLibraryManager &GetInstance()
    {
        static CloudSyncLibraryManager instance;
        return instance;
    }

    void *GetHandle() const
    {
        return handle_;
    }
    bool IsLoaded() const
    {
        return handle_ != nullptr;
    }

    GetUnsupportedListFunc GetUnsupportedListSymbol()
    {
        return GetSymbolFunc<GetUnsupportedListFunc>("CloudSync_GetDecompressUnsupportedList");
    }

    GetSystemFeatureFunc GetSystemFeatureSymbol()
    {
        return GetSymbolFunc<GetSystemFeatureFunc>("CloudSync_GetDecompressSystemFeature");
    }

    GetCrossFsConfigFunc GetCrossFsConfigSymbol()
    {
        return GetSymbolFunc<GetCrossFsConfigFunc>("CloudSync_GetDecompressCrossFsConfig");
    }

private:
    CloudSyncLibraryManager()
    {
        handle_ = dlopen(CLOUD_SYNC_LIB_PATH, RTLD_LAZY | RTLD_LOCAL | RTLD_NODELETE);
        if (handle_ == nullptr) {
            LOGE("dlopen failed for %{public}s: %{public}s", CLOUD_SYNC_LIB_PATH, dlerror());
        }
    }

    ~CloudSyncLibraryManager()
    {
        // No dlclose: RTLD_NODELETE ensures library stays loaded, system auto-cleanup.
    }

    template<typename FuncType>
    FuncType GetSymbolFunc(const char *symbol)
    {
        if (handle_ == nullptr) {
            return nullptr;
        }
        auto func = reinterpret_cast<FuncType>(dlsym(handle_, symbol));
        if (func == nullptr) {
            LOGE("dlsym %{public}s failed: %{public}s", symbol, dlerror());
        }
        return func;
    }

    void *handle_ = nullptr;
};

static bool SplitTargetPath(const std::string &targetPath, std::string &dirPath, std::string &fileName)
{
    size_t lastSlash = targetPath.find_last_of('/');
    if (lastSlash == std::string::npos) {
        LOGE("Target path must be absolute path: %{public}s", targetPath.c_str());
        return false;
    }
    dirPath = targetPath.substr(0, lastSlash);
    fileName = targetPath.substr(lastSlash + 1);
    return true;
}

static bool ValidateSourcePath(const std::string &path)
{
    if (path.empty() || path.length() > MAX_PATH_LENGTH) {
        LOGE("Source path is empty or too long: %{public}zu", path.length());
        return false;
    }

    char resolvedPath[PATH_MAX] = {0};
    if (realpath(path.c_str(), resolvedPath) == nullptr) {
        LOGE("realpath failed for source path: %{public}s, errno: %{public}d", path.c_str(), errno);
        return false;
    }

    return true;
}

static bool ValidateTargetPath(const std::string &targetPath)
{
    if (targetPath.empty() || targetPath.length() > MAX_PATH_LENGTH) {
        LOGE("Target path is empty or too long: %{public}zu", targetPath.length());
        return false;
    }

    std::string dirPath;
    std::string fileName;
    if (!SplitTargetPath(targetPath, dirPath, fileName)) {
        return false;
    }

    char resolvedDirPath[PATH_MAX] = {0};
    if (realpath(dirPath.c_str(), resolvedDirPath) == nullptr) {
        LOGE("realpath failed for target directory: %{public}s, errno: %{public}d", dirPath.c_str(), errno);
        return false;
    }
    return true;
}

static int OpenSourceFile(const std::string &sourcePath)
{
    int innerFd = open(sourcePath.c_str(), O_RDONLY);
    if (innerFd < 0) {
        LOGE("Failed to open source file: %{public}s, errno: %{public}d", sourcePath.c_str(), errno);
        return -1;
    }
    return innerFd;
}

static struct hmfs_pdedup_create_arg_v2 *PreparePdedupCreate(int innerFd,
                                                             const std::string &fileName,
                                                             uint32_t offset,
                                                             uint32_t size,
                                                             bool generateBuildhash)
{
    size_t createSize = sizeof(struct hmfs_pdedup_create_arg_v2) + sizeof(struct hmfs_pdedup_extent);
    auto *create = static_cast<struct hmfs_pdedup_create_arg_v2 *>(malloc(createSize));
    if (!create) {
        LOGE("Failed to allocate memory for pdedup create");
        return nullptr;
    }

    errno_t ret = memset_s(create, createSize, 0, createSize);
    if (ret != EOK) {
        LOGE("memset_s failed: %{public}d", ret);
        free(create);
        return nullptr;
    }

    create->common.version = PDEDUP_CREATE_VERSION;
    create->common.inner_fd = innerFd;
    create->common.file_size = size;
    create->common.nr_extents = 1;
    create->common.mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    create->flags = generateBuildhash ? HMFS_PDEDUP_CREATE_FLAG_BUILD_HASH : 0;

    ret = strncpy_s(create->common.name, sizeof(create->common.name), fileName.c_str(), fileName.length());
    if (ret != EOK) {
        LOGE("strncpy_s failed: %{public}d", ret);
        free(create);
        return nullptr;
    }

    create->extents[0].outer_off = 0;
    create->extents[0].inner_off = offset;
    create->extents[0].len = size;

    return create;
}

struct InnerFileCtx {
    int innerFd = -1;
    int dirFd = -1;
    std::string dirPath;
    std::string fileName;
    struct hmfs_pdedup_create_arg_v2 *create = nullptr;
};

static bool ExecutePdedupCreate(InnerFileCtx &ctx, const CreateInnerFileParam &param)
{
    for (int i = 0; i < MAX_RETRY; i++) {
        int ioctlRet = ioctl(ctx.dirFd, HMFS_IOC_PDEDLUP_CREATE, ctx.create);
        if (ioctlRet >= 0) {
            LOGD("Successfully created pdedup file: %{private}s, offset: %{public}u, size: %{public}u",
                 param.targetPath.c_str(), param.offset, param.size);
            return true;
        }
        if (errno == EINTR || errno == EAGAIN || errno == EBUSY || errno == ENOMEM) {
            LOGW("ioctl retry %{public}d/%{public}d for %{public}s, errno: %{public}d", i + 1, MAX_RETRY,
                 param.targetPath.c_str(), errno);
            usleep(RETRY_DELAY_US);
            continue;
        }
        break;
    }

    int32_t lastErrno = errno;
    LOGE("Failed to create pdedup for %{public}s, offset: %{public}u, errno: %{public}d", param.targetPath.c_str(),
         param.offset, lastErrno);
    if (param.fileType == FileType::SO_FILE) {
        DecompressFaultReporter::GetInstance().ReportFsCommonInfo(
            {lastErrno, "Failed to create pdedup file", param.bundleName, ctx.fileName});
    }

    return false;
}

static bool PrepareInnerFileResources(const CreateInnerFileParam &param, InnerFileCtx &ctx)
{
    if (!ValidateSourcePath(param.sourcePath)) {
        LOGE("Invalid source path: %{public}s", param.sourcePath.c_str());
        return false;
    }
    if (!ValidateTargetPath(param.targetPath)) {
        LOGE("Invalid target path: %{public}s", param.targetPath.c_str());
        return false;
    }
    ctx.innerFd = OpenSourceFile(param.sourcePath);
    if (ctx.innerFd < 0) {
        return false;
    }
    if (!SplitTargetPath(param.targetPath, ctx.dirPath, ctx.fileName)) {
        close(ctx.innerFd);
        ctx.innerFd = -1;
        return false;
    }
    LOGD("SplitTargetPath result, dirPath: %{private}s, fileName: %{public}s", ctx.dirPath.c_str(),
         ctx.fileName.c_str());
    ctx.dirFd = open(ctx.dirPath.c_str(), O_RDONLY | O_DIRECTORY);
    if (ctx.dirFd < 0) {
        LOGE("Failed to open directory: %{public}s, errno: %{public}d", ctx.dirPath.c_str(), errno);
        close(ctx.innerFd);
        ctx.innerFd = -1;
        return false;
    }
    return true;
}

bool CreateInnerFile(const CreateInnerFileParam &param)
{
    LOGD(
        "CreateInnerFile start, sourcePath: %{private}s, targetPath: %{private}s, offset: %{public}u, size: "
        "%{public}u, "
        "bundleName: %{public}s, isSystemApp: %{public}d, fileType: %{public}d, generateBuildhash: %{public}d",
        param.sourcePath.c_str(), param.targetPath.c_str(), param.offset, param.size, param.bundleName.c_str(),
        static_cast<int>(param.isSystemApp), static_cast<int>(param.fileType),
        static_cast<int>(param.generateBuildhash));
    InnerFileCtx ctx;
    if (!PrepareInnerFileResources(param, ctx)) {
        return false;
    }
    ctx.create = PreparePdedupCreate(ctx.innerFd, ctx.fileName, param.offset, param.size, param.generateBuildhash);
    if (!ctx.create) {
        close(ctx.dirFd);
        close(ctx.innerFd);
        return false;
    }
    bool success = ExecutePdedupCreate(ctx, param);
    free(ctx.create);
    close(ctx.dirFd);
    close(ctx.innerFd);
    DecompressGainsReporter::GetInstance().UpdateGainsInfo(param, success);
    return success;
}

bool BundelInstallFinished(const std::string &bundleName)
{
    return DecompressGainsReporter::GetInstance().DumpAndReport(bundleName);
}

static void InitUnsupportedCache(std::unordered_set<std::string> &cache)
{
    auto &manager = CloudSyncLibraryManager::GetInstance();
    if (!manager.IsLoaded()) {
        LOGE("Failed to load cloud sync library");
        return;
    }
    auto func = manager.GetUnsupportedListSymbol();
    if (func != nullptr) {
        std::vector<std::string> unsupportedList;
        int32_t ret = func(&unsupportedList);
        if (ret == 0) {
            for (const auto &name : unsupportedList) {
                cache.insert(name);
            }
            LOGI("Cached unsupported list: %{public}zu items", cache.size());
        } else {
            LOGE("Get unsupported list failed, ret: %{public}d", ret);
        }
    }
}

static bool IsSupportDiffFileSystem()
{
    static bool cachedConfig = false;
    static std::once_flag initFlag;

    std::call_once(initFlag, []() {
        auto &manager = CloudSyncLibraryManager::GetInstance();
        if (!manager.IsLoaded()) {
            LOGE("Failed to load cloud sync library for cross fs config");
            return;
        }

        auto func = manager.GetCrossFsConfigSymbol();
        if (func != nullptr) {
            bool crossFsConfig = false;
            int32_t ret = func(&crossFsConfig);
            if (ret == 0) {
                cachedConfig = crossFsConfig;
                LOGI("Cross fs config cached: %{public}s", cachedConfig ? "true" : "false");
            } else {
                LOGE("Get cross fs config via IPC failed, ret: %{public}d", ret);
            }
        }
    });

    return cachedConfig;
}

bool CheckBundleSupported(const std::string &bundleName,
                          const bool isKeepAlive,
                          const std::string &hapPath,
                          const bool generateBuildhash)
{
    if (isKeepAlive) {
        LOGD("Bundle %{public}s iskeepAlive is true", bundleName.c_str());
        return false;
    }
    static std::unordered_set<std::string> unsupportedCache;
    static std::once_flag initFlag;

    std::call_once(initFlag, []() { InitUnsupportedCache(unsupportedCache); });

    bool supported = unsupportedCache.find(bundleName) == unsupportedCache.end();
    if (supported) {
        struct stat hapStat = {};
        struct stat dstStat = {};
        if (stat(hapPath.c_str(), &hapStat) != 0) {
            LOGE("stat hapPath failed: %{private}s, errno: %{public}d", hapPath.c_str(), errno);
            return false;
        }
        if (stat(DST_PATH, &dstStat) != 0) {
            LOGE("stat dstPath failed: %{private}s, errno: %{public}d", DST_PATH, errno);
            return false;
        }
        bool sameFileSystem = (hapStat.st_dev == dstStat.st_dev);
        if (!sameFileSystem) {
            bool supportDiffFs = IsSupportDiffFileSystem();
            supported = supportDiffFs && generateBuildhash;
            LOGD("Bundle %{public}s different filesystem, supportDiffFs: %{public}s, generateBuildhash: %{public}s, "
                 "supported: %{public}s",
                 bundleName.c_str(), supportDiffFs ? "true" : "false", generateBuildhash ? "true" : "false",
                 supported ? "true" : "false");
        }
    }

    LOGD("Bundle %{public}s is %{public}s", bundleName.c_str(), supported ? "supported" : "not supported");
    return supported;
}

static bool ReadPdedupFeatureNode()
{
    FILE *file = fopen(PDEDUP_FEATURE_NODE_PATH, "r");
    if (file == nullptr) {
        LOGD("Failed to open pdedup feature node file: %{public}s", PDEDUP_FEATURE_NODE_PATH);
        return false;
    }
    char *line = nullptr;
    size_t len = 0;
    ssize_t read = getline(&line, &len, file);
    if (fclose(file) != 0) {
        LOGE("Failed to close pdedup feature node file, errno: %{public}d", errno);
    }
    if (read == -1) {
        LOGD("Failed to read pdedup feature node file");
        if (line != nullptr) {
            free(line);
        }
        return false;
    }
    bool supported = (line != nullptr && line[0] == '1');
    LOGD("Pdedup feature node content: %{public}s, supported: %{public}s", line ? line : "",
         supported ? "true" : "false");
    if (line != nullptr) {
        free(line);
    }
    return supported;
}

bool GetSystemFeature()
{
    static bool cachedFeature = false;
    static std::once_flag initFlag;

    std::call_once(initFlag, []() {
        bool nodeSupported = ReadPdedupFeatureNode();
        if (!nodeSupported) {
            LOGD("Pdedup feature node not supported");
            return;
        }

        auto &manager = CloudSyncLibraryManager::GetInstance();
        if (!manager.IsLoaded()) {
            LOGE("Failed to load cloud sync library");
            return;
        }

        auto func = manager.GetSystemFeatureSymbol();
        if (func != nullptr) {
            bool systemFeature = false;
            int32_t ret = func(&systemFeature);
            if (ret == 0) {
                cachedFeature = systemFeature;
                LOGD("System feature cached: %{public}s", cachedFeature ? "true" : "false");
            } else {
                LOGE("Get system feature via IPC failed, ret: %{public}d", ret);
            }
        }
    });

    return cachedFeature;
}

} // namespace Decompress
} // namespace FileManagement
} // namespace OHOS