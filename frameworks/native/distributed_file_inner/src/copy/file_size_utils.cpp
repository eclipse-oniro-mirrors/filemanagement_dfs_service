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

#include "copy/file_size_utils.h"

#include <sys/stat.h>
#include <filesystem>

#include "dfs_error.h"
#include "file_uri.h"
#include "utils_log.h"

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0xD004315
#define LOG_TAG "distributedfile_daemon"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
using namespace AppFileService;
using namespace AppFileService::ModuleFileUri;
using namespace FileManagement;
static constexpr int DISMATCH = 0;
static constexpr int MATCH = 1;
static constexpr char PATH_INVALID_FLAG1[] = "../";
static constexpr char PATH_INVALID_FLAG2[] = "/..";
static const uint32_t PATH_INVALID_FLAG_LEN = 3;
static const char FILE_SEPARATOR_CHAR = '/';
static constexpr char NETWORK_ID[] = "?networkid=";

int32_t FileSizeUtils::GetSize(const std::string &uri, bool isSrcUri, uint64_t &size)
{
    if (!IsFilePathValid(GetRealUri(uri))) {
        LOGE("path is forbidden");
        return OHOS::FileManagement::E_ILLEGAL_URI;
    }
    auto path = GetPathFromUri(uri, isSrcUri);
    if (path.empty()) {
        return ERR_BAD_VALUE;
    }

    bool isDirectory;
    auto ret = IsDirectory(uri, isSrcUri, isDirectory);
    if (ret != E_OK) {
        return ret;
    }
    if (isDirectory) {
        return GetDirSize(path, size);
    } else {
        return GetFileSize(path, size);
    }
}

int32_t FileSizeUtils::IsDirectory(const std::string &uri, bool isSrcUri, bool &isDirectory)
{
    if (!IsFilePathValid(GetRealUri(uri))) {
        LOGE("path is forbidden");
        return OHOS::FileManagement::E_ILLEGAL_URI;
    }
    auto path = GetPathFromUri(uri, isSrcUri);
    if (path.empty()) {
        return ERR_BAD_VALUE;
    }

    bool isDir = false;
    auto ret = IsDirectory(path, isDir);
    if (ret != E_OK) {
        return ret;
    }

    bool isFile = false;
    ret = IsFile(path, isFile);
    if (ret != E_OK) {
        return ret;
    }

    if (isDir == isFile) {
        return ERR_BAD_VALUE;
    }
    isDirectory = isDir;
    return E_OK;
}

int FileSizeUtils::FilterFunc(const struct dirent *filename)
{
    if (std::string_view(filename->d_name) == "." || std::string_view(filename->d_name) == "..") {
        return DISMATCH;
    }
    return MATCH;
}
 
void FileSizeUtils::Deleter(struct NameList *arg)
{
    for (int i = 0; i < arg->direntNum; i++) {
        if ((arg->namelist)[i]) {
            free((arg->namelist)[i]);
        }
        (arg->namelist)[i] = nullptr;
    }
    free(arg->namelist);
    arg->namelist = nullptr;
    delete arg;
    arg = nullptr;
}

std::unique_ptr<struct NameList, decltype(FileSizeUtils::Deleter) *> FileSizeUtils::GetDirNameList(
    const std::string &path)
{
    std::unique_ptr<struct NameList, decltype(Deleter) *> pNameList = { new (std::nothrow) struct NameList, Deleter };
    if (pNameList == nullptr) {
        LOGE("Failed to request heap memory.");
        return pNameList;
    }
    int num = scandir(path.c_str(), &(pNameList->namelist), FilterFunc, alphasort);
    pNameList->direntNum = num;
    return pNameList;
}

int32_t FileSizeUtils::GetFileSize(const std::string &path, uint64_t &size)
{
    struct stat buf {};
    int ret = stat(path.c_str(), &buf);
    if (ret == -1) {
        LOGI("Stat failed.");
        size = 0;
        return errno;
    }
    size = static_cast<uint64_t>(buf.st_size);
    return E_OK;
}

int32_t FileSizeUtils::GetDirSize(const std::string &path, uint64_t &size)
{
    auto pNameList = GetDirNameList(path);
    if (pNameList == nullptr) {
        return ENOMEM;
    }
    size = 0;
    for (int i = 0; i < pNameList->direntNum; i++) {
        if ((pNameList->namelist[i]) == nullptr) {
            continue;
        }
        std::string dest = path + '/' + std::string((pNameList->namelist[i])->d_name);
        if ((pNameList->namelist[i])->d_type == DT_LNK) {
            continue;
        }
        if ((pNameList->namelist[i])->d_type == DT_DIR) {
            uint64_t subSize = 0;
            auto err = GetDirSize(dest, subSize);
            if (err != E_OK) {
                LOGE("GetDirSize failed");
                return err;
            }
            size += subSize;
        } else {
            struct stat st {};
            if (stat(dest.c_str(), &st) == -1) {
                return errno;
            }
            size += static_cast<uint64_t>(st.st_size);
        }
    }
    return E_OK;
}

int32_t FileSizeUtils::IsFile(const std::string &path, bool &result)
{
    struct stat buf {};
    int ret = stat(path.c_str(), &buf);
    if (ret == -1) {
        return errno;
    }
    result = (buf.st_mode & S_IFMT) == S_IFREG;
    return E_OK;
}

int32_t FileSizeUtils::IsDirectory(const std::string &path, bool &result)
{
    struct stat buf {};
    int ret = stat(path.c_str(), &buf);
    if (ret == -1) {
        return errno;
    }
    result = (buf.st_mode & S_IFMT) == S_IFDIR;
    return E_OK;
}

std::string FileSizeUtils::GetPathFromUri(const std::string &uri, bool isSrcUri)
{
    std::string path;
    FileUri fileUri(uri);
    if (isSrcUri) {
        path = GetRealPath(fileUri.GetRealPath());
    } else {
        path = GetRealPath(fileUri.GetPath());
    }
    return path;
}

std::string FileSizeUtils::GetRealPath(const std::string &path)
{
    std::filesystem::path tempPath(path);
    std::filesystem::path realPath{};
    for (const auto& component : tempPath) {
        if (component == ".") {
            continue;
        } else if (component == "..") {
            realPath = realPath.parent_path();
        } else {
            realPath /= component;
        }
    }
    return realPath.string();
}

std::string FileSizeUtils::GetRealUri(const std::string &uri)
{
    std::string realUri = uri;
    auto pos = uri.find(NETWORK_ID);
    if (pos != std::string::npos) {
        realUri = uri.substr(0, pos);
    }
    return realUri;
}

bool FileSizeUtils::IsFilePathValid(const std::string &filePath)
{
    size_t pos = filePath.find(PATH_INVALID_FLAG1);
    while (pos != std::string::npos) {
        if (pos == 0 || filePath[pos - 1] == FILE_SEPARATOR_CHAR) {
            LOGE("Relative path is not allowed, path contain ../");
            return false;
        }
        pos = filePath.find(PATH_INVALID_FLAG1, pos + PATH_INVALID_FLAG_LEN);
    }
    pos = filePath.rfind(PATH_INVALID_FLAG2);
    if ((pos != std::string::npos) && (filePath.size() - pos == PATH_INVALID_FLAG_LEN)) {
        LOGE("Relative path is not allowed, path tail is /..");
        return false;
    }
    return true;
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
