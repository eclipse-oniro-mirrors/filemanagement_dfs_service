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

#include "decompress_gains_reporter.h"

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <securec.h>
#include <sstream>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>

#include "dfs_error.h"
#include "hisysevent.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement {
namespace Decompress {

#define DECOMPRESS_SYS_EVENT(eventName, type, ...) \
    HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::FILEMANAGEMENT, eventName, type, ##__VA_ARGS__)

namespace {
constexpr const char *USERDATA_PARTITION_PATH = "/data";
constexpr size_t MAX_PATH_LENGTH = PATH_MAX;
constexpr const char *FAKE_DECOMPRESS_DIR_PATH = "/data/service/el1/public/cloudfile/fake_decompress";
constexpr const char *FAKE_DECOMPRESS_GAINS_FILE_NAME = "fakeDecompressGains";
constexpr mode_t FAKE_DECOMPRESS_DIR_MODE = 0770;
constexpr mode_t FAKE_DECOMPRESS_FILE_MODE = 0660;
constexpr int32_t GAINS_REPORT_THRESHOLD = 20;
constexpr uid_t OID_DFS = 1009;
constexpr int DECIMAL_BASE = 10;

std::string GetGainsFilePath()
{
    return std::string(FAKE_DECOMPRESS_DIR_PATH) + "/" + FAKE_DECOMPRESS_GAINS_FILE_NAME;
}
} // namespace

static uint64_t CalcDecompressGains(const std::string &targetPath, uint32_t size)
{
    if (targetPath.empty() || targetPath.length() > MAX_PATH_LENGTH) {
        LOGE("Target path is empty or too long: %{public}zu", targetPath.length());
        return 0;
    }
    struct stat st = {};
    if (stat(targetPath.c_str(), &st) != 0) {
        LOGE("stat failed for %{public}s, errno: %{public}d", targetPath.c_str(), errno);
        return 0;
    }
    constexpr uint64_t statBlockSize = 512;
    uint64_t actualSize = static_cast<uint64_t>(st.st_blocks) * statBlockSize;
    uint64_t logicalSize = static_cast<uint64_t>(size);
    if (logicalSize > actualSize) {
        return logicalSize - actualSize;
    }
    return 0;
}

static std::string GetFileSystemType(const std::string &path)
{
    if (path.empty() || path.length() > MAX_PATH_LENGTH) {
        LOGE("Path is empty or too long: %{public}zu", path.length());
        return "";
    }
    struct statfs fsInfo = {};
    if (statfs(path.c_str(), &fsInfo) != 0) {
        LOGE("statfs failed for %{public}s, errno: %{public}d", path.c_str(), errno);
        return "";
    }
    unsigned long fsType = static_cast<unsigned long>(fsInfo.f_type);
    if (fsType == 0xF2F52010) { // HMFS_SUPER_MAGIC
        return "hmfs";
    } else if (fsType == 0xE0F5E1E2) { // EROFS_SUPER_MAGIC_V1
        return "erofs";
    }
    char buf[32] = {0};
    if (snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, "0x%lx", fsType) < 0) {
        LOGE("snprintf_s fs type failed, errno: %{public}d", errno);
        return "";
    }
    return std::string(buf);
}

static void GetUserdataSpace(uint64_t &totalBytes, uint64_t &leftBytes)
{
    struct statfs fsInfo = {};
    if (statfs(USERDATA_PARTITION_PATH, &fsInfo) != 0) {
        LOGE("statfs failed for %{public}s, errno: %{public}d", USERDATA_PARTITION_PATH, errno);
        return;
    }
    uint64_t frsize = static_cast<uint64_t>(fsInfo.f_frsize);
    if (frsize == 0) {
        frsize = static_cast<uint64_t>(fsInfo.f_bsize);
    }
    if (frsize == 0) {
        LOGE("invalid block size for %{public}s", USERDATA_PARTITION_PATH);
        return;
    }
    totalBytes = static_cast<uint64_t>(fsInfo.f_blocks) * frsize;
    leftBytes = static_cast<uint64_t>(fsInfo.f_bavail) * frsize;
}

static bool WriteAll(int fd, const std::string &content)
{
    const char *data = content.data();
    size_t remaining = content.size();
    while (remaining > 0) {
        ssize_t n = write(fd, data, remaining);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        data += n;
        remaining -= static_cast<size_t>(n);
    }
    return true;
}

