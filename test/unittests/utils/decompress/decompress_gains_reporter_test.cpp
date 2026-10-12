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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <climits>
#include <fstream>
#include <securec.h>
#include <sys/stat.h>
#include <unistd.h>

#include "decompress_mock.h"
#include "hisysevent.h"

#define DECOMPRESS_UNIT_TEST
#include "decompress_syscall_mock.h"
#undef getline
#include "decompress_gains_reporter.cpp"
#undef open
#undef close
#undef stat
#undef access
#undef mkdir
#undef chown
#undef fchown
#undef flock
#undef lseek
#undef ftruncate
#undef write
#undef realpath
#undef dlopen
#undef dlsym
#undef fopen
#undef fclose
#undef fgets

#include "utils_log.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::FileManagement::Decompress;

namespace {
const std::string TEST_GAINS_DIR = "/data/test_decompress_gains";
const std::string TEST_GAINS_FILE = TEST_GAINS_DIR + "/fakeDecompressGains";
} // namespace

class DecompressGainsReporterTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp()
    {
        DecompressMock::Reset();
        mkdir(TEST_GAINS_DIR.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        mkdir(FAKE_DECOMPRESS_DIR_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        unlink(GetGainsFilePath().c_str());
    }
    void TearDown()
    {
        DecompressMock::Reset();
        unlink(TEST_GAINS_FILE.c_str());
        rmdir(TEST_GAINS_DIR.c_str());
        unlink(GetGainsFilePath().c_str());
    };

protected:
    void WriteGainsFile(const std::string &content)
    {
        std::ofstream ofs(TEST_GAINS_FILE);
        ofs << content;
        ofs.close();
    }
    std::string ReadGainsFile()
    {
        std::ifstream ifs(TEST_GAINS_FILE);
        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        return content;
    }
    void ClearGainsFile()
    {
        std::ofstream ofs(TEST_GAINS_FILE, std::ios::trunc);
        ofs.close();
    }
    void WriteRealGainsFile(const std::string &content)
    {
        std::ofstream ofs(GetGainsFilePath());
        ofs << content;
        ofs.close();
    }
    std::string ReadRealGainsFile()
    {
        std::ifstream ifs(GetGainsFilePath());
        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        return content;
    }
    int32_t CountGainsRecords(const std::string &content)
    {
        int32_t count = 0;
        size_t pos = 0;
        while ((pos = content.find("bundleName=", pos)) != std::string::npos) {
            count++;
            pos++;
        }
        return count;
    }
};

void DecompressGainsReporterTest::SetUpTestCase(void) {}
void DecompressGainsReporterTest::TearDownTestCase(void) {}

// ==================== CalcDecompressGains ====================

HWTEST_F(DecompressGainsReporterTest, CalcDecompressGains_EmptyPath, TestSize.Level1)
{
    EXPECT_EQ(CalcDecompressGains("", 1024), 0u);
}

HWTEST_F(DecompressGainsReporterTest, CalcDecompressGains_StatFail, TestSize.Level1)
{
    DecompressMock::g_mockState.statEnabled = true;
    DecompressMock::g_mockState.statFail = true;
    EXPECT_EQ(CalcDecompressGains("/nonexistent", 1024), 0u);
}

HWTEST_F(DecompressGainsReporterTest, CalcDecompressGains_LogicalLessThanActual, TestSize.Level1)
{
    DecompressMock::g_mockState.statEnabled = true;
    DecompressMock::g_mockState.statFail = false;
    // mock stat memsets st_blocks to 0, so actualSize = 0, gains = size - 0 = size
    EXPECT_EQ(CalcDecompressGains("/data/test", 4096), 4096u);
}

HWTEST_F(DecompressGainsReporterTest, CalcDecompressGains_ZeroSize, TestSize.Level1)
{
    DecompressMock::g_mockState.statEnabled = true;
    DecompressMock::g_mockState.statFail = false;
    EXPECT_EQ(CalcDecompressGains("/data/test", 0), 0u);
}

HWTEST_F(DecompressGainsReporterTest, CalcDecompressGains_PathTooLong, TestSize.Level1)
{
    std::string longPath(MAX_PATH_LENGTH + 1, 'a');
    EXPECT_EQ(CalcDecompressGains(longPath, 1024), 0u);
}

// ==================== BuildGainsRecord ====================

HWTEST_F(DecompressGainsReporterTest, BuildGainsRecord_Format, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    info.bundleName = "com.test.app";
    info.isSystemApp = true;
    info.successSoCount = 3;
    info.successSoGains = 1024;
    info.failedSoCount = 1;
    info.successResfileCount = 5;
    info.successResfileGains = 2048;
    info.failedResfileCount = 0;
    info.srcFileSystem = "0x9fa2";
    info.userdataSpace = 64000000000;
    info.userdataSpaceLeft = 32000000000;

    std::string record = BuildGainsRecord(info);
    EXPECT_NE(record.find("bundleName=com.test.app"), std::string::npos);
    EXPECT_NE(record.find("isSystemApp=1"), std::string::npos);
    EXPECT_NE(record.find("successSoCount=3"), std::string::npos);
    EXPECT_NE(record.find("soGains=1024"), std::string::npos);
    EXPECT_NE(record.find("failedSoCount=1"), std::string::npos);
    EXPECT_NE(record.find("successResfileCount=5"), std::string::npos);
    EXPECT_NE(record.find("resfileGains=2048"), std::string::npos);
    EXPECT_NE(record.find("failedResfileCount=0"), std::string::npos);
    EXPECT_NE(record.find("srcFileSystem=0x9fa2"), std::string::npos);
    EXPECT_NE(record.find("userdataSpace=64000000000"), std::string::npos);
    EXPECT_NE(record.find("userdataSpaceLeft=32000000000"), std::string::npos);
    EXPECT_EQ(record.back(), '\n');
}

HWTEST_F(DecompressGainsReporterTest, BuildGainsRecord_EmptyBundle, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    std::string record = BuildGainsRecord(info);
    EXPECT_NE(record.find("bundleName="), std::string::npos);
    EXPECT_NE(record.find("isSystemApp=0"), std::string::npos);
}

// ==================== CollectGainsInfo ====================

HWTEST_F(DecompressGainsReporterTest, CollectGainsInfo_SingleRecord, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    info.bundleName = "com.app1";
    info.userdataSpace = 100;
    info.userdataSpaceLeft = 50;
    FakeDecompressGainsReport report;
    CollectGainsInfo(info, report);
    EXPECT_EQ(report.bundleNames.size(), 1u);
    EXPECT_EQ(report.userdataSpace, 100u);
    EXPECT_EQ(report.userdataSpaceLeft, 50u);
}

HWTEST_F(DecompressGainsReporterTest, CollectGainsInfo_LastRecordWins, TestSize.Level1)
{
    FakeDecompressGainsInfo info1;
    info1.bundleName = "com.app1";
    info1.userdataSpace = 100;
    info1.userdataSpaceLeft = 50;
    FakeDecompressGainsInfo info2;
    info2.bundleName = "com.app2";
    info2.userdataSpace = 200;
    info2.userdataSpaceLeft = 100;
    FakeDecompressGainsReport report;
    CollectGainsInfo(info1, report);
    CollectGainsInfo(info2, report);
    EXPECT_EQ(report.bundleNames.size(), 2u);
    EXPECT_EQ(report.userdataSpace, 200u);
    EXPECT_EQ(report.userdataSpaceLeft, 100u);
}

// ==================== ParseGainsInfoLine ====================

HWTEST_F(DecompressGainsReporterTest, ParseGainsInfoLine_NoEquals, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    bool hasData = false;
    ParseGainsInfoLine("invalidLine", info, hasData);
    EXPECT_FALSE(hasData);
}

HWTEST_F(DecompressGainsReporterTest, ParseGainsInfoLine_BundleName, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    bool hasData = false;
    ParseGainsInfoLine("bundleName=com.test.app", info, hasData);
    EXPECT_TRUE(hasData);
    EXPECT_EQ(info.bundleName, "com.test.app");
}

HWTEST_F(DecompressGainsReporterTest, ParseGainsInfoLine_IsSystemApp, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    bool hasData = false;
    ParseGainsInfoLine("isSystemApp=1", info, hasData);
    EXPECT_TRUE(info.isSystemApp);
    ParseGainsInfoLine("isSystemApp=0", info, hasData);
    EXPECT_FALSE(info.isSystemApp);
}

HWTEST_F(DecompressGainsReporterTest, ParseGainsInfoLine_SuccessSoCount, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    bool hasData = false;
    ParseGainsInfoLine("successSoCount=42", info, hasData);
    EXPECT_EQ(info.successSoCount, 42u);
}

HWTEST_F(DecompressGainsReporterTest, ParseGainsInfoLine_SuccessSoGains, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    bool hasData = false;
    ParseGainsInfoLine("soGains=999999", info, hasData);
    EXPECT_EQ(info.successSoGains, 999999u);
}

HWTEST_F(DecompressGainsReporterTest, ParseGainsInfoLine_SrcFileSystem, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    bool hasData = false;
    ParseGainsInfoLine("srcFileSystem=0xabcd", info, hasData);
    EXPECT_EQ(info.srcFileSystem, "0xabcd");
}

HWTEST_F(DecompressGainsReporterTest, ParseGainsInfoLine_InvalidNumber_NoCrash, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    bool hasData = false;
    ParseGainsInfoLine("successSoCount=notanumber", info, hasData);
    EXPECT_EQ(info.successSoCount, 0u);
}

HWTEST_F(DecompressGainsReporterTest, ParseGainsInfoLine_UnknownKey, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    bool hasData = false;
    ParseGainsInfoLine("unknownKey=value", info, hasData);
    EXPECT_FALSE(hasData);
    EXPECT_TRUE(info.bundleName.empty());
}

HWTEST_F(DecompressGainsReporterTest, ParseGainsInfoLine_FailedSoCount, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    bool hasData = false;
    ParseGainsInfoLine("failedSoCount=7", info, hasData);
    EXPECT_EQ(info.failedSoCount, 7u);
}

HWTEST_F(DecompressGainsReporterTest, ParseGainsInfoLine_SuccessResfileCount, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    bool hasData = false;
    ParseGainsInfoLine("successResfileCount=4", info, hasData);
    EXPECT_EQ(info.successResfileCount, 4u);
}

HWTEST_F(DecompressGainsReporterTest, ParseGainsInfoLine_FailedResfileCount, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    bool hasData = false;
    ParseGainsInfoLine("failedResfileCount=2", info, hasData);
    EXPECT_EQ(info.failedResfileCount, 2u);
}

HWTEST_F(DecompressGainsReporterTest, ParseGainsInfoLine_ResfileGains, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    bool hasData = false;
    ParseGainsInfoLine("resfileGains=888888", info, hasData);
    EXPECT_EQ(info.successResfileGains, 888888u);
}

// ==================== BuildGainsReport ====================

HWTEST_F(DecompressGainsReporterTest, BuildGainsReport_Empty, TestSize.Level1)
{
    FakeDecompressGainsReport report = BuildGainsReport("");
    EXPECT_TRUE(report.bundleNames.empty());
}

HWTEST_F(DecompressGainsReporterTest, BuildGainsReport_SingleRecord, TestSize.Level1)
{
    std::string content = "bundleName=com.app1\nisSystemApp=0\nsuccessSoCount=5\n\n";
    FakeDecompressGainsReport report = BuildGainsReport(content);
    EXPECT_EQ(report.bundleNames.size(), 1u);
    EXPECT_EQ(report.bundleNames[0], "com.app1");
    EXPECT_EQ(report.successSoCounts[0], 5u);
}

HWTEST_F(DecompressGainsReporterTest, BuildGainsReport_MultiRecord, TestSize.Level1)
{
    std::string content;
    content += "bundleName=com.app1\nisSystemApp=0\nsuccessSoCount=3\n\n";
    content += "bundleName=com.app2\nisSystemApp=1\nsuccessSoCount=7\n\n";
    FakeDecompressGainsReport report = BuildGainsReport(content);
    EXPECT_EQ(report.bundleNames.size(), 2u);
    EXPECT_EQ(report.bundleNames[0], "com.app1");
    EXPECT_EQ(report.bundleNames[1], "com.app2");
    EXPECT_EQ(report.successSoCounts[0], 3u);
    EXPECT_EQ(report.successSoCounts[1], 7u);
}

HWTEST_F(DecompressGainsReporterTest, BuildGainsReport_NoTrailingNewline, TestSize.Level1)
{
    std::string content = "bundleName=com.app1\nisSystemApp=0\nsuccessSoCount=5";
    FakeDecompressGainsReport report = BuildGainsReport(content);
    EXPECT_EQ(report.bundleNames.size(), 1u);
    EXPECT_EQ(report.bundleNames[0], "com.app1");
}

HWTEST_F(DecompressGainsReporterTest, BuildGainsReport_LastRecordUserdataWins, TestSize.Level1)
{
    std::string content;
    content += "bundleName=com.app1\nuserdataSpace=100\nuserdataSpaceLeft=50\n\n";
    content += "bundleName=com.app2\nuserdataSpace=200\nuserdataSpaceLeft=150\n\n";
    FakeDecompressGainsReport report = BuildGainsReport(content);
    EXPECT_EQ(report.userdataSpace, 200u);
    EXPECT_EQ(report.userdataSpaceLeft, 150u);
}

// ==================== CountGainsRecords ====================

HWTEST_F(DecompressGainsReporterTest, CountGainsRecords_Empty, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    EXPECT_EQ(reporter.CountGainsRecords(""), 0);
}

HWTEST_F(DecompressGainsReporterTest, CountGainsRecords_Single, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    std::string content = "bundleName=com.app1\nisSystemApp=0\n\n";
    EXPECT_EQ(reporter.CountGainsRecords(content), 1);
}

HWTEST_F(DecompressGainsReporterTest, CountGainsRecords_Multiple, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    std::string content;
    content += "bundleName=com.app1\n\n";
    content += "bundleName=com.app2\n\n";
    content += "bundleName=com.app3\n\n";
    EXPECT_EQ(reporter.CountGainsRecords(content), 3);
}

// ==================== SplitGainsContent ====================

HWTEST_F(DecompressGainsReporterTest, SplitGainsContent_NoTrailingNewline, TestSize.Level1)
{
    std::string content = "bundleName=com.app1\nisSystemApp=0";
    std::string firstN;
    std::string remaining;
    SplitGainsContent(content, 5, firstN, remaining);
    EXPECT_EQ(CountGainsRecords(firstN), 1);
    EXPECT_TRUE(remaining.empty());
}

HWTEST_F(DecompressGainsReporterTest, SplitGainsContent_AboveMaxKeepsRemaining, TestSize.Level1)
{
    std::string content;
    for (int i = 0; i < GAINS_REPORT_THRESHOLD + 2; i++) {
        content += "bundleName=com.app" + std::to_string(i) + "\n\n";
    }
    std::string firstN;
    std::string remaining;
    SplitGainsContent(content, GAINS_REPORT_THRESHOLD, firstN, remaining);
    EXPECT_EQ(CountGainsRecords(firstN), GAINS_REPORT_THRESHOLD);
    EXPECT_EQ(CountGainsRecords(remaining), 2);
}

// ==================== DecompressGainsReporter::UpdateGainsInfo ====================

HWTEST_F(DecompressGainsReporterTest, UpdateGainsInfo_SoFileSuccess, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    DecompressMock::g_mockState.statEnabled = true;
    DecompressMock::g_mockState.statFail = false;
    DecompressMock::g_mockState.statfsEnabled = true;
    DecompressMock::g_mockState.statfsFail = false;
    CreateInnerFileParam param;
    param.sourcePath = "/data/test/source";
    param.targetPath = "/data/test/target";
    param.size = 4096;
    param.bundleName = "com.test.so";
    param.fileType = FileType::SO_FILE;
    reporter.UpdateGainsInfo(param, true);
    auto &gainsInfo = reporter.gainsMap_["com.test.so"];
    EXPECT_EQ(gainsInfo.bundleName, "com.test.so");
    EXPECT_EQ(gainsInfo.successSoCount, 1u);
    EXPECT_EQ(gainsInfo.failedSoCount, 0u);
    EXPECT_EQ(gainsInfo.srcFileSystem, "hmfs");
}

HWTEST_F(DecompressGainsReporterTest, UpdateGainsInfo_SoFileFail, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    CreateInnerFileParam param;
    param.bundleName = "com.test.sofail";
    param.fileType = FileType::SO_FILE;
    reporter.UpdateGainsInfo(param, false);
    auto &gainsInfo = reporter.gainsMap_["com.test.sofail"];
    EXPECT_EQ(gainsInfo.bundleName, "com.test.sofail");
    EXPECT_EQ(gainsInfo.failedSoCount, 1u);
    EXPECT_EQ(gainsInfo.successSoCount, 0u);
}

HWTEST_F(DecompressGainsReporterTest, UpdateGainsInfo_ResfileSuccess, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    DecompressMock::g_mockState.statEnabled = true;
    DecompressMock::g_mockState.statFail = false;
    DecompressMock::g_mockState.statfsEnabled = true;
    DecompressMock::g_mockState.statfsFail = false;
    CreateInnerFileParam param;
    param.sourcePath = "/data/test/source";
    param.targetPath = "/data/test/target";
    param.size = 2048;
    param.bundleName = "com.test.res";
    param.fileType = FileType::RESOURCE_FILE;
    reporter.UpdateGainsInfo(param, true);
    auto &gainsInfo = reporter.gainsMap_["com.test.res"];
    EXPECT_EQ(gainsInfo.bundleName, "com.test.res");
    EXPECT_EQ(gainsInfo.successResfileCount, 1u);
    EXPECT_EQ(gainsInfo.failedResfileCount, 0u);
    EXPECT_EQ(gainsInfo.srcFileSystem, "hmfs");
}

HWTEST_F(DecompressGainsReporterTest, UpdateGainsInfo_ResfileFail, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    CreateInnerFileParam param;
    param.bundleName = "com.test.resfail";
    param.fileType = FileType::RESOURCE_FILE;
    reporter.UpdateGainsInfo(param, false);
    auto &gainsInfo = reporter.gainsMap_["com.test.resfail"];
    EXPECT_EQ(gainsInfo.bundleName, "com.test.resfail");
    EXPECT_EQ(gainsInfo.failedResfileCount, 1u);
    EXPECT_EQ(gainsInfo.successResfileCount, 0u);
}

// ==================== DecompressGainsReporter::DumpAndReport ====================

HWTEST_F(DecompressGainsReporterTest, DumpAndReport_EmptyName, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    EXPECT_FALSE(reporter.DumpAndReport(""));
}

// ==================== RemoveBundleRecord ====================

HWTEST_F(DecompressGainsReporterTest, RemoveBundleRecord_EmptyContent, TestSize.Level1)
{
    EXPECT_TRUE(RemoveBundleRecord("", "com.test.app").empty());
}

HWTEST_F(DecompressGainsReporterTest, RemoveBundleRecord_NoMatch, TestSize.Level1)
{
    std::string content = "bundleName=com.app1\nisSystemApp=0\n\n";
    std::string result = RemoveBundleRecord(content, "com.app2");
    EXPECT_EQ(result, content);
}

HWTEST_F(DecompressGainsReporterTest, RemoveBundleRecord_SingleMatch, TestSize.Level1)
{
    std::string content = "bundleName=com.app1\nisSystemApp=0\n\n";
    std::string result = RemoveBundleRecord(content, "com.app1");
    EXPECT_TRUE(result.empty());
}

HWTEST_F(DecompressGainsReporterTest, RemoveBundleRecord_MultipleRemoveMiddle, TestSize.Level1)
{
    std::string content;
    content += "bundleName=com.app1\n\n";
    content += "bundleName=com.app2\n\n";
    content += "bundleName=com.app3\n\n";
    std::string result = RemoveBundleRecord(content, "com.app2");
    EXPECT_NE(result.find("com.app1"), std::string::npos);
    EXPECT_NE(result.find("com.app3"), std::string::npos);
    EXPECT_EQ(result.find("com.app2"), std::string::npos);
}

HWTEST_F(DecompressGainsReporterTest, RemoveBundleRecord_MultipleRemoveFirst, TestSize.Level1)
{
    std::string content;
    content += "bundleName=com.app1\n\n";
    content += "bundleName=com.app2\n\n";
    content += "bundleName=com.app3\n\n";
    std::string result = RemoveBundleRecord(content, "com.app1");
    EXPECT_EQ(result.find("com.app1"), std::string::npos);
    EXPECT_NE(result.find("com.app2"), std::string::npos);
    EXPECT_NE(result.find("com.app3"), std::string::npos);
}

HWTEST_F(DecompressGainsReporterTest, RemoveBundleRecord_NoTrailingNewline, TestSize.Level1)
{
    std::string content = "bundleName=com.app1\nisSystemApp=0";
    std::string result = RemoveBundleRecord(content, "com.app1");
    EXPECT_TRUE(result.empty());
}

HWTEST_F(DecompressGainsReporterTest, RemoveBundleRecord_TrailingEmptyLineNoMatch, TestSize.Level1)
{
    std::string content = "bundleName=com.app1\nisSystemApp=0\n\n";
    std::string result = RemoveBundleRecord(content, "com.app2");
    EXPECT_NE(result.find("com.app1"), std::string::npos);
}

// ==================== WriteOrUpdateGainsRecord ====================

HWTEST_F(DecompressGainsReporterTest, WriteOrUpdateGainsRecord_NewFile, TestSize.Level1)
{
    FakeDecompressGainsInfo info;
    info.bundleName = "com.test.new";
    info.isSystemApp = false;
    info.successSoCount = 5;
    info.successSoGains = 4096;
    info.failedSoCount = 1;
    info.successResfileCount = 0;
    info.successResfileGains = 0;
    info.failedResfileCount = 0;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    EXPECT_TRUE(reporter.WriteOrUpdateGainsRecord(info));

    std::string content = ReadRealGainsFile();
    EXPECT_NE(content.find("bundleName=com.test.new"), std::string::npos);
    EXPECT_NE(content.find("soGains=4096"), std::string::npos);
}

HWTEST_F(DecompressGainsReporterTest, WriteOrUpdateGainsRecord_UpdateExisting, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    FakeDecompressGainsInfo info1;
    info1.bundleName = "com.test.update";
    info1.successSoGains = 100;
    EXPECT_TRUE(reporter.WriteOrUpdateGainsRecord(info1));

    info1.successSoGains = 500;
    EXPECT_TRUE(reporter.WriteOrUpdateGainsRecord(info1));

    std::string content = ReadRealGainsFile();
    EXPECT_EQ(CountGainsRecords(content), 1);
    EXPECT_NE(content.find("soGains=500"), std::string::npos);
    EXPECT_EQ(content.find("soGains=100"), std::string::npos);
}

HWTEST_F(DecompressGainsReporterTest, WriteOrUpdateGainsRecord_DifferentBundles, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    FakeDecompressGainsInfo info1;
    info1.bundleName = "com.test.a";
    info1.successSoGains = 100;
    EXPECT_TRUE(reporter.WriteOrUpdateGainsRecord(info1));

    FakeDecompressGainsInfo info2;
    info2.bundleName = "com.test.b";
    info2.successSoGains = 200;
    EXPECT_TRUE(reporter.WriteOrUpdateGainsRecord(info2));

    std::string content = ReadRealGainsFile();
    EXPECT_EQ(CountGainsRecords(content), 2);
    EXPECT_NE(content.find("com.test.a"), std::string::npos);
    EXPECT_NE(content.find("com.test.b"), std::string::npos);
}

HWTEST_F(DecompressGainsReporterTest, WriteOrUpdateGainsRecord_OpenFail, TestSize.Level1)
{
    DecompressMock::g_mockState.openEnabled = true;
    DecompressMock::g_mockState.openReturnFd = -1;
    DecompressMock::g_mockState.openErrno = EIO;

    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    FakeDecompressGainsInfo info;
    info.bundleName = "com.test.openfail";
    EXPECT_FALSE(reporter.WriteOrUpdateGainsRecord(info));
}

// ==================== UpdateGainsInfo (cross-process persistence) ====================

HWTEST_F(DecompressGainsReporterTest, UpdateGainsInfo_PersistsToSoFile, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    DecompressMock::g_mockState.statEnabled = true;
    DecompressMock::g_mockState.statFail = false;
    CreateInnerFileParam param;
    param.sourcePath = "/data/test/source";
    param.targetPath = "/data/test/target";
    param.size = 4096;
    param.bundleName = "com.test.persist";
    param.fileType = FileType::SO_FILE;
    reporter.UpdateGainsInfo(param, true);
    reporter.DumpAndReport("com.test.persist");

    std::string content = ReadRealGainsFile();
    EXPECT_NE(content.find("bundleName=com.test.persist"), std::string::npos);
    EXPECT_NE(content.find("successSoCount=1"), std::string::npos);
    EXPECT_NE(content.find("soGains=4096"), std::string::npos);
}

HWTEST_F(DecompressGainsReporterTest, DumpAndReport_WriteOrUpdateFail, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    DecompressMock::g_mockState.statEnabled = true;
    DecompressMock::g_mockState.statFail = false;
    CreateInnerFileParam param;
    param.sourcePath = "/data/test/source";
    param.targetPath = "/data/test/target";
    param.size = 4096;
    param.bundleName = "com.test.writefail";
    param.fileType = FileType::SO_FILE;
    reporter.UpdateGainsInfo(param, true);

    DecompressMock::g_mockState.openEnabled = true;
    DecompressMock::g_mockState.openReturnFd = -1;
    DecompressMock::g_mockState.openErrno = EIO;

    EXPECT_TRUE(reporter.DumpAndReport("com.test.writefail"));
}

HWTEST_F(DecompressGainsReporterTest, UpdateGainsInfo_AggregatesMultipleCalls, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    DecompressMock::g_mockState.statEnabled = true;
    DecompressMock::g_mockState.statFail = false;
    CreateInnerFileParam param;
    param.sourcePath = "/data/test/source";
    param.targetPath = "/data/test/target";
    param.size = 4096;
    param.bundleName = "com.test.agg";
    param.fileType = FileType::SO_FILE;

    reporter.UpdateGainsInfo(param, true);
    reporter.UpdateGainsInfo(param, true);
    reporter.DumpAndReport("com.test.agg");

    std::string content = ReadRealGainsFile();
    EXPECT_EQ(CountGainsRecords(content), 1);
    EXPECT_NE(content.find("successSoCount=2"), std::string::npos);
    EXPECT_NE(content.find("soGains=8192"), std::string::npos);
}

// ==================== DumpAndReport (threshold-based, no gainsMap_ lookup) ====================

HWTEST_F(DecompressGainsReporterTest, DumpAndReport_NoFile, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    EXPECT_TRUE(reporter.DumpAndReport("com.nonexistent.bundle"));
}

HWTEST_F(DecompressGainsReporterTest, DumpAndReport_BelowThreshold, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    std::string content;
    for (int i = 0; i < 4; i++) {
        content += "bundleName=com.test." + std::to_string(i) + "\n\n";
    }
    WriteRealGainsFile(content);

    EXPECT_TRUE(reporter.DumpAndReport("com.test.0"));

    std::string after = ReadRealGainsFile();
    EXPECT_EQ(CountGainsRecords(after), 4);
}

HWTEST_F(DecompressGainsReporterTest, DumpAndReport_AboveThreshold, TestSize.Level1)
{
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    std::string content;
    for (int i = 0; i < GAINS_REPORT_THRESHOLD; i++) {
        content += "bundleName=com.test." + std::to_string(i) + "\n\n";
    }
    WriteRealGainsFile(content);

    EXPECT_TRUE(reporter.DumpAndReport("com.test.0"));

    std::string after = ReadRealGainsFile();
    EXPECT_EQ(CountGainsRecords(after), 0);
}

// ==================== DecompressGainsReporter::Report ====================

HWTEST_F(DecompressGainsReporterTest, Report_NoFile, TestSize.Level1)
{
    ClearGainsFile();
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    EXPECT_FALSE(reporter.Report());
}

HWTEST_F(DecompressGainsReporterTest, Report_NoValidRecords, TestSize.Level1)
{
    WriteRealGainsFile("garbage content without bundleName\n\n");
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    EXPECT_FALSE(reporter.Report());
}

HWTEST_F(DecompressGainsReporterTest, Report_TimedWithData, TestSize.Level1)
{
    std::string content;
    for (int i = 0; i < 3; i++) {
        content += "bundleName=com.test." + std::to_string(i) + "\n\n";
    }
    WriteRealGainsFile(content);

    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    EXPECT_TRUE(reporter.Report());

    std::string after = ReadRealGainsFile();
    EXPECT_EQ(CountGainsRecords(after), 0);
}

HWTEST_F(DecompressGainsReporterTest, Report_TimedMoreThanThreshold, TestSize.Level1)
{
    int total = GAINS_REPORT_THRESHOLD + 5;
    std::string content;
    for (int i = 0; i < total; i++) {
        content += "bundleName=com.test." + std::to_string(i) + "\n\n";
    }
    WriteRealGainsFile(content);

    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    EXPECT_TRUE(reporter.Report());

    std::string after = ReadRealGainsFile();
    EXPECT_EQ(CountGainsRecords(after), total - GAINS_REPORT_THRESHOLD);
}

HWTEST_F(DecompressGainsReporterTest, Report_ThresholdBelowSkips, TestSize.Level1)
{
    std::string content;
    for (int i = 0; i < GAINS_REPORT_THRESHOLD - 1; i++) {
        content += "bundleName=com.test." + std::to_string(i) + "\n\n";
    }
    WriteRealGainsFile(content);

    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    EXPECT_FALSE(reporter.Report(true));

    std::string after = ReadRealGainsFile();
    EXPECT_EQ(CountGainsRecords(after), GAINS_REPORT_THRESHOLD - 1);
}

HWTEST_F(DecompressGainsReporterTest, Report_ThresholdExactTrigger, TestSize.Level1)
{
    std::string content;
    for (int i = 0; i < GAINS_REPORT_THRESHOLD; i++) {
        content += "bundleName=com.test." + std::to_string(i) + "\n\n";
    }
    WriteRealGainsFile(content);

    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    EXPECT_TRUE(reporter.Report(true));

    std::string after = ReadRealGainsFile();
    EXPECT_EQ(CountGainsRecords(after), 0);
}

HWTEST_F(DecompressGainsReporterTest, Report_ThresholdAboveKeepsRemaining, TestSize.Level1)
{
    int total = GAINS_REPORT_THRESHOLD + 3;
    std::string content;
    for (int i = 0; i < total; i++) {
        content += "bundleName=com.test." + std::to_string(i) + "\n\n";
    }
    WriteRealGainsFile(content);

    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    EXPECT_TRUE(reporter.Report(true));

    std::string after = ReadRealGainsFile();
    EXPECT_EQ(CountGainsRecords(after), total - GAINS_REPORT_THRESHOLD);
}

// ==================== DecompressFaultReporter::ReportFsCommonInfo ====================

HWTEST_F(DecompressGainsReporterTest, ReportFsCommonInfo_Normal, TestSize.Level1)
{
    DecompressFaultReporter &reporter = DecompressFaultReporter::GetInstance();
    DecompressMock::g_mockState.fopenEnabled = true;
    DecompressMock::g_mockState.fopenSucceed = true;
    DecompressMock::g_mockState.fopenHandle = reinterpret_cast<FILE *>(0x1234);
    DecompressMock::g_mockState.fgetsEnabled = true;
    DecompressMock::g_mockState.fgetsSucceed = true;
    strncpy_s(DecompressMock::g_mockState.fgetsContent, sizeof(DecompressMock::g_mockState.fgetsContent), "test_proc\n",
              sizeof("test_proc\n"));
    auto &hisysState = OHOS::HiviewDFX::GetHiSysEventMockState();
    hisysState.enabled = true;
    hisysState.ret = 0;
    hisysState.callCount = 0;

    DecompressErrInfo info;
    info.errCode = EIO;
    info.errMsg = "test error";
    info.bundleName = "com.test.app";
    info.failedFileName = "test.so";
    reporter.ReportFsCommonInfo(info);
    EXPECT_EQ(hisysState.callCount, 1);
}

HWTEST_F(DecompressGainsReporterTest, ReportFsCommonInfo_FopenFail, TestSize.Level1)
{
    DecompressFaultReporter &reporter = DecompressFaultReporter::GetInstance();
    DecompressMock::g_mockState.fopenEnabled = true;
    DecompressMock::g_mockState.fopenSucceed = false;
    auto &hisysState = OHOS::HiviewDFX::GetHiSysEventMockState();
    hisysState.enabled = true;
    hisysState.ret = -1;
    hisysState.callCount = 0;

    DecompressErrInfo info;
    info.errCode = EIO;
    info.errMsg = "test error";
    info.bundleName = "com.test.app";
    info.failedFileName = "test.so";
    reporter.ReportFsCommonInfo(info);
    EXPECT_EQ(hisysState.callCount, 1);
}

// ==================== WriteAll ====================

HWTEST_F(DecompressGainsReporterTest, WriteAll_EmptyContent, TestSize.Level1)
{
    int fd = open(TEST_GAINS_FILE.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0660);
    ASSERT_GE(fd, 0);
    EXPECT_TRUE(WriteAll(fd, ""));
    close(fd);
    EXPECT_EQ(ReadGainsFile(), "");
}

HWTEST_F(DecompressGainsReporterTest, WriteAll_Normal, TestSize.Level1)
{
    int fd = open(TEST_GAINS_FILE.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0660);
    ASSERT_GE(fd, 0);
    EXPECT_TRUE(WriteAll(fd, "hello world"));
    close(fd);
    EXPECT_EQ(ReadGainsFile(), "hello world");
}

// ==================== GetProcessName ====================

HWTEST_F(DecompressGainsReporterTest, GetProcessName_FopenFail, TestSize.Level1)
{
    DecompressMock::g_mockState.fopenEnabled = true;
    DecompressMock::g_mockState.fopenSucceed = false;
    EXPECT_EQ(GetProcessName(), "");
}

HWTEST_F(DecompressGainsReporterTest, GetProcessName_Normal, TestSize.Level1)
{
    DecompressMock::g_mockState.fopenEnabled = true;
    DecompressMock::g_mockState.fopenSucceed = true;
    DecompressMock::g_mockState.fopenHandle = reinterpret_cast<FILE *>(0x1234);
    DecompressMock::g_mockState.fgetsEnabled = true;
    DecompressMock::g_mockState.fgetsSucceed = true;
    strncpy_s(DecompressMock::g_mockState.fgetsContent, sizeof(DecompressMock::g_mockState.fgetsContent), "test_proc\n",
              sizeof("test_proc\n"));
    EXPECT_EQ(GetProcessName(), "test_proc");
}

HWTEST_F(DecompressGainsReporterTest, GetProcessName_FgetsFail, TestSize.Level1)
{
    DecompressMock::g_mockState.fopenEnabled = true;
    DecompressMock::g_mockState.fopenSucceed = true;
    DecompressMock::g_mockState.fopenHandle = reinterpret_cast<FILE *>(0x1234);
    DecompressMock::g_mockState.fgetsEnabled = true;
    DecompressMock::g_mockState.fgetsSucceed = false;
    EXPECT_EQ(GetProcessName(), "");
}

// ==================== GetFileSystemType (statfs mock) ====================

HWTEST_F(DecompressGainsReporterTest, GetFileSystemType_Erofs, TestSize.Level1)
{
    DecompressMock::g_mockState.statfsEnabled = true;
    DecompressMock::g_mockState.statfsFail = false;
    DecompressMock::g_mockState.statfsType = 0xE0F5E1E2; // EROFS_SUPER_MAGIC_V1
    CreateInnerFileParam param;
    param.sourcePath = "/data/test/source";
    param.targetPath = "/data/test/target";
    param.size = 4096;
    param.bundleName = "com.test.erofs";
    param.fileType = FileType::SO_FILE;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    reporter.UpdateGainsInfo(param, true);
    auto &gainsInfo = reporter.gainsMap_["com.test.erofs"];
    EXPECT_EQ(gainsInfo.srcFileSystem, "erofs");
    EXPECT_EQ(gainsInfo.successSoCount, 1u);
}

HWTEST_F(DecompressGainsReporterTest, GetFileSystemType_Unknown, TestSize.Level1)
{
    DecompressMock::g_mockState.statfsEnabled = true;
    DecompressMock::g_mockState.statfsFail = false;
    DecompressMock::g_mockState.statfsType = 0xABCD1234; // unknown fs type
    CreateInnerFileParam param;
    param.sourcePath = "/data/test/source";
    param.targetPath = "/data/test/target";
    param.size = 4096;
    param.bundleName = "com.test.unknownfs";
    param.fileType = FileType::SO_FILE;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    reporter.UpdateGainsInfo(param, true);
    auto &gainsInfo = reporter.gainsMap_["com.test.unknownfs"];
    EXPECT_EQ(gainsInfo.srcFileSystem, "0xabcd1234");
    EXPECT_EQ(gainsInfo.successSoCount, 1u);
}

HWTEST_F(DecompressGainsReporterTest, GetFileSystemType_StatfsFail, TestSize.Level1)
{
    DecompressMock::g_mockState.statfsEnabled = true;
    DecompressMock::g_mockState.statfsFail = true;
    CreateInnerFileParam param;
    param.sourcePath = "/data/test/source";
    param.targetPath = "/data/test/target";
    param.size = 4096;
    param.bundleName = "com.test.statfsfail";
    param.fileType = FileType::SO_FILE;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    reporter.UpdateGainsInfo(param, true);
    auto &gainsInfo = reporter.gainsMap_["com.test.statfsfail"];
    EXPECT_EQ(gainsInfo.srcFileSystem, "");
    EXPECT_EQ(gainsInfo.successSoCount, 1u);
}

HWTEST_F(DecompressGainsReporterTest, GetFileSystemType_EmptyPath, TestSize.Level1)
{
    DecompressMock::g_mockState.statfsEnabled = true;
    CreateInnerFileParam param;
    param.sourcePath = "";
    param.targetPath = "/data/test/target";
    param.size = 4096;
    param.bundleName = "com.test.emptypath";
    param.fileType = FileType::SO_FILE;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    reporter.UpdateGainsInfo(param, true);
    auto &gainsInfo = reporter.gainsMap_["com.test.emptypath"];
    EXPECT_EQ(gainsInfo.srcFileSystem, "");
    EXPECT_EQ(gainsInfo.successSoCount, 1u);
}

// ==================== GetUserdataSpace (statfs mock) ====================

HWTEST_F(DecompressGainsReporterTest, GetUserdataSpace_FrsizeFallbackToBsize, TestSize.Level1)
{
    DecompressMock::g_mockState.statfsEnabled = true;
    DecompressMock::g_mockState.statfsFail = false;
    DecompressMock::g_mockState.statfsFrsize = 0;
    DecompressMock::g_mockState.statfsBsize = 4096;
    DecompressMock::g_mockState.statfsBlocks = 1000;
    DecompressMock::g_mockState.statfsBavail = 500;
    CreateInnerFileParam param;
    param.sourcePath = "/data/test/source";
    param.targetPath = "/data/test/target";
    param.size = 4096;
    param.bundleName = "com.test.frsize0";
    param.fileType = FileType::SO_FILE;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    reporter.UpdateGainsInfo(param, true);
    auto &gainsInfo = reporter.gainsMap_["com.test.frsize0"];
    EXPECT_EQ(gainsInfo.userdataSpace, 4096000u);
    EXPECT_EQ(gainsInfo.userdataSpaceLeft, 2048000u);
    EXPECT_EQ(gainsInfo.srcFileSystem, "hmfs");
}

HWTEST_F(DecompressGainsReporterTest, GetUserdataSpace_InvalidBlockSize, TestSize.Level1)
{
    DecompressMock::g_mockState.statfsEnabled = true;
    DecompressMock::g_mockState.statfsFail = false;
    DecompressMock::g_mockState.statfsFrsize = 0;
    DecompressMock::g_mockState.statfsBsize = 0;
    CreateInnerFileParam param;
    param.sourcePath = "/data/test/source";
    param.targetPath = "/data/test/target";
    param.size = 4096;
    param.bundleName = "com.test.bs0";
    param.fileType = FileType::SO_FILE;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    reporter.UpdateGainsInfo(param, true);
    auto &gainsInfo = reporter.gainsMap_["com.test.bs0"];
    EXPECT_EQ(gainsInfo.userdataSpace, 0u);
    EXPECT_EQ(gainsInfo.userdataSpaceLeft, 0u);
    EXPECT_EQ(gainsInfo.srcFileSystem, "hmfs");
}

HWTEST_F(DecompressGainsReporterTest, GetUserdataSpace_StatfsFail, TestSize.Level1)
{
    DecompressMock::g_mockState.statfsEnabled = true;
    DecompressMock::g_mockState.statfsFail = true;
    CreateInnerFileParam param;
    param.sourcePath = "/data/test/source";
    param.targetPath = "/data/test/target";
    param.size = 4096;
    param.bundleName = "com.test.udstatfsfail";
    param.fileType = FileType::SO_FILE;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    reporter.UpdateGainsInfo(param, true);
    auto &gainsInfo = reporter.gainsMap_["com.test.udstatfsfail"];
    EXPECT_EQ(gainsInfo.userdataSpace, 0u);
    EXPECT_EQ(gainsInfo.userdataSpaceLeft, 0u);
    EXPECT_EQ(gainsInfo.srcFileSystem, "");
    EXPECT_EQ(gainsInfo.successSoCount, 1u);
}

// ==================== WriteAll (write mock) ====================

HWTEST_F(DecompressGainsReporterTest, WriteAll_EintrRetry, TestSize.Level1)
{
    int fd = open(TEST_GAINS_FILE.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0660);
    ASSERT_GE(fd, 0);
    DecompressMock::g_mockState.writeEnabled = true;
    DecompressMock::g_mockState.writeIntrBeforeCount = 1;
    DecompressMock::g_mockState.writeFailAfterCount = -1;
    EXPECT_TRUE(WriteAll(fd, "hello"));
    close(fd);
    EXPECT_EQ(ReadGainsFile(), "hello");
}

HWTEST_F(DecompressGainsReporterTest, WriteAll_WriteFail, TestSize.Level1)
{
    int fd = open(TEST_GAINS_FILE.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0660);
    ASSERT_GE(fd, 0);
    DecompressMock::g_mockState.writeEnabled = true;
    DecompressMock::g_mockState.writeIntrBeforeCount = -1;
    DecompressMock::g_mockState.writeFailAfterCount = 0;
    EXPECT_FALSE(WriteAll(fd, "hello"));
    close(fd);
}

// ==================== WriteAndTruncate (lseek/ftruncate mock) ====================

HWTEST_F(DecompressGainsReporterTest, WriteAndTruncate_LseekFail, TestSize.Level1)
{
    int fd = open(TEST_GAINS_FILE.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0660);
    ASSERT_GE(fd, 0);
    DecompressMock::g_mockState.lseekEnabled = true;
    DecompressMock::g_mockState.lseekFail = true;
    EXPECT_FALSE(WriteAndTruncate(fd, "content"));
    close(fd);
}

HWTEST_F(DecompressGainsReporterTest, WriteAndTruncate_FtruncateFail, TestSize.Level1)
{
    int fd = open(TEST_GAINS_FILE.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0660);
    ASSERT_GE(fd, 0);
    DecompressMock::g_mockState.ftruncateEnabled = true;
    DecompressMock::g_mockState.ftruncateFail = true;
    EXPECT_FALSE(WriteAndTruncate(fd, "content"));
    close(fd);
}

// ==================== OpenGainsFileWithLock (access/mkdir/chown/fchown/flock mock) ====================

HWTEST_F(DecompressGainsReporterTest, OpenGainsFileWithLock_MkdirFail, TestSize.Level1)
{
    DecompressMock::g_mockState.accessEnabled = true;
    DecompressMock::g_mockState.accessFail = true;
    DecompressMock::g_mockState.mkdirEnabled = true;
    DecompressMock::g_mockState.mkdirFail = true;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    FakeDecompressGainsInfo info;
    info.bundleName = "com.test.mkdirfail";
    EXPECT_FALSE(reporter.WriteOrUpdateGainsRecord(info));
}

HWTEST_F(DecompressGainsReporterTest, OpenGainsFileWithLock_MkdirSuccess, TestSize.Level1)
{
    DecompressMock::g_mockState.accessEnabled = true;
    DecompressMock::g_mockState.accessFail = true;
    DecompressMock::g_mockState.mkdirEnabled = true;
    DecompressMock::g_mockState.mkdirFail = false;
    DecompressMock::g_mockState.openEnabled = true;
    DecompressMock::g_mockState.openReturnFd = -1;
    DecompressMock::g_mockState.openErrno = EIO;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    FakeDecompressGainsInfo info;
    info.bundleName = "com.test.mkdirok";
    EXPECT_FALSE(reporter.WriteOrUpdateGainsRecord(info));
}

HWTEST_F(DecompressGainsReporterTest, OpenGainsFileWithLock_ChownFail, TestSize.Level1)
{
    DecompressMock::g_mockState.chownEnabled = true;
    DecompressMock::g_mockState.chownFail = true;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    FakeDecompressGainsInfo info;
    info.bundleName = "com.test.chownfail";
    /* chown fails but execution continues; open succeeds -> record written */
    EXPECT_TRUE(reporter.WriteOrUpdateGainsRecord(info));
}

HWTEST_F(DecompressGainsReporterTest, OpenGainsFileWithLock_FchownFail, TestSize.Level1)
{
    DecompressMock::g_mockState.fchownEnabled = true;
    DecompressMock::g_mockState.fchownFail = true;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    FakeDecompressGainsInfo info;
    info.bundleName = "com.test.fchownfail";
    /* fchown fails but execution continues; record written */
    EXPECT_TRUE(reporter.WriteOrUpdateGainsRecord(info));
}

HWTEST_F(DecompressGainsReporterTest, OpenGainsFileWithLock_FlockFail, TestSize.Level1)
{
    DecompressMock::g_mockState.flockEnabled = true;
    DecompressMock::g_mockState.flockFail = true;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    FakeDecompressGainsInfo info;
    info.bundleName = "com.test.flockfail";
    EXPECT_FALSE(reporter.WriteOrUpdateGainsRecord(info));
}

// ==================== Report (flock/WriteAndTruncate mock) ====================

HWTEST_F(DecompressGainsReporterTest, Report_FlockFail, TestSize.Level1)
{
    std::string content;
    for (int i = 0; i < 3; i++) {
        content += "bundleName=com.test." + std::to_string(i) + "\n\n";
    }
    WriteRealGainsFile(content);

    DecompressMock::g_mockState.flockEnabled = true;
    DecompressMock::g_mockState.flockFail = true;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    EXPECT_FALSE(reporter.Report());
}

HWTEST_F(DecompressGainsReporterTest, Report_WriteAndTruncateFail, TestSize.Level1)
{
    std::string content;
    for (int i = 0; i < GAINS_REPORT_THRESHOLD + 3; i++) {
        content += "bundleName=com.test." + std::to_string(i) + "\n\n";
    }
    WriteRealGainsFile(content);

    DecompressMock::g_mockState.writeEnabled = true;
    DecompressMock::g_mockState.writeIntrBeforeCount = -1;
    DecompressMock::g_mockState.writeFailAfterCount = 0;
    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    EXPECT_FALSE(reporter.Report(true));
}

// ==================== WriteFsCommonInfo (HiSysEvent mock) ====================

HWTEST_F(DecompressGainsReporterTest, WriteFsCommonInfo_HiSysEventFail, TestSize.Level1)
{
    auto &state = OHOS::HiviewDFX::GetHiSysEventMockState();
    state.enabled = true;
    state.ret = -1;

    DecompressFaultReporter &reporter = DecompressFaultReporter::GetInstance();
    DecompressErrInfo info;
    info.errCode = EIO;
    info.errMsg = "test error";
    info.bundleName = "com.test.app";
    info.failedFileName = "test.so";
    int32_t before =  OHOS::HiviewDFX::GetHiSysEventMockState().callCount;
    reporter.ReportFsCommonInfo(info);
    int32_t after =  OHOS::HiviewDFX::GetHiSysEventMockState().callCount;
    EXPECT_EQ(after - before, 1);
}

// ==================== ReportGainsArray (HiSysEvent mock) ====================

HWTEST_F(DecompressGainsReporterTest, ReportGainsArray_HiSysEventFail, TestSize.Level1)
{
    std::string content;
    for (int i = 0; i < 3; i++) {
        content += "bundleName=com.test." + std::to_string(i) + "\n\n";
    }
    WriteRealGainsFile(content);

    auto &state = OHOS::HiviewDFX::GetHiSysEventMockState();
    state.enabled = true;
    state.ret = -1;

    DecompressGainsReporter &reporter = DecompressGainsReporter::GetInstance();
    EXPECT_TRUE(reporter.Report());
}

// ==================== GetProcessName (fclose mock) ====================

HWTEST_F(DecompressGainsReporterTest, GetProcessName_FcloseFailAfterFgetsSuccess, TestSize.Level1)
{
    DecompressMock::g_mockState.fopenEnabled = true;
    DecompressMock::g_mockState.fopenSucceed = true;
    DecompressMock::g_mockState.fopenHandle = reinterpret_cast<FILE *>(0x1234);
    DecompressMock::g_mockState.fgetsEnabled = true;
    DecompressMock::g_mockState.fgetsSucceed = true;
    DecompressMock::g_mockState.fcloseFail = true;
    strncpy_s(DecompressMock::g_mockState.fgetsContent, sizeof(DecompressMock::g_mockState.fgetsContent), "test_proc\n",
              sizeof("test_proc\n"));
    EXPECT_EQ(GetProcessName(), "test_proc");
}

HWTEST_F(DecompressGainsReporterTest, GetProcessName_FcloseFailAfterFgetsFail, TestSize.Level1)
{
    DecompressMock::g_mockState.fopenEnabled = true;
    DecompressMock::g_mockState.fopenSucceed = true;
    DecompressMock::g_mockState.fopenHandle = reinterpret_cast<FILE *>(0x1234);
    DecompressMock::g_mockState.fgetsEnabled = true;
    DecompressMock::g_mockState.fgetsSucceed = false;
    DecompressMock::g_mockState.fcloseFail = true;
    EXPECT_EQ(GetProcessName(), "");
}
