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

#ifndef OHOS_FILEMGMT_DECOMPRESS_GAINS_REPORTER_H
#define OHOS_FILEMGMT_DECOMPRESS_GAINS_REPORTER_H

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "decompress.h"

namespace OHOS {
namespace FileManagement {
namespace Decompress {

struct FakeDecompressGainsInfo {
    std::string bundleName;
    bool isSystemApp = false;
    uint32_t successSoCount = 0; /** Total so count of successful fake decompressions */
    uint64_t successSoGains = 0; /** Total so size of successful fake decompressions */
    uint32_t failedSoCount = 0;  /** Total so count of failed fake decompressions */
    uint32_t successResfileCount = 0;
    uint64_t successResfileGains = 0;
    uint32_t failedResfileCount = 0;
    std::string srcFileSystem;      /** File system of src hap path */
    uint64_t userdataSpace = 0;     /** User data partition total size in Bytes */
    uint64_t userdataSpaceLeft = 0; /** User data partition left size in Bytes */
};

struct FakeDecompressGainsReport {
    std::vector<std::string> bundleNames;
    std::vector<bool> isSystemApps;
    std::vector<uint32_t> successSoCounts;
    std::vector<uint64_t> successSoGains;
    std::vector<uint32_t> failedSoCounts;
    std::vector<uint32_t> successResfileCounts;
    std::vector<uint64_t> successResfileGains;
    std::vector<uint32_t> failedResfileCounts;
    std::vector<std::string> srcFileSystems;
    uint64_t userdataSpace = 0;
    uint64_t userdataSpaceLeft = 0;
};

/** 由调用方提供的字段，PID/UID/PROCESS_NAME由上报函数内部自动采集 */
struct DecompressErrInfo {
    int32_t errCode = 0;
    std::string errMsg;
    std::string bundleName;
    std::string failedFileName;
};

class DecompressFaultReporter {
public:
    static DecompressFaultReporter &GetInstance();
    void ReportFsCommonInfo(const DecompressErrInfo &info);

private:
    DecompressFaultReporter() = default;
    ~DecompressFaultReporter() = default;
    DecompressFaultReporter(const DecompressFaultReporter &) = delete;
    DecompressFaultReporter &operator=(const DecompressFaultReporter &) = delete;

    void WriteFsCommonEvent(int32_t pid, int32_t uid, const std::string &processName, const DecompressErrInfo &info);
};

class DecompressGainsReporter {
public:
    static DecompressGainsReporter &GetInstance();

    void UpdateGainsInfo(const CreateInnerFileParam &param, bool success);
    bool DumpAndReport(const std::string &bundleName);
    bool Report(bool requireThreshold = false);

private:
    DecompressGainsReporter() = default;
    ~DecompressGainsReporter() = default;
    DecompressGainsReporter(const DecompressGainsReporter &) = delete;
    DecompressGainsReporter &operator=(const DecompressGainsReporter &) = delete;

    int32_t CountGainsRecords(const std::string &content);
    void ParseAndReport(const std::string &content);

    void UpdateSoGains(FakeDecompressGainsInfo &info, bool success, const CreateInnerFileParam &param);
    bool WriteOrUpdateGainsRecord(const FakeDecompressGainsInfo &info);
    void ReportGainsArray(const FakeDecompressGainsReport &report);

    std::unordered_map<std::string, FakeDecompressGainsInfo> gainsMap_;
    std::mutex mutex_;
};

} // namespace Decompress
} // namespace FileManagement
} // namespace OHOS

#endif // OHOS_FILEMGMT_DECOMPRESS_GAINS_REPORTER_H