static int OpenGainsFileWithLock()
{
    if (access(FAKE_DECOMPRESS_DIR_PATH, F_OK) != 0) {
        if (mkdir(FAKE_DECOMPRESS_DIR_PATH, FAKE_DECOMPRESS_DIR_MODE) != 0 && errno != EEXIST) {
            LOGE("mkdir %{private}s failed, errno: %{public}d", FAKE_DECOMPRESS_DIR_PATH, errno);
            return -1;
        }
    }
    if (chown(FAKE_DECOMPRESS_DIR_PATH, OID_DFS, OID_DFS) != 0) {
        LOGE("chown gains dir to OID_DFS failed, errno: %{public}d", errno);
    }
    std::string gainsFilePath = GetGainsFilePath();
    int fd = open(gainsFilePath.c_str(), O_RDWR | O_CREAT, FAKE_DECOMPRESS_FILE_MODE);
    if (fd < 0) {
        LOGE("OpenGainsFile (r/w) failed, errno: %{public}d", errno);
        return -1;
    }
    if (fchown(fd, OID_DFS, OID_DFS) != 0) {
        LOGE("fchown gains file to OID_DFS failed, errno: %{public}d", errno);
    }
    if (flock(fd, LOCK_EX) != 0) {
        LOGE("flock gains file failed, errno: %{public}d", errno);
        close(fd);
        return -1;
    }
    return fd;
}

static std::string ReadGainsContent(int fd)
{
    std::string content;
    char buf[4096];
    ssize_t n = 0;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        content.append(buf, static_cast<size_t>(n));
    }
    return content;
}

static bool WriteAndTruncate(int fd, const std::string &content)
{
    if (lseek(fd, 0, SEEK_SET) < 0) {
        LOGE("lseek gains file failed, errno: %{public}d", errno);
        return false;
    }
    if (!WriteAll(fd, content)) {
        LOGE("write gains failed, errno: %{public}d", errno);
        return false;
    }
    if (ftruncate(fd, content.size()) != 0) {
        LOGE("ftruncate gains file failed, errno: %{public}d", errno);
        return false;
    }
    return true;
}

static std::string BuildGainsRecord(const FakeDecompressGainsInfo &info)
{
    std::string record;
    record += "bundleName=" + info.bundleName + "\n";
    record += "isSystemApp=" + std::to_string(info.isSystemApp ? 1 : 0) + "\n";
    record += "successSoCount=" + std::to_string(info.successSoCount) + "\n";
    record += "successSoGains=" + std::to_string(info.successSoGains) + "\n";
    record += "failedSoCount=" + std::to_string(info.failedSoCount) + "\n";
    record += "successResfileCount=" + std::to_string(info.successResfileCount) + "\n";
    record += "successResfileGains=" + std::to_string(info.successResfileGains) + "\n";
    record += "failedResfileCount=" + std::to_string(info.failedResfileCount) + "\n";
    record += "srcFileSystem=" + info.srcFileSystem + "\n";
    record += "userdataSpace=" + std::to_string(info.userdataSpace) + "\n";
    record += "userdataSpaceLeft=" + std::to_string(info.userdataSpaceLeft) + "\n";
    record += "\n";
    return record;
}

static void CollectGainsInfo(const FakeDecompressGainsInfo &info, FakeDecompressGainsReport &report)
{
    report.userdataSpace = info.userdataSpace;
    report.userdataSpaceLeft = info.userdataSpaceLeft;
    report.bundleNames.push_back(info.bundleName);
    report.isSystemApps.push_back(info.isSystemApp);
    report.successSoCounts.push_back(info.successSoCount);
    report.successSoGains.push_back(info.successSoGains);
    report.failedSoCounts.push_back(info.failedSoCount);
    report.successResfileCounts.push_back(info.successResfileCount);
    report.successResfileGains.push_back(info.successResfileGains);
    report.failedResfileCounts.push_back(info.failedResfileCount);
    report.srcFileSystems.push_back(info.srcFileSystem);
}

