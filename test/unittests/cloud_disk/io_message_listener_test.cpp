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

#include "io_message_listener.h"

#include <cstring>
#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include "assistant.h"
#include "application_state_observer_stub.h"
#include "dfs_error.h"
#include "utils_log.h"

namespace OHOS::FileManagement::CloudDisk::Test {
using namespace testing;
using namespace testing::ext;
using namespace std;

const int64_t ZERO = 0;
const int64_t ONE = 1;
const int32_t ABOVE_DATA = 2000;
const int32_t THREAD_SLEEP_TIME = 100;
const int32_t FRONT_EVENT = 2;
const int32_t BACKGROUND_EVENT = 4;
const int32_t UNKNOWN_EVENT = 8;

class IoMessageListenerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static inline shared_ptr<IoMessageManager> ioMessageManager_ = nullptr;
    static inline shared_ptr<AssistantMock> insMock = nullptr;
};

void IoMessageListenerTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
    ioMessageManager_ = make_shared<IoMessageManager>();
    insMock = make_shared<AssistantMock>();
    Assistant::ins = insMock;
}

void IoMessageListenerTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
    ioMessageManager_ = nullptr;
    Assistant::ins = nullptr;
    insMock = nullptr;
}

void IoMessageListenerTest::SetUp(void)
{
    GTEST_LOG_(INFO) << "SetUp";
}

void IoMessageListenerTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "TearDown";
}

