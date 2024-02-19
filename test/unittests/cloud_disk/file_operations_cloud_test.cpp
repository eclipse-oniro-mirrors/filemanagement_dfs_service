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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "file_operations_cloud.h"

#include "assistant.h"
#include "cloud_disk_inode.h"
#include "clouddisk_rdb_utils.h"
#include "dfs_error.h"

namespace OHOS::FileManagement::CloudDisk {
size_t CloudDiskRdbUtils::FuseDentryAlignSize(const char *name)
{
    return 0;
}
}

namespace OHOS::FileManagement::CloudDisk::Test {
using namespace testing;
using namespace testing::ext;
using namespace std;

class FileOperationsCloudTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static inline shared_ptr<FileOperationsCloud> fileOperationsCloud_ = nullptr;
    static inline shared_ptr<AssistantMock> insMock = nullptr;
};

void FileOperationsCloudTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
    fileOperationsCloud_ = make_shared<FileOperationsCloud>();
    insMock = make_shared<AssistantMock>();
    Assistant::ins = insMock;
}

void FileOperationsCloudTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
    Assistant::ins = nullptr;
    insMock = nullptr;
    fileOperationsCloud_ = nullptr;
}

void FileOperationsCloudTest::SetUp(void)
{
    GTEST_LOG_(INFO) << "SetUp";
}

void FileOperationsCloudTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "TearDown";
}

/**
 * @tc.name: LookupTest001
 * @tc.desc: Verify the Lookup function
 * @tc.type: FUNC
 * @tc.require: issuesI91IOG
 */
HWTEST_F(FileOperationsCloudTest, LookupTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "LookupTest001 Start";
    try {
        CloudDiskFuseData data;
        EXPECT_CALL(*insMock, fuse_req_userdata(_)).WillOnce(Return(reinterpret_cast<void*>(&data)));
        EXPECT_CALL(*insMock, fuse_reply_err(_, _)).WillOnce(Return(E_OK));

        fuse_ino_t parent = FUSE_ROOT_ID;
        const char *name = "";
        fileOperationsCloud_->Lookup(nullptr, parent, name);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "LookupTest001  ERROR";
    }
    GTEST_LOG_(INFO) << "LookupTest001 End";
}

/**
 * @tc.name: OpenTest001
 * @tc.desc: Verify the Open function
 * @tc.type: FUNC
 * @tc.require: issuesI91IOG
 */
HWTEST_F(FileOperationsCloudTest, OpenTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OpenTest001 Start";
    try {
        CloudDiskFuseData data;
        fuse_req_t req = nullptr;
        CloudDiskInode ino;
        struct fuse_file_info fi;

        EXPECT_CALL(*insMock, fuse_req_userdata(_)).WillOnce(Return(reinterpret_cast<void*>(&data)));
        EXPECT_CALL(*insMock, access(_, _)).WillOnce(Return(E_OK));
        EXPECT_CALL(*insMock, open(_, _)).WillOnce(Return(E_OK));
        EXPECT_CALL(*insMock, fuse_reply_open(_, _)).WillOnce(Return(E_OK));
        fi.flags |= (O_ACCMODE | O_WRONLY);
        fileOperationsCloud_->Open(req, reinterpret_cast<fuse_ino_t>(&ino), &fi);
        EXPECT_TRUE(true);

        EXPECT_CALL(*insMock, fuse_req_userdata(_)).WillOnce(Return(reinterpret_cast<void*>(&data)));
        EXPECT_CALL(*insMock, access(_, _)).WillOnce(Return(E_OK));
        EXPECT_CALL(*insMock, open(_, _)).WillOnce(Return(E_OK));
        EXPECT_CALL(*insMock, fuse_reply_open(_, _)).WillOnce(Return(E_OK));
        fi.flags |= O_APPEND;
        fileOperationsCloud_->Open(req, reinterpret_cast<fuse_ino_t>(&ino), &fi);
        EXPECT_TRUE(true);

        EXPECT_CALL(*insMock, fuse_req_userdata(_)).WillOnce(Return(reinterpret_cast<void*>(&data)));
        EXPECT_CALL(*insMock, access(_, _)).WillOnce(Return(E_OK));
        EXPECT_CALL(*insMock, open(_, _)).WillOnce(Return(E_OK));
        EXPECT_CALL(*insMock, fuse_reply_open(_, _)).WillOnce(Return(E_OK));
        fi.flags |= O_DIRECT;
        fileOperationsCloud_->Open(req, reinterpret_cast<fuse_ino_t>(&ino), &fi);
        EXPECT_TRUE(true);

        EXPECT_CALL(*insMock, fuse_req_userdata(_)).WillOnce(Return(reinterpret_cast<void*>(&data)));
        EXPECT_CALL(*insMock, access(_, _)).WillOnce(Return(E_OK));
        EXPECT_CALL(*insMock, open(_, _)).WillOnce(Return(-1));
        EXPECT_CALL(*insMock, fuse_reply_err(_, _)).WillOnce(Return(E_OK));
        fileOperationsCloud_->Open(req, reinterpret_cast<fuse_ino_t>(&ino), &fi);
        EXPECT_TRUE(true);

        EXPECT_CALL(*insMock, fuse_req_userdata(_))
            .WillOnce(Return(reinterpret_cast<void*>(&data)))
            .WillOnce(Return(reinterpret_cast<void*>(&data)));
        EXPECT_CALL(*insMock, access(_, _)).WillOnce(Return(E_INVAL_ARG));
        EXPECT_CALL(*insMock, fuse_reply_open(_, _)).WillOnce(Return(E_OK));
        fileOperationsCloud_->Open(req, reinterpret_cast<fuse_ino_t>(&ino), &fi);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OpenTest001  ERROR";
    }
    GTEST_LOG_(INFO) << "OpenTest001 End";
}