static void ParseGainsInfoLine(const std::string &line, FakeDecompressGainsInfo &info, bool &hasData)
{
    size_t eq = line.find('=');
    if (eq == std::string::npos) {
        return;
    }
    std::string key = line.substr(0, eq);
    std::string val = line.substr(eq + 1);
    if (key == "bundleName") {
        info.bundleName = val;
        hasData = true;
    } else if (key == "isSystemApp") {
        info.isSystemApp = (val == "1");
    } else if (key == "successSoCount") {
        info.successSoCount = static_cast<uint32_t>(strtoul(val.c_str(), nullptr, DECIMAL_BASE));
    } else if (key == "successSoGains") {
        info.successSoGains = static_cast<uint64_t>(strtoull(val.c_str(), nullptr, DECIMAL_BASE));
    } else if (key == "failedSoCount") {
        info.failedSoCount = static_cast<uint32_t>(strtoul(val.c_str(), nullptr, DECIMAL_BASE));
    } else if (key == "successResfileCount") {
        info.successResfileCount = static_cast<uint32_t>(strtoul(val.c_str(), nullptr, DECIMAL_BASE));
    } else if (key == "successResfileGains") {
        info.successResfileGains = static_cast<uint64_t>(strtoull(val.c_str(), nullptr, DECIMAL_BASE));
    } else if (key == "failedResfileCount") {
        info.failedResfileCount = static_cast<uint32_t>(strtoul(val.c_str(), nullptr, DECIMAL_BASE));
    } else if (key == "srcFileSystem") {
        info.srcFileSystem = val;
    } else if (key == "userdataSpace") {
        info.userdataSpace = static_cast<uint64_t>(strtoull(val.c_str(), nullptr, DECIMAL_BASE));
    } else if (key == "userdataSpaceLeft") {
        info.userdataSpaceLeft = static_cast<uint64_t>(strtoull(val.c_str(), nullptr, DECIMAL_BASE));
    }
}

static FakeDecompressGainsReport BuildGainsReport(const std::string &content)
{
    FakeDecompressGainsReport report;
    std::istringstream stream(content);
    std::string line;
    FakeDecompressGainsInfo info;
    bool hasData = false;
    while (std::getline(stream, line)) {
        if (line.empty()) {
            if (hasData) {
                CollectGainsInfo(info, report);
                info = FakeDecompressGainsInfo();
                hasData = false;
            }
            continue;
        }
        ParseGainsInfoLine(line, info, hasData);
    }
    if (hasData) {
        CollectGainsInfo(info, report);
    }
    return report;
}

static std::string GetProcessName()
{
    char buf[256] = {0};
    FILE *fp = fopen("/proc/self/comm", "r");
    if (fp == nullptr) {
        return "";
    }
    if (fgets(buf, sizeof(buf), fp) == nullptr) {
        if (fclose(fp) != 0) {
            LOGE("Failed to close proc comm file, errno: %{public}d", errno);
        }
        return "";
    }
    if (fclose(fp) != 0) {
        LOGE("Failed to close proc comm file, errno: %{public}d", errno);
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }
    return std::string(buf);
}

DecompressFaultReporter &DecompressFaultReporter::GetInstance()
{
    static DecompressFaultReporter instance;
    return instance;
}

void DecompressFaultReporter::ReportFsCommonInfo(const DecompressErrInfo &info)
{
    int32_t pid = static_cast<int32_t>(getpid());
    int32_t uid = static_cast<int32_t>(getuid());
    std::string processName = GetProcessName();
    WriteFsCommonEvent(pid, uid, processName, info);
}

void DecompressFaultReporter::WriteFsCommonEvent(int32_t pid,
                                                 int32_t uid,
                                                 const std::string &processName,
                                                 const DecompressErrInfo &info)
{
    int32_t ret = DECOMPRESS_SYS_EVENT("FS_COMMON_INFO", HiviewDFX::HiSysEvent::EventType::FAULT, "PID", pid, "UID",
                                       uid, "PROCESS_NAME", processName, "MODULE", "dfs_service", "FEATURE",
                                       "fake_decompress", "ERRCODE", info.errCode, "ERRMSG", info.errMsg, "BUNDLE_NAME",
                                       info.bundleName, "FAILED_FILE_NAME", info.failedFileName);
    if (ret != E_OK) {
        LOGE("report FS_COMMON_INFO error %{public}d", ret);
    }
}

DecompressGainsReporter &DecompressGainsReporter::GetInstance()
{
    static DecompressGainsReporter instance;
    return instance;
}

static std::string RemoveBundleRecord(const std::string &content, const std::string &bundleName)
{
    std::string result;
    std::istringstream stream(content);
    std::string line;
    std::string currentRecord;
    bool skipCurrent = false;
    while (std::getline(stream, line)) {
        if (line.empty()) {
            if (!skipCurrent && !currentRecord.empty()) {
                result += currentRecord + "\n";
            }
            currentRecord.clear();
            skipCurrent = false;
            continue;
        }
        if (currentRecord.empty() && line.find("bundleName=") == 0) {
            std::string name = line.substr(strlen("bundleName="));
            skipCurrent = (name == bundleName);
        }
        currentRecord += line + "\n";
    }
    if (!skipCurrent && !currentRecord.empty()) {
        result += currentRecord + "\n";
    }
    return result;
}