/**
 * @tc.name: ReadIoDataFromFileTest001
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest001 Start";
    try {
        string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest001 ERROR";
    }
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest001 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest002
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest002 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_TRUE(ret);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest002 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest002 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest003
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest003 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:a\n";
        write(fd, line1.c_str(), line1.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest003 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest003 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest004
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest004 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:a\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest004 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest004 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest005
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest005 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:1\n";
        string line5 = "syscstat:1\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        write(fd, line5.c_str(), line5.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_TRUE(ret);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest005 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest005 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest006
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest006 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:a\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest006 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest006 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest007
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest007, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest007 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:a\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest007 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest007 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest008
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest008, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest008 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:1\n";
        string line5 = "syscstat:a\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        write(fd, line5.c_str(), line5.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest008 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest008 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest009
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest009, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest009 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:99999999999999999999\n";
        write(fd, line1.c_str(), line1.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest009 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest009 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest010
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest010, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest010 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:99999999999999999999\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest010 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest010 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest011
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest011, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest011 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:99999999999999999999\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest011 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest011 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest012
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest012, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest012 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:99999999999999999999\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest012 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest012 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest013
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest013, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest013 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:1\n";
        string line5 = "syscstat:99999999999999999999\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        write(fd, line5.c_str(), line5.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest013 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest013 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest014
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest014, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest014 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        write(fd, line1.c_str(), line1.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_TRUE(ret);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest014 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest014 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest015
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest015, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest015 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_TRUE(ret);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest015 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest015 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest016
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest016, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest016 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:1\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_TRUE(ret);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest016 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest016 End";
}

/**
 * @tc.name: ReadIoDataFromFileTest017
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ReadIoDataFromFileTest017, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest017 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        close(fd);
        bool ret = ioMessageManager_->ReadIoDataFromFile(path);
        EXPECT_TRUE(ret);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ReadIoDataFromFileTest017 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ReadIoDataFromFileTest017 End";
}

/**
 * @tc.name: IsFirstLineHeaderTest001
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, IsFirstLineHeaderTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsFirstLineHeaderTest001 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        bool ret = ioMessageManager_->IsFirstLineHeader(path);
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "IsFirstLineHeaderTest001 ERROR";
    }
    GTEST_LOG_(INFO) << "IsFirstLineHeaderTest001 End";
}

/**
 * @tc.name: IsFirstLineHeaderTest002
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, IsFirstLineHeaderTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsFirstLineHeaderTest002 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        close(fd);
        bool ret = ioMessageManager_->IsFirstLineHeader(path);
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "IsFirstLineHeaderTest002 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "IsFirstLineHeaderTest002 End";
}

/**
 * @tc.name: IsFirstLineHeaderTest003
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, IsFirstLineHeaderTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsFirstLineHeaderTest003 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "time\n";
        write(fd, line1.c_str(), line1.size());
        close(fd);
        bool ret = ioMessageManager_->IsFirstLineHeader(path);
        EXPECT_TRUE(ret);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "IsFirstLineHeaderTest003 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "IsFirstLineHeaderTest003 End";
}

/**
 * @tc.name: RecordDataToFileTest001
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, RecordDataToFileTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RecordDataToFileTest001 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        ioMessageManager_->RecordDataToFile(path);
        EXPECT_FALSE(false);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "RecordDataToFileTest001 ERROR";
    }
    GTEST_LOG_(INFO) << "RecordDataToFileTest001 End";
}

/**
 * @tc.name: RecordDataToFileTest002
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, RecordDataToFileTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RecordDataToFileTest002 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "time\n";
        write(fd, line1.c_str(), line1.size());
        close(fd);
        ioMessageManager_->RecordDataToFile(path);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RecordDataToFileTest002 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "RecordDataToFileTest002 End";
}

/**
 * @tc.name: RecordDataToFileTest003
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, RecordDataToFileTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RecordDataToFileTest003 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "test\n";
        write(fd, line1.c_str(), line1.size());
        close(fd);
        ioMessageManager_->RecordDataToFile(path);
        EXPECT_FALSE(false);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "RecordDataToFileTest003 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "RecordDataToFileTest003 End";
}

/**
 * @tc.name: ProcessIoDataTest001
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ProcessIoDataTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ProcessIoDataTest001 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        ioMessageManager_->currentBundleName = "";
        ioMessageManager_->lastestAppStateData.bundleName = "";
        ioMessageManager_->ProcessIoData(path);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ProcessIoDataTest001 ERROR";
    }
    GTEST_LOG_(INFO) << "ProcessIoDataTest001 End";
}

/**
 * @tc.name: ProcessIoDataTest002
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ProcessIoDataTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ProcessIoDataTest002 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        ioMessageManager_->currentBundleName = "";
        ioMessageManager_->lastestAppStateData.bundleName = "test";
        ioMessageManager_->ProcessIoData(path);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ProcessIoDataTest002 ERROR";
    }
    GTEST_LOG_(INFO) << "ProcessIoDataTest002 End";
}

/**
 * @tc.name: ProcessIoDataTest003
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ProcessIoDataTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ProcessIoDataTest003 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:1\n";
        string line5 = "syscstat:a\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        write(fd, line5.c_str(), line5.size());
        close(fd);
        ioMessageManager_->ProcessIoData(path);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ProcessIoDataTest003 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ProcessIoDataTest003 End";
}

/**
 * @tc.name: ProcessIoDataTest004
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ProcessIoDataTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ProcessIoDataTest004 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:1\n";
        string line5 = "syscstat:99999999999999999999\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        write(fd, line5.c_str(), line5.size());
        close(fd);
        ioMessageManager_->ProcessIoData(path);
        EXPECT_FALSE(false);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ProcessIoDataTest004 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ProcessIoDataTest004 End";
}

/**
 * @tc.name: ProcessIoDataTest005
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ProcessIoDataTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ProcessIoDataTest005 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:1\n";
        string line5 = "syscstat:1\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        write(fd, line5.c_str(), line5.size());
        close(fd);
        ioMessageManager_->preData.rchar = ONE;
        ioMessageManager_->ProcessIoData(path);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ProcessIoDataTest005 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ProcessIoDataTest005 End";
}

/**
 * @tc.name: ProcessIoDataTest006
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ProcessIoDataTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ProcessIoDataTest006 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:1\n";
        string line5 = "syscstat:1\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        write(fd, line5.c_str(), line5.size());
        close(fd);
        ioMessageManager_->preData.rchar = ZERO;
        ioMessageManager_->ProcessIoData(path);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ProcessIoDataTest006 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ProcessIoDataTest006 End";
}

/**
 * @tc.name: ProcessIoDataTest007
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ProcessIoDataTest007, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ProcessIoDataTest007 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        ioMessageManager_->currentBundleName = "";
        ioMessageManager_->lastestAppStateData.bundleName = "test";
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:2\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:1\n";
        string line5 = "syscstat:1\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        write(fd, line5.c_str(), line5.size());
        close(fd);
        ioMessageManager_->preData.rchar = ONE;
        ioMessageManager_->ProcessIoData(path);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ProcessIoDataTest007 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ProcessIoDataTest007 End";
}

/**
 * @tc.name: ProcessIoDataTest008
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ProcessIoDataTest008, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ProcessIoDataTest008 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        ioMessageManager_->currentBundleName = "";
        ioMessageManager_->lastestAppStateData.bundleName = "test";
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:0\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:1\n";
        string line5 = "syscstat:1\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        write(fd, line5.c_str(), line5.size());
        close(fd);
        ioMessageManager_->preData.rchar = ONE;
        ioMessageManager_->ProcessIoData(path);
        EXPECT_FALSE(false);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ProcessIoDataTest008 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ProcessIoDataTest008 End";
}

/**
 * @tc.name: ProcessIoDataTest009
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ProcessIoDataTest009, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ProcessIoDataTest009 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        ioMessageManager_->currentBundleName = "";
        ioMessageManager_->lastestAppStateData.bundleName = "test";
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:0\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:1\n";
        string line5 = "syscstat:1\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        write(fd, line5.c_str(), line5.size());
        close(fd);
        ioMessageManager_->preData.rchar = ONE;
        ioMessageManager_->dataToWrite.result = ABOVE_DATA;
        ioMessageManager_->ProcessIoData(path);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ProcessIoDataTest009 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ProcessIoDataTest009 End";
}

/**
 * @tc.name: ProcessIoDataTest010
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ProcessIoDataTest010, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ProcessIoDataTest010 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        ioMessageManager_->currentBundleName = "";
        ioMessageManager_->lastestAppStateData.bundleName = "test";
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:0\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:2000\n";
        string line5 = "syscstat:0\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        write(fd, line5.c_str(), line5.size());
        close(fd);
        ioMessageManager_->preData.rchar = ONE;
        ioMessageManager_->dataToWrite.result = ZERO;
        ioMessageManager_->preData.syscopen = ZERO;
        ioMessageManager_->ProcessIoData(path);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ProcessIoDataTest010 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ProcessIoDataTest010 End";
}

/**
 * @tc.name: ProcessIoDataTest011
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ProcessIoDataTest011, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ProcessIoDataTest011 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        ioMessageManager_->currentBundleName = "";
        ioMessageManager_->lastestAppStateData.bundleName = "test";
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:0\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:0\n";
        string line5 = "syscstat:2000\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        write(fd, line5.c_str(), line5.size());
        close(fd);
        ioMessageManager_->preData.rchar = ONE;
        ioMessageManager_->dataToWrite.result = ZERO;
        ioMessageManager_->preData.syscopen = ZERO;
        ioMessageManager_->preData.syscstat = ZERO;
        ioMessageManager_->ProcessIoData(path);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ProcessIoDataTest011 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ProcessIoDataTest011 End";
}

/**
 * @tc.name: ProcessIoDataTest012
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ProcessIoDataTest012, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ProcessIoDataTest012 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        ioMessageManager_->currentBundleName = "";
        ioMessageManager_->lastestAppStateData.bundleName = "test";
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:0\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:0\n";
        string line5 = "syscstat:0\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        write(fd, line5.c_str(), line5.size());
        close(fd);
        ioMessageManager_->preData.rchar = ZERO;
        ioMessageManager_->dataToWrite.result = ZERO;
        ioMessageManager_->preData.syscopen = ZERO;
        ioMessageManager_->preData.syscstat = ZERO;
        ioMessageManager_->ProcessIoData(path);
        EXPECT_FALSE(false);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ProcessIoDataTest012 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ProcessIoDataTest012 End";
}

/**
 * @tc.name: ProcessIoDataTest013
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ProcessIoDataTest013, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ProcessIoDataTest013 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:1\n";
        string line2 = "syscr:1\n";
        string line3 = "read_bytes:1\n";
        string line4 = "syscopen:1\n";
        string line5 = "syscstat:a\n";
        write(fd, line1.c_str(), line1.size());
        write(fd, line2.c_str(), line2.size());
        write(fd, line3.c_str(), line3.size());
        write(fd, line4.c_str(), line4.size());
        write(fd, line5.c_str(), line5.size());
        close(fd);
        ioMessageManager_->ProcessIoData(path);
        EXPECT_FALSE(false);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ProcessIoDataTest013 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ProcessIoDataTest013 End";
}

/**
 * @tc.name: ProcessIoDataTest014
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, ProcessIoDataTest014, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ProcessIoDataTest014 Start";
    string path = "/data/service/el1/public/cloudfile/rdb/1.txt";
    try {
        ioMessageManager_->currentBundleName = "";
        ioMessageManager_->lastestAppStateData.bundleName = "test";
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        string line1 = "rchar:99999999999999999999\n";
        write(fd, line1.c_str(), line1.size());
        close(fd);
        ioMessageManager_->currentData.rchar = ZERO;
        ioMessageManager_->preData.rchar = ONE;
        ioMessageManager_->dataToWrite.result = ZERO;
        ioMessageManager_->currentData.syscopen = ZERO;
        ioMessageManager_->preData.syscopen = ZERO;
        ioMessageManager_->currentData.syscstat = ZERO;
        ioMessageManager_->preData.syscopen = ZERO;
        ioMessageManager_->ProcessIoData(path);
        EXPECT_FALSE(false);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "ProcessIoDataTest014 ERROR";
    }
    unlink(path.c_str());
    GTEST_LOG_(INFO) << "ProcessIoDataTest014 End";
}

/**
 * @tc.name: RecordIoDataTest001
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, RecordIoDataTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RecordIoDataTest001 Start";
    try {
        ioMessageManager_->RecordIoData();
        EXPECT_FALSE(false);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "RecordIoDataTest001 ERROR";
    }
    GTEST_LOG_(INFO) << "RecordIoDataTest001 End";
}

/**
 * @tc.name: RecordIoDataTest002
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, RecordIoDataTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RecordIoDataTest002 Start";
    try {
        ioMessageManager_->isThreadRunning = true;
        std::thread recordThread([this]() {
            this->ioMessageManager_->RecordIoData();
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_TIME));
        ioMessageManager_->isThreadRunning = false;
        recordThread.join();
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RecordIoDataTest002 ERROR";
    }
    GTEST_LOG_(INFO) << "RecordIoDataTest002 End";
}

/**
 * @tc.name: RecordIoDataTest003
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, RecordIoDataTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RecordIoDataTest003 Start";
    try {
        ioMessageManager_->isThreadRunning = true;
        ioMessageManager_->lastestAppStateData.bundleName = "test";
        std::thread recordThread([this]() {
            this->ioMessageManager_->RecordIoData();
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_TIME));
        ioMessageManager_->isThreadRunning = false;
        recordThread.join();
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RecordIoDataTest003 ERROR";
    }
    GTEST_LOG_(INFO) << "RecordIoDataTest003 End";
}

/**
 * @tc.name: GetInstanceTest001
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, GetInstanceTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetInstanceTest001 Start";
    try {
        shared_ptr<IoMessageManager> ioMessageManager1_ = make_shared<IoMessageManager>();
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetInstanceTest001 ERROR";
    }
    GTEST_LOG_(INFO) << "GetInstanceTest001 End";
}

/**
 * @tc.name: OnReceiveEventTest001
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, OnReceiveEventTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnReceiveEventTest001 Start";
    try {
        AppExecFwk::AppStateData appStateData;
        appStateData.bundleName = "com.ohos.sceneboard";
        ioMessageManager_->OnReceiveEvent(appStateData);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnReceiveEventTest001 ERROR";
    }
    GTEST_LOG_(INFO) << "OnReceiveEventTest001 End";
}

/**
 * @tc.name: OnReceiveEventTest002
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, OnReceiveEventTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnReceiveEventTest002 Start";
    try {
        AppExecFwk::AppStateData appStateData;
        appStateData.bundleName = "";
        appStateData.state = FRONT_EVENT;
        ioMessageManager_->ioThread =std::thread();
        ioMessageManager_->OnReceiveEvent(appStateData);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnReceiveEventTest002 ERROR";
    }
    GTEST_LOG_(INFO) << "OnReceiveEventTest002 End";
}

/**
 * @tc.name: OnReceiveEventTest004
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, OnReceiveEventTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnReceiveEventTest004 Start";
    AppExecFwk::AppStateData appStateData;
    appStateData.bundleName = "";
    appStateData.state = BACKGROUND_EVENT;
    try {
        ioMessageManager_->OnReceiveEvent(appStateData);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnReceiveEventTest004 ERROR";
    }
    GTEST_LOG_(INFO) << "OnReceiveEventTest004 End";
}

/**
 * @tc.name: OnReceiveEventTest005
 * @tc.desc: Read IO data
 * @tc.type: FUNC
 * @tc.require: issuesI92WQP
 */
HWTEST_F(IoMessageListenerTest, OnReceiveEventTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnReceiveEventTest005 Start";
    AppExecFwk::AppStateData appStateData;
    appStateData.bundleName = "";
    appStateData.state = UNKNOWN_EVENT;
    try {
        ioMessageManager_->OnReceiveEvent(appStateData);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnReceiveEventTest005 ERROR";
    }
    GTEST_LOG_(INFO) << "OnReceiveEventTest005 End";
}

} // namespace OHOS::FileManagement::CloudDisk::Test