/**
 * @tc.name: ForgetTest001
 * @tc.desc: Verify the Forget function
 * @tc.type: FUNC
 * @tc.require: issuesI91IOG
 */
HWTEST_F(FileOperationsCloudTest, ForgetTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ForgetTest001 Start";
    try {
        CloudDiskFuseData data;
        fuse_req_t req = nullptr;
        CloudDiskInode ino;
        uint64_t nLookup = 0;

        EXPECT_CALL(*insMock, fuse_req_userdata(_)).WillOnce(Return(reinterpret_cast<void*>(&data)));
        EXPECT_CALL(*insMock, fuse_reply_none(_)).WillOnce(Return());
        fileOperationsCloud_->Forget(req, FUSE_ROOT_ID, nLookup);
        EXPECT_TRUE(true);

        EXPECT_CALL(*insMock, fuse_req_userdata(_)).WillOnce(Return(reinterpret_cast<void*>(&data)));
        EXPECT_CALL(*insMock, fuse_reply_none(_)).WillOnce(Return());
        fileOperationsCloud_->Forget(req, 0, nLookup);
        EXPECT_TRUE(true);

        ino.parent = 0;
        EXPECT_CALL(*insMock, fuse_req_userdata(_)).WillOnce(Return(reinterpret_cast<void*>(&data)));
        EXPECT_CALL(*insMock, fuse_reply_none(_)).WillOnce(Return());
        fileOperationsCloud_->Forget(req, reinterpret_cast<fuse_ino_t>(&ino), nLookup);
        EXPECT_TRUE(true);

        ino.parent = reinterpret_cast<fuse_ino_t>(&ino);
        EXPECT_CALL(*insMock, fuse_req_userdata(_)).WillOnce(Return(reinterpret_cast<void*>(&data)));
        EXPECT_CALL(*insMock, fuse_reply_none(_)).WillOnce(Return());
        fileOperationsCloud_->Forget(req, reinterpret_cast<fuse_ino_t>(&ino), nLookup);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ForgetTest001  ERROR";
    }
    GTEST_LOG_(INFO) << "ForgetTest001 End";
}

/**
 * @tc.name: MkDirTest001
 * @tc.desc: Verify the MkDir function
 * @tc.type: FUNC
 * @tc.require: issuesI91IOG
 */
HWTEST_F(FileOperationsCloudTest, MkDirTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "MkDirTest001 Start";
    try {
        CloudDiskFuseData data;
        fuse_req_t req = nullptr;
        CloudDiskInode ino;
        const char *name = nullptr;
        mode_t mode = 0;

        ino.parent = reinterpret_cast<fuse_ino_t>(&ino);
        EXPECT_CALL(*insMock, fuse_req_userdata(_)).WillOnce(Return(reinterpret_cast<void*>(&data)));
        EXPECT_CALL(*insMock, fuse_reply_err(_, _)).WillOnce(Return(E_OK));
        fileOperationsCloud_->MkDir(req, reinterpret_cast<fuse_ino_t>(&ino), name, mode);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "MkDirTest001  ERROR";
    }
    GTEST_LOG_(INFO) << "MkDirTest001 End";
}

/**
 * @tc.name: RmDirTest001
 * @tc.desc: Verify the RmDir function
 * @tc.type: FUNC
 * @tc.require: issuesI91IOG
 */
HWTEST_F(FileOperationsCloudTest, RmDirTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RmDirTest001 Start";
    try {
        CloudDiskFuseData data;
        fuse_req_t req = nullptr;
        CloudDiskInode ino;
        const char name[1] = "" ;

        ino.bundleName = "test";
        ino.parent = reinterpret_cast<fuse_ino_t>(&ino);
        EXPECT_CALL(*insMock, fuse_req_userdata(_)).WillOnce(Return(reinterpret_cast<void*>(&data)));
        EXPECT_CALL(*insMock, fuse_reply_err(_, _)).WillOnce(Return(E_OK));
        fileOperationsCloud_->RmDir(req, reinterpret_cast<fuse_ino_t>(&ino), name);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RmDirTest001  ERROR";
    }
    GTEST_LOG_(INFO) << "RmDirTest001 End";
}

/**
 * @tc.name: UnlinkTest001
 * @tc.desc: Verify the Unlink function
 * @tc.type: FUNC
 * @tc.require: issuesI91IOG
 */
HWTEST_F(FileOperationsCloudTest, UnlinkTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnlinkTest001 Start";
    try {
        CloudDiskFuseData data;
        fuse_req_t req = nullptr;
        CloudDiskInode ino;
        const char name[1] = "" ;

        ino.bundleName = "test";
        ino.parent = reinterpret_cast<fuse_ino_t>(&ino);
        EXPECT_CALL(*insMock, fuse_req_userdata(_)).WillOnce(Return(reinterpret_cast<void*>(&data)));
        EXPECT_CALL(*insMock, fuse_reply_err(_, _)).WillOnce(Return(E_OK));
        fileOperationsCloud_->Unlink(req, reinterpret_cast<fuse_ino_t>(&ino), name);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UnlinkTest001  ERROR";
    }
    GTEST_LOG_(INFO) << "UnlinkTest001 End";
}
}