void DecompressGainsReporter::UpdateGainsInfo(const CreateInnerFileParam &param, bool success)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto &gainsInfo = gainsMap_[param.bundleName];
    gainsInfo.bundleName = param.bundleName;
    gainsInfo.isSystemApp = param.isSystemApp;
    if (gainsInfo.srcFileSystem.empty()) {
        gainsInfo.srcFileSystem = GetFileSystemType(param.sourcePath);
    }
    UpdateSoGains(gainsInfo, success, param);
    GetUserdataSpace(gainsInfo.userdataSpace, gainsInfo.userdataSpaceLeft);
    LOGD(
        "UpdateGainsInfo done, bundle: %{public}s, success: %{public}d, fileType: %{public}d, "
        "successSoCount: %{public}u, successSoGains: %{public}llu, failedSoCount: %{public}u, "
        "successResfileCount: %{public}u, failedResfileCount: %{public}u, gainsMap size: %{public}zu",
        param.bundleName.c_str(), static_cast<int>(success), static_cast<int>(param.fileType), gainsInfo.successSoCount,
        static_cast<unsigned long long>(gainsInfo.successSoGains), gainsInfo.failedSoCount,
        gainsInfo.successResfileCount, gainsInfo.failedResfileCount, gainsMap_.size());
}

void DecompressGainsReporter::UpdateSoGains(FakeDecompressGainsInfo &info,
                                            bool success,
                                            const CreateInnerFileParam &param)
{
    if (param.fileType == FileType::SO_FILE) {
        if (success) {
            uint64_t gains = CalcDecompressGains(param.targetPath, param.size);
            info.successSoCount++;
            info.successSoGains += gains;
        } else {
            info.failedSoCount++;
        }
    } else {
        if (success) {
            uint64_t gains = CalcDecompressGains(param.targetPath, param.size);
            info.successResfileCount++;
            info.successResfileGains += gains;
        } else {
            info.failedResfileCount++;
        }
    }
}

static void SplitGainsContent(const std::string &allContent, int32_t maxCount, std::string &firstN,
                              std::string &remaining)
{
    std::istringstream stream(allContent);
    std::string line;
    std::string currentRecord;
    int32_t recordCount = 0;
    bool hasData = false;
    auto dispatch = [&maxCount, &firstN, &remaining, &recordCount, &currentRecord, &hasData]() {
        recordCount++;
        if (recordCount <= maxCount) {
            firstN += currentRecord + "\n";
        } else {
            remaining += currentRecord + "\n";
        }
        currentRecord.clear();
        hasData = false;
    };
    while (std::getline(stream, line)) {
        if (line.empty()) {
            if (hasData) {
                dispatch();
            }
            continue;
        }
        currentRecord += line + "\n";
        hasData = true;
    }
    if (hasData) {
        dispatch();
    }
}

bool DecompressGainsReporter::WriteOrUpdateGainsRecord(const FakeDecompressGainsInfo &info)
{
    int fd = OpenGainsFileWithLock();
    if (fd < 0) {
        return false;
    }
    std::string updatedContent = ReadGainsContent(fd);
    updatedContent = RemoveBundleRecord(updatedContent, info.bundleName);
    updatedContent += BuildGainsRecord(info);
    if (!WriteAndTruncate(fd, updatedContent)) {
        LOGE("write gains failed, errno: %{public}d", errno);
        flock(fd, LOCK_UN);
        close(fd);
        return false;
    }
    LOGI(
        "WriteOrUpdateGainsRecord done, bundle: %{public}s, successSoGains: %{public}llu, "
        "successResfileGains: %{public}llu, failedSoCount: %{public}u, failedResfileCount: %{public}u",
        info.bundleName.c_str(), static_cast<unsigned long long>(info.successSoGains),
        static_cast<unsigned long long>(info.successResfileGains), info.failedSoCount, info.failedResfileCount);
    flock(fd, LOCK_UN);
    close(fd);
    return true;
}

bool DecompressGainsReporter::DumpAndReport(const std::string &bundleName)
{
    if (bundleName.empty()) {
        LOGE("bundleName is empty");
        return false;
    }
    LOGI("DumpAndReport enter, bundleName: %{public}s", bundleName.c_str());
    FakeDecompressGainsInfo info;
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = gainsMap_.find(bundleName);
        if (it != gainsMap_.end()) {
            info = it->second;
            gainsMap_.erase(it);
            found = true;
        }
    }
    if (found) {
        if (!WriteOrUpdateGainsRecord(info)) {
            LOGE("WriteOrUpdateGainsRecord failed for bundle: %{public}s", bundleName.c_str());
        }
    } else {
        LOGD("DumpAndReport: bundle %{public}s not found in gainsMap", bundleName.c_str());
    }
    Report(true);
    return true;
}

bool DecompressGainsReporter::Report(bool requireThreshold)
{
    const char *tag = requireThreshold ? "threshold report" : "timed report";
    int fd = open(GetGainsFilePath().c_str(), O_RDWR);
    if (fd < 0) {
        LOGD("%{public}s: gains file not found, skip", tag);
        return false;
    }
    if (flock(fd, LOCK_EX) != 0) {
        LOGE("%{public}s: flock failed, errno: %{public}d", tag, errno);
        close(fd);
        return false;
    }
    std::string content = ReadGainsContent(fd);
    int32_t count = CountGainsRecords(content);
    if (count <= 0) {
        LOGD("%{public}s: no gains data, skip", tag);
        flock(fd, LOCK_UN);
        close(fd);
        return false;
    }
    if (requireThreshold && count < GAINS_REPORT_THRESHOLD) {
        LOGD("%{public}s: count %{public}d < threshold %{public}d, skip", tag, count, GAINS_REPORT_THRESHOLD);
        flock(fd, LOCK_UN);
        close(fd);
        return false;
    }
    LOGI("%{public}s start, count: %{public}d", tag, count);
    std::string firstN;
    std::string remaining;
    SplitGainsContent(content, GAINS_REPORT_THRESHOLD, firstN, remaining);
    if (!WriteAndTruncate(fd, remaining)) {
        LOGE("%{public}s: write remaining gains failed", tag);
        flock(fd, LOCK_UN);
        close(fd);
        return false;
    }
    flock(fd, LOCK_UN);
    close(fd);
    ParseAndReport(firstN);
    LOGI("%{public}s finished, count: %{public}d", tag, count);
    return true;
}

int32_t DecompressGainsReporter::CountGainsRecords(const std::string &content)
{
    int32_t count = 0;
    size_t pos = 0;
    while ((pos = content.find("bundleName=", pos)) != std::string::npos) {
        count++;
        pos++;
    }
    return count;
}

void DecompressGainsReporter::ParseAndReport(const std::string &content)
{
    FakeDecompressGainsReport report = BuildGainsReport(content);
    if (report.bundleNames.empty()) {
        LOGE("report failed: no valid bundle names parsed from gains file");
        return;
    }
    ReportGainsArray(report);
}

void DecompressGainsReporter::ReportGainsArray(const FakeDecompressGainsReport &report)
{
    uint64_t totalSoGains = 0;
    uint64_t totalResfileGains = 0;
    for (size_t i = 0; i < report.successSoGains.size(); i++) {
        totalSoGains += report.successSoGains[i];
    }
    for (size_t i = 0; i < report.successResfileGains.size(); i++) {
        totalResfileGains += report.successResfileGains[i];
    }
    LOGI("ReportGainsArray: bundle count: %{public}zu, totalSoGains: %{public}llu, totalResfileGains: %{public}llu",
         report.bundleNames.size(), static_cast<unsigned long long>(totalSoGains),
         static_cast<unsigned long long>(totalResfileGains));
    int32_t ret = DECOMPRESS_SYS_EVENT(
        "FAKEDECOMPRESS", HiviewDFX::HiSysEvent::EventType::STATISTIC, "BUNDLE_NAME", report.bundleNames,
        "IS_SYSTEM_APP", report.isSystemApps, "SUCCESS_SO_COUNT", report.successSoCounts, "FAIL_SO_COUNT",
        report.failedSoCounts, "SO_GAINS", report.successSoGains, "SUCCESS_RESFILE_COUNT", report.successResfileCounts,
        "FAIL_RESFILE_COUNT", report.failedResfileCounts, "RESFILE_GAINS", report.successResfileGains,
        "SRC_FILE_SYSTEM", report.srcFileSystems, "USERDATA_SPACE", report.userdataSpace, "USERDATA_SPACE_LEFT",
        report.userdataSpaceLeft);
    if (ret != E_OK) {
        LOGE("report FAKEDECOMPRESS error %{public}d", ret);
    } else {
        LOGI("ReportGainsArray: FAKEDECOMPRESS event reported successfully, bundle count: %{public}zu",
             report.bundleNames.size());
    }
}

} // namespace Decompress
} // namespace FileManagement
} // namespace OHOS
