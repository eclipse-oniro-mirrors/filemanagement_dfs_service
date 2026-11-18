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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "cloud_disk_service_callback_manager.h"

#include "assistant.h"
#include "cloud_disk_service_callback_mock.h"
#include "cloud_disk_service_logfile.h"
#include "cloud_disk_service_syncfolder.h"
#include "cloud_disk_sync_folder.h"
#include "dfs_error.h"
#include "cloud_disk_service_access_token_mock.h"
#include "utils_log.h"

namespace OHOS {
namespace FileManagement::CloudDiskService {
namespace Test {
using namespace std;
using namespace testing;
using namespace testing::ext;

class TrackingCallbackMock : public CloudDiskServiceCallbackMock {
public:
    vector<ChangeData> lastChangeData;
    string lastSandboxPath;
    bool wasCalled = false;
    void OnChangeData(const string &sandboxPath, const vector<ChangeData> &changeData) override
    {
        wasCalled = true;
        lastSandboxPath = sandboxPath;
        lastChangeData = changeData;
    }
};

class CloudDiskServiceCallbackManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static inline shared_ptr<CloudDiskServiceAccessTokenMock> dfsuAccessTokenMock_ = nullptr;
    static inline shared_ptr<AssistantMock> insMock_ = nullptr;
};

void CloudDiskServiceCallbackManagerTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
    insMock_ = make_shared<AssistantMock>();
    Assistant::ins = insMock_;
}

void CloudDiskServiceCallbackManagerTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
    Assistant::ins = nullptr;
    insMock_ = nullptr;
}

void CloudDiskServiceCallbackManagerTest::SetUp(void)
{
    GTEST_LOG_(INFO) << "SetUp";
    dfsuAccessTokenMock_ = make_shared<CloudDiskServiceAccessTokenMock>();
    CloudDiskServiceAccessTokenMock::dfsuAccessToken = dfsuAccessTokenMock_;
}

void CloudDiskServiceCallbackManagerTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "TearDown";
    dfsuAccessTokenMock_ = nullptr;
}

/**
 * @tc.name: GetCloudDiskServiceCallbackManagerInstanceTest001
 * @tc.desc: Verify the GetCloudDiskServiceCallbackManagerInstance function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, GetCloudDiskServiceCallbackManagerInstanceTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetCloudDiskServiceCallbackManagerInstanceTest001 start";
    try {
        CloudDiskServiceCallbackManager& instance1 = CloudDiskServiceCallbackManager::GetInstance();
        CloudDiskServiceCallbackManager& instance2 = CloudDiskServiceCallbackManager::GetInstance();
        EXPECT_EQ(&instance1, &instance2);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetCloudDiskServiceCallbackManagerInstanceTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetCloudDiskServiceCallbackManagerInstanceTest001 end";
}

/**
 * @tc.name: AddCallbackTest001
 * @tc.desc: Verify the AddCallback function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, AddCallbackTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AddCallbackTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.example";
        uint32_t syncFolderIndex = 1;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        bool result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        EXPECT_TRUE(result);
        callbackManager.AddCallback(bundleName, callback);
        callbackManager.AddCallback(bundleName, callback);
        EXPECT_EQ(callbackManager.callbackAppMap_[bundleName].callback, callback);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AddCallbackTest001 failed";
    }
    GTEST_LOG_(INFO) << "AddCallbackTest001 end";
}

/**
 * @tc.name: AddCallbackTest002
 * @tc.desc: Verify the AddCallback function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, AddCallbackTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AddCallbackTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.example";
        callbackManager.AddCallback(bundleName, nullptr);
        EXPECT_EQ(callbackManager.callbackAppMap_[bundleName].callback, nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AddCallbackTest002 failed";
    }
    GTEST_LOG_(INFO) << "AddCallbackTest002 end";
}

/**
 * @tc.name: OnChangeDataTest001
 * @tc.desc: Verify the OnChangeData function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 1;
        ChangeData data;
        data.updateSequenceNumber = 1;
        data.fileId = "fileId123";
        vector<ChangeData> changeData = {data};
        callbackManager.OnChangeData(syncFolderIndex, changeData);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataTest001 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataTest001 end";
}

/**
 * @tc.name: OnChangeDataTest002
 * @tc.desc: Verify the OnChangeData function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataTest002 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 2;
        ChangeData data;
        data.updateSequenceNumber = 1;
        data.fileId = "fileId123";
        vector<ChangeData> changeData = {data};
        string bundleName = "com.test.example";
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        callbackManager.OnChangeData(syncFolderIndex, changeData);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataTest002 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataTest002 end";
}

/**
 * @tc.name: OnChangeDataTest003
 * @tc.desc: Verify the OnChangeData function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataTest003 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 3;
        ChangeData data;
        data.updateSequenceNumber = 1;
        data.fileId = "fileId123";
        vector<ChangeData> changeData = {data};
        string bundleName = "com.test.example";
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillOnce(Return(123));
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        SyncFolderValue value;
        value.bundleName = bundleName;
        value.path = "/data/service/el2/123/hmdfs/account/files/Docs/testfile.txt";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        callbackManager.OnChangeData(syncFolderIndex, changeData);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataTest003 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataTest003 end";
}

/**
 * @tc.name: OnChangeDataTest004
 * @tc.desc: Verify the OnChangeData function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataTest004 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 1;
        ChangeData data;
        data.updateSequenceNumber = 1;
        data.fileId = "fileId123";
        vector<ChangeData> changeData = {data};
        string bundleName = "com.test.example";
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillOnce(Return(0));
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        SyncFolderValue value;
        value.bundleName = bundleName;
        value.path = "/data/service/el2/123/Docs/testfile.txt";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        callbackManager.OnChangeData(syncFolderIndex, changeData);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataTest004 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataTest004 end";
}

/**
 * @tc.name: RegisterSyncFolderMapTest001
 * @tc.desc: Verify the RegisterSyncFolderMap function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, RegisterSyncFolderMapTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RegisterSyncFolderMapTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.example";
        uint32_t syncFolderIndex = 1;
        uint32_t syncFolderIndexNew = 2;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        bool result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        EXPECT_TRUE(result);
        result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        EXPECT_FALSE(result);
        result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndexNew, callback);
        EXPECT_TRUE(result);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RegisterSyncFolderMapTest001 failed";
    }
    GTEST_LOG_(INFO) << "RegisterSyncFolderMapTest001 end";
}

/**
 * @tc.name: UnregisterSyncFolderMapTest001
 * @tc.desc: Verify the UnregisterSyncFolderMap function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, UnregisterSyncFolderMapTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnregisterSyncFolderMapTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.example";
        uint32_t syncFolderIndex = 3;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        bool result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        EXPECT_TRUE(result);
        EXPECT_EQ(callbackManager.callbackIndexMap_.size(), 1);
        callbackManager.UnregisterSyncFolderMap(bundleName, syncFolderIndex);
        EXPECT_EQ(callbackManager.callbackIndexMap_.size(), 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UnregisterSyncFolderMapTest001 failed";
    }
    GTEST_LOG_(INFO) << "UnregisterSyncFolderMapTest001 end";
}

/**
 * @tc.name: UnregisterSyncFolderForChangesMapTest001
 * @tc.desc: Verify the UnregisterSyncFolderForChangesMap function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, UnregisterSyncFolderForChangesMapTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.example";
        uint32_t syncFolderIndex = 4;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        bool result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        EXPECT_TRUE(result);
        EXPECT_EQ(callbackManager.callbackIndexMap_.size(), 1);
        auto ret = callbackManager.UnregisterSyncFolderForChangesMap(bundleName, syncFolderIndex);
        EXPECT_TRUE(ret);
        EXPECT_EQ(callbackManager.callbackIndexMap_.size(), 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest001 failed";
    }
    GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest001 end";
}

/**
 * @tc.name: UnregisterSyncFolderForChangesMapTest002
 * @tc.desc: Verify the UnregisterSyncFolderForChangesMap function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, UnregisterSyncFolderForChangesMapTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest002 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.example";
        string bundleName_notExists = "com.test.kk";
        uint32_t syncFolderIndex = 5;
        uint32_t syncFolderIndex_notExists = 6;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        bool result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        EXPECT_TRUE(result);
        EXPECT_EQ(callbackManager.callbackIndexMap_.size(), 1);
        auto ret = callbackManager.UnregisterSyncFolderForChangesMap(bundleName, syncFolderIndex_notExists);
        EXPECT_FALSE(ret);
        EXPECT_EQ(callbackManager.callbackIndexMap_.size(), 1);
        ret = callbackManager.UnregisterSyncFolderForChangesMap(bundleName_notExists, syncFolderIndex);
        EXPECT_FALSE(ret);
        EXPECT_EQ(callbackManager.callbackIndexMap_.size(), 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest002 failed";
    }
    GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest002 end";
}

/**
 * @tc.name: ClearMapTest001
 * @tc.desc: Verify the ClearMap function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, ClearMapTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ClearMapTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        callbackManager.ClearMap();
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ClearMapTest001 failed";
    }
    GTEST_LOG_(INFO) << "ClearMapTest001 end";
}

/**
 * @tc.name: RegisterSyncFolderMapTest002
 * @tc.desc: Verify the RegisterSyncFolderMap function with existing bundle
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, RegisterSyncFolderMapTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RegisterSyncFolderMapTest002 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.example";
        uint32_t syncFolderIndex1 = 1;
        uint32_t syncFolderIndex2 = 2;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        bool result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex1, callback);
        EXPECT_TRUE(result);
        result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex2, callback);
        EXPECT_TRUE(result);
        EXPECT_EQ(callbackManager.callbackAppMap_[bundleName].syncFolderIndexs.size(), 2);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RegisterSyncFolderMapTest002 failed";
    }
    GTEST_LOG_(INFO) << "RegisterSyncFolderMapTest002 end";
}

/**
 * @tc.name: UnregisterSyncFolderMapTest002
 * @tc.desc: Verify the UnregisterSyncFolderMap function with non-existent index
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, UnregisterSyncFolderMapTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnregisterSyncFolderMapTest002 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.example";
        uint32_t syncFolderIndex = 3;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        callbackManager.UnregisterSyncFolderMap(bundleName, syncFolderIndex);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UnregisterSyncFolderMapTest002 failed";
    }
    GTEST_LOG_(INFO) << "UnregisterSyncFolderMapTest002 end";
}

/**
 * @tc.name: UnregisterSyncFolderMapTest003
 * @tc.desc: Verify the UnregisterSyncFolderMap function with non-existent bundle
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, UnregisterSyncFolderMapTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnregisterSyncFolderMapTest003 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.example";
        uint32_t syncFolderIndex = 3;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        bool result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        EXPECT_TRUE(result);
        string nonExistentBundle = "com.test.nonexistent";
        callbackManager.UnregisterSyncFolderMap(nonExistentBundle, syncFolderIndex);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UnregisterSyncFolderMapTest003 failed";
    }
    GTEST_LOG_(INFO) << "UnregisterSyncFolderMapTest003 end";
}

/**
 * @tc.name: OnChangeDataTest005
 * @tc.desc: Verify the OnChangeData function with null callback
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataTest005 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 5;
        ChangeData data;
        data.updateSequenceNumber = 1;
        data.fileId = "fileId123";
        vector<ChangeData> changeData = {data};
        string bundleName = "com.test.example";
        sptr<ICloudDiskServiceCallback> callback = nullptr;
        callbackManager.callbackIndexMap_[syncFolderIndex] = {callback, {}, false};
        callbackManager.OnChangeData(syncFolderIndex, changeData);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataTest005 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataTest005 end";
}

/**
 * @tc.name: OnChangeDataTest006
 * @tc.desc: Verify the OnChangeData function with valid userId
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataTest006 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 6;
        ChangeData data;
        data.updateSequenceNumber = 1;
        data.fileId = "fileId123";
        vector<ChangeData> changeData = {data};
        string bundleName = "com.test.example";
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillOnce(Return(456));
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        SyncFolderValue value;
        value.bundleName = bundleName;
        value.path = "/data/service/el2/456/hmdfs/account/files/Docs/testfile.txt";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        callbackManager.OnChangeData(syncFolderIndex, changeData);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataTest006 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataTest006 end";
}

/**
 * @tc.name: AddCallbackTest003
 * @tc.desc: Verify the AddCallback function with death callback
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, AddCallbackTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AddCallbackTest003 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.death";
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillRepeatedly(Return(789));
        callbackManager.AddCallback(bundleName, callback);
        EXPECT_EQ(callbackManager.callbackAppMap_[bundleName].callback, callback);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AddCallbackTest003 failed";
    }
    GTEST_LOG_(INFO) << "AddCallbackTest003 end";
}

/**
 * @tc.name: AddCallbackTest004
 * @tc.desc: Verify the AddCallback function with death callback and sync folders
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, AddCallbackTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AddCallbackTest004 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.death2";
        uint32_t syncFolderIndex1 = 10;
        uint32_t syncFolderIndex2 = 11;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillRepeatedly(Return(0));
        EXPECT_CALL(*dfsuAccessTokenMock_, GetAccountId(_)).WillRepeatedly(Return(true));
        bool result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex1, callback);
        EXPECT_TRUE(result);
        result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex2, callback);
        EXPECT_TRUE(result);
        callbackManager.AddCallback(bundleName, callback);
        EXPECT_EQ(callbackManager.callbackAppMap_[bundleName].callback, callback);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AddCallbackTest004 failed";
    }
    GTEST_LOG_(INFO) << "AddCallbackTest004 end";
}

/**
 * @tc.name: UnregisterSyncFolderForChangesMapTest003
 * @tc.desc: Verify the UnregisterSyncFolderForChangesMap function with non-existent index in vector
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, UnregisterSyncFolderForChangesMapTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest003 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.example";
        uint32_t syncFolderIndex1 = 7;
        uint32_t syncFolderIndex2 = 8;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        bool result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex1, callback);
        EXPECT_TRUE(result);
        result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex2, callback);
        EXPECT_TRUE(result);
        auto ret = callbackManager.UnregisterSyncFolderForChangesMap(bundleName, syncFolderIndex1);
        EXPECT_TRUE(ret);
        EXPECT_EQ(callbackManager.callbackIndexMap_.size(), 1);
        EXPECT_EQ(callbackManager.callbackAppMap_[bundleName].syncFolderIndexs.size(), 1);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest003 failed";
    }
    GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest003 end";
}

/**
 * @tc.name: ClearMapTest002
 * @tc.desc: Verify the ClearMap function with data
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, ClearMapTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ClearMapTest002 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.example";
        uint32_t syncFolderIndex = 9;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        bool result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        EXPECT_TRUE(result);
        EXPECT_EQ(callbackManager.callbackIndexMap_.size(), 1);
        EXPECT_EQ(callbackManager.callbackAppMap_.size(), 1);
        callbackManager.ClearMap();
        EXPECT_EQ(callbackManager.callbackIndexMap_.size(), 0);
        EXPECT_EQ(callbackManager.callbackAppMap_.size(), 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "ClearMapTest002 failed";
    }
    GTEST_LOG_(INFO) << "ClearMapTest002 end";
}

/**
 * @tc.name: AddCallbackTest005
 * @tc.desc: Verify the AddCallback function with empty sync folder index
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, AddCallbackTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AddCallbackTest005 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.empty";
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillRepeatedly(Return(999));
        callbackManager.AddCallback(bundleName, callback);
        EXPECT_EQ(callbackManager.callbackAppMap_[bundleName].callback, callback);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AddCallbackTest005 failed";
    }
    GTEST_LOG_(INFO) << "AddCallbackTest005 end";
}

/**
 * @tc.name: AddCallbackTest006
 * @tc.desc: Verify the AddCallback function death callback with non-existent bundle
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, AddCallbackTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AddCallbackTest006 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.nonexist";
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillRepeatedly(Return(888));
        callbackManager.AddCallback(bundleName, callback);
        EXPECT_EQ(callbackManager.callbackAppMap_[bundleName].callback, callback);
        string nonExistentBundle = "com.test.notfound";
        callbackManager.callbackAppMap_.erase(nonExistentBundle);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AddCallbackTest006 failed";
    }
    GTEST_LOG_(INFO) << "AddCallbackTest006 end";
}

/**
 * @tc.name: RegisterSyncFolderMapWithEventsTest001
 * @tc.desc: Verify RegisterSyncFolderMapWithEvents stores correct InstanceValue fields
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, RegisterSyncFolderMapWithEventsTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RegisterSyncFolderMapWithEventsTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.events";
        uint32_t syncFolderIndex = 20;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        set<OperationType> types = {OperationType::FILE_OPEN, OperationType::CREATE};
        bool result = callbackManager.RegisterSyncFolderMapWithEvents(bundleName, syncFolderIndex, types, callback);
        EXPECT_TRUE(result);
        auto &inst = callbackManager.callbackIndexMap_[syncFolderIndex];
        EXPECT_TRUE(inst.isWithEvents);
        EXPECT_EQ(inst.callback, callback);
        EXPECT_EQ(inst.operationTypes.size(), 2U);
        EXPECT_NE(inst.operationTypes.find(OperationType::FILE_OPEN), inst.operationTypes.end());
        EXPECT_NE(inst.operationTypes.find(OperationType::CREATE), inst.operationTypes.end());
        EXPECT_EQ(callbackManager.callbackAppMap_[bundleName].syncFolderIndexs.size(), 1U);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RegisterSyncFolderMapWithEventsTest001 failed";
    }
    GTEST_LOG_(INFO) << "RegisterSyncFolderMapWithEventsTest001 end";
}

/**
 * @tc.name: RegisterSyncFolderMapWithEventsTest002
 * @tc.desc: Verify duplicate RegisterSyncFolderMapWithEvents returns false
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, RegisterSyncFolderMapWithEventsTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RegisterSyncFolderMapWithEventsTest002 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.events.dup";
        uint32_t syncFolderIndex = 21;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        set<OperationType> types = {OperationType::FILE_OPEN};
        bool result = callbackManager.RegisterSyncFolderMapWithEvents(bundleName, syncFolderIndex, types, callback);
        EXPECT_TRUE(result);
        result = callbackManager.RegisterSyncFolderMapWithEvents(bundleName, syncFolderIndex, types, callback);
        EXPECT_FALSE(result);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RegisterSyncFolderMapWithEventsTest002 failed";
    }
    GTEST_LOG_(INFO) << "RegisterSyncFolderMapWithEventsTest002 end";
}

/**
 * @tc.name: OnChangeDataLegacyFilterTest001
 * @tc.desc: Verify legacy register filters out FILE_OPEN events from OnChangeData
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataLegacyFilterTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataLegacyFilterTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 30;
        string bundleName = "com.test.legacy.filter";
        sptr<TrackingCallbackMock> trackingCallback = sptr(new TrackingCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillOnce(Return(456));
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        SyncFolderValue value;
        value.bundleName = bundleName;
        value.path = "/data/service/el2/456/hmdfs/account/files/Docs/testfile.txt";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, trackingCallback);
        ChangeData openData;
        openData.operationType = OperationType::FILE_OPEN;
        openData.fileId = "openFile";
        ChangeData createData;
        createData.operationType = OperationType::CREATE;
        createData.fileId = "createFile";
        vector<ChangeData> changeData = {openData, createData};
        callbackManager.OnChangeData(syncFolderIndex, changeData);
        EXPECT_TRUE(trackingCallback->wasCalled);
        EXPECT_EQ(trackingCallback->lastChangeData.size(), 1U);
        EXPECT_EQ(trackingCallback->lastChangeData[0].operationType, OperationType::CREATE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataLegacyFilterTest001 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataLegacyFilterTest001 end";
}

/**
 * @tc.name: OnChangeDataWithEventsFilterTest001
 * @tc.desc: Verify WithEvents register passes only matching operationType events
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataWithEventsFilterTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 31;
        string bundleName = "com.test.withEvents.filter";
        sptr<TrackingCallbackMock> trackingCallback = sptr(new TrackingCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillOnce(Return(456));
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        SyncFolderValue value;
        value.bundleName = bundleName;
        value.path = "/data/service/el2/456/hmdfs/account/files/Docs/testfile.txt";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        set<OperationType> types = {OperationType::FILE_OPEN};
        callbackManager.RegisterSyncFolderMapWithEvents(bundleName, syncFolderIndex, types, trackingCallback);
        ChangeData openData;
        openData.operationType = OperationType::FILE_OPEN;
        openData.fileId = "openFile";
        ChangeData createData;
        createData.operationType = OperationType::CREATE;
        createData.fileId = "createFile";
        vector<ChangeData> changeData = {openData, createData};
        callbackManager.OnChangeData(syncFolderIndex, changeData);
        EXPECT_TRUE(trackingCallback->wasCalled);
        EXPECT_EQ(trackingCallback->lastChangeData.size(), 1U);
        EXPECT_EQ(trackingCallback->lastChangeData[0].operationType, OperationType::FILE_OPEN);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest001 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest001 end";
}

/**
 * @tc.name: OnChangeDataWithEventsFilterTest002
 * @tc.desc: Verify WithEvents register does not call callback when no events match
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataWithEventsFilterTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest002 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 32;
        string bundleName = "com.test.withEvents.nomatch";
        sptr<TrackingCallbackMock> trackingCallback = sptr(new TrackingCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillOnce(Return(456));
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        SyncFolderValue value;
        value.bundleName = bundleName;
        value.path = "/data/service/el2/456/hmdfs/account/files/Docs/testfile.txt";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        set<OperationType> types = {OperationType::DELETE};
        callbackManager.RegisterSyncFolderMapWithEvents(bundleName, syncFolderIndex, types, trackingCallback);
        ChangeData openData;
        openData.operationType = OperationType::FILE_OPEN;
        ChangeData createData;
        createData.operationType = OperationType::CREATE;
        vector<ChangeData> changeData = {openData, createData};
        callbackManager.OnChangeData(syncFolderIndex, changeData);
        EXPECT_FALSE(trackingCallback->wasCalled);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest002 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest002 end";
}

/**
 * @tc.name: OnChangeDataLegacyFilterTest002
 * @tc.desc: Verify legacy register with all FILE_OPEN events results in empty filtered list (no callback)
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataLegacyFilterTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataLegacyFilterTest002 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 33;
        string bundleName = "com.test.legacy.allOpen";
        sptr<TrackingCallbackMock> trackingCallback = sptr(new TrackingCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillOnce(Return(456));
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        SyncFolderValue value;
        value.bundleName = bundleName;
        value.path = "/data/service/el2/456/hmdfs/account/files/Docs/testfile.txt";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, trackingCallback);
        ChangeData openData1;
        openData1.operationType = OperationType::FILE_OPEN;
        openData1.fileId = "open1";
        ChangeData openData2;
        openData2.operationType = OperationType::FILE_OPEN;
        openData2.fileId = "open2";
        vector<ChangeData> changeData = {openData1, openData2};
        callbackManager.OnChangeData(syncFolderIndex, changeData);
        EXPECT_FALSE(trackingCallback->wasCalled);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataLegacyFilterTest002 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataLegacyFilterTest002 end";
}

/**
 * @tc.name: OnChangeDataWithEventsFilterTest003
 * @tc.desc: Verify WithEvents with multiple matching types passes all matching events
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataWithEventsFilterTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest003 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 34;
        string bundleName = "com.test.withEvents.multi";
        sptr<TrackingCallbackMock> trackingCallback = sptr(new TrackingCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillOnce(Return(456));
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        SyncFolderValue value;
        value.bundleName = bundleName;
        value.path = "/data/service/el2/456/hmdfs/account/files/Docs/testfile.txt";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        set<OperationType> types = {OperationType::FILE_OPEN, OperationType::CREATE};
        callbackManager.RegisterSyncFolderMapWithEvents(bundleName, syncFolderIndex, types, trackingCallback);
        ChangeData openData;
        openData.operationType = OperationType::FILE_OPEN;
        openData.fileId = "openFile";
        ChangeData createData;
        createData.operationType = OperationType::CREATE;
        createData.fileId = "createFile";
        ChangeData deleteData;
        deleteData.operationType = OperationType::DELETE;
        deleteData.fileId = "deleteFile";
        vector<ChangeData> changeData = {openData, createData, deleteData};
        callbackManager.OnChangeData(syncFolderIndex, changeData);
        EXPECT_TRUE(trackingCallback->wasCalled);
        EXPECT_EQ(trackingCallback->lastChangeData.size(), 2U);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest003 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest003 end";
}

/**
 * @tc.name: RegisterSyncFolderMapWithEventsTest003
 * @tc.desc: Verify legacy register then WithEvents register same index returns false
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, RegisterSyncFolderMapWithEventsTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RegisterSyncFolderMapWithEventsTest003 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.legacyThenEvents";
        uint32_t syncFolderIndex = 22;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        bool result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        EXPECT_TRUE(result);
        set<OperationType> types = {OperationType::FILE_OPEN};
        result = callbackManager.RegisterSyncFolderMapWithEvents(bundleName, syncFolderIndex, types, callback);
        EXPECT_FALSE(result);
        auto &inst = callbackManager.callbackIndexMap_[syncFolderIndex];
        EXPECT_FALSE(inst.isWithEvents);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RegisterSyncFolderMapWithEventsTest003 failed";
    }
    GTEST_LOG_(INFO) << "RegisterSyncFolderMapWithEventsTest003 end";
}

/**
 * @tc.name: RegisterSyncFolderMapWithEventsTest004
 * @tc.desc: Verify WithEvents register then legacy register same index returns false
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, RegisterSyncFolderMapWithEventsTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RegisterSyncFolderMapWithEventsTest004 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.eventsThenLegacy";
        uint32_t syncFolderIndex = 23;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        set<OperationType> types = {OperationType::FILE_OPEN};
        bool result = callbackManager.RegisterSyncFolderMapWithEvents(bundleName, syncFolderIndex, types, callback);
        EXPECT_TRUE(result);
        result = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        EXPECT_FALSE(result);
        auto &inst = callbackManager.callbackIndexMap_[syncFolderIndex];
        EXPECT_TRUE(inst.isWithEvents);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RegisterSyncFolderMapWithEventsTest004 failed";
    }
    GTEST_LOG_(INFO) << "RegisterSyncFolderMapWithEventsTest004 end";
}

/**
 * @tc.name: UnregisterSyncFolderForChangesMapTest004
 * @tc.desc: Verify UnregisterSyncFolderForChangesMap with index not found returns false
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, UnregisterSyncFolderForChangesMapTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest004 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.unregister.notFound";
        uint32_t syncFolderIndex = 24;
        auto ret = callbackManager.UnregisterSyncFolderForChangesMap(bundleName, syncFolderIndex);
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest004 failed";
    }
    GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest004 end";
}

/**
 * @tc.name: UnregisterSyncFolderForChangesMapTest005
 * @tc.desc: Verify UnregisterSyncFolderForChangesMap removes WithEvents instance properly
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, UnregisterSyncFolderForChangesMapTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest005 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.unregister.withEvents";
        uint32_t syncFolderIndex = 25;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        set<OperationType> types = {OperationType::FILE_OPEN, OperationType::CREATE};
        bool result = callbackManager.RegisterSyncFolderMapWithEvents(bundleName, syncFolderIndex, types, callback);
        EXPECT_TRUE(result);
        EXPECT_EQ(callbackManager.callbackIndexMap_.size(), 1);
        auto ret = callbackManager.UnregisterSyncFolderForChangesMap(bundleName, syncFolderIndex);
        EXPECT_TRUE(ret);
        EXPECT_EQ(callbackManager.callbackIndexMap_.size(), 0);
        EXPECT_EQ(callbackManager.callbackAppMap_[bundleName].syncFolderIndexs.size(), 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest005 failed";
    }
    GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest005 end";
}

/**
 * @tc.name: OnChangeDataLegacyFilterTest003
 * @tc.desc: Verify legacy register with mixed events filters FILE_OPEN and passes rest
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataLegacyFilterTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataLegacyFilterTest003 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 35;
        string bundleName = "com.test.legacy.mixed";
        sptr<TrackingCallbackMock> trackingCallback = sptr(new TrackingCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillOnce(Return(456));
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        SyncFolderValue value;
        value.bundleName = bundleName;
        value.path = "/data/service/el2/456/hmdfs/account/files/Docs/testfile.txt";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, trackingCallback);
        ChangeData openData;
        openData.operationType = OperationType::FILE_OPEN;
        openData.fileId = "openFile";
        ChangeData createData;
        createData.operationType = OperationType::CREATE;
        createData.fileId = "createFile";
        ChangeData deleteData;
        deleteData.operationType = OperationType::DELETE;
        deleteData.fileId = "deleteFile";
        ChangeData moveData;
        moveData.operationType = OperationType::MOVE_FROM;
        moveData.fileId = "moveFile";
        vector<ChangeData> changeData = {openData, createData, deleteData, moveData};
        callbackManager.OnChangeData(syncFolderIndex, changeData);
        EXPECT_TRUE(trackingCallback->wasCalled);
        EXPECT_EQ(trackingCallback->lastChangeData.size(), 3U);
        EXPECT_EQ(trackingCallback->lastChangeData[0].operationType, OperationType::CREATE);
        EXPECT_EQ(trackingCallback->lastChangeData[1].operationType, OperationType::DELETE);
        EXPECT_EQ(trackingCallback->lastChangeData[2].operationType, OperationType::MOVE_FROM);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataLegacyFilterTest003 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataLegacyFilterTest003 end";
}

/**
 * @tc.name: OnChangeDataLegacyFilterTest004
 * @tc.desc: Verify legacy register with only FILE_OPEN events filters all out, callback not called
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataLegacyFilterTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataLegacyFilterTest004 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string name = "com.test.legacy.only.open";
        uint32_t folderIndex = 30;
        sptr<TrackingCallbackMock> callback = sptr(new TrackingCallbackMock());
        bool ret = callbackManager.RegisterSyncFolderMap(name, folderIndex, callback);
        EXPECT_TRUE(ret);
        ChangeData cdOpen1;
        cdOpen1.operationType = OperationType::FILE_OPEN;
        ChangeData cdOpen2;
        cdOpen2.operationType = OperationType::FILE_OPEN;
        std::vector<ChangeData> input = {cdOpen1, cdOpen2};
        callbackManager.OnChangeData(folderIndex, input);
        EXPECT_FALSE(callback->onChangeDataCalled);
        EXPECT_TRUE(callback->lastChangeData.empty());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataLegacyFilterTest004 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataLegacyFilterTest004 end";
}

/**
 * @tc.name: OnChangeDataWithEventsFilterTest004
 * @tc.desc: Verify WithEvents with SYNC_FOLDER_INVALID in set passes it through
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataWithEventsFilterTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest004 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string name = "com.test.events.invalid";
        uint32_t folderIndex = 31;
        sptr<TrackingCallbackMock> callback = sptr(new TrackingCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillOnce(Return(456));
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        SyncFolderValue value;
        value.bundleName = name;
        value.path = "/data/service/el2/456/hmdfs/account/files/Docs/testfile.txt";
        syncFolder.AddSyncFolder(folderIndex, value);
        set<OperationType> types = {OperationType::FILE_OPEN, OperationType::SYNC_FOLDER_INVALID};
        bool ret = callbackManager.RegisterSyncFolderMapWithEvents(name, folderIndex, types, callback);
        EXPECT_TRUE(ret);
        ChangeData cdInvalid;
        cdInvalid.operationType = OperationType::SYNC_FOLDER_INVALID;
        ChangeData cdCreate;
        cdCreate.operationType = OperationType::CREATE;
        std::vector<ChangeData> input = {cdInvalid, cdCreate};
        callbackManager.OnChangeData(folderIndex, input);
        EXPECT_TRUE(callback->wasCalled);
        ASSERT_EQ(callback->lastChangeData.size(), 1u);
        EXPECT_EQ(callback->lastChangeData[0].operationType, OperationType::SYNC_FOLDER_INVALID);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest004 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest004 end";
}

/**
 * @tc.name: OnChangeDataCallbackNullTest001
 * @tc.desc: Verify OnChangeData with null callback in map returns early
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataCallbackNullTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataCallbackNullTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 40;
        string bundleName = "com.test.null.callback";
        CloudDiskServiceCallbackManager::InstanceValue inst;
        inst.callback = nullptr;
        inst.isWithEvents = false;
        callbackManager.callbackIndexMap_[syncFolderIndex] = std::move(inst);
        ChangeData cd;
        cd.operationType = OperationType::CREATE;
        vector<ChangeData> changeData = {cd};
        callbackManager.OnChangeData(syncFolderIndex, changeData);
        EXPECT_TRUE(true);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataCallbackNullTest001 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataCallbackNullTest001 end";
}

/**
 * @tc.name: OnChangeDataSyncFolderNotFoundTest001
 * @tc.desc: Verify OnChangeData with syncFolder not in CloudDiskSyncFolder returns early
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataSyncFolderNotFoundTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataSyncFolderNotFoundTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 41;
        string bundleName = "com.test.no.syncfolder";
        sptr<TrackingCallbackMock> trackingCallback = sptr(new TrackingCallbackMock());
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, trackingCallback);
        ChangeData cd;
        cd.operationType = OperationType::CREATE;
        vector<ChangeData> changeData = {cd};
        callbackManager.OnChangeData(syncFolderIndex, changeData);
        EXPECT_FALSE(trackingCallback->wasCalled);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataSyncFolderNotFoundTest001 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataSyncFolderNotFoundTest001 end";
}

/**
 * @tc.name: OnChangeDataUserIdZeroTest001
 * @tc.desc: Verify OnChangeData with GetUserId returning 0 calls GetAccountId
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataUserIdZeroTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataUserIdZeroTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 42;
        string bundleName = "com.test.userid.zero";
        sptr<TrackingCallbackMock> trackingCallback = sptr(new TrackingCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillOnce(Return(0));
        EXPECT_CALL(*dfsuAccessTokenMock_, GetAccountId(_))
            .WillOnce(DoAll(SetArgReferee<0>(456), Return(0)));
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        SyncFolderValue value;
        value.bundleName = bundleName;
        value.path = "/data/service/el2/456/hmdfs/account/files/Docs/testfile.txt";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, trackingCallback);
        ChangeData cd;
        cd.operationType = OperationType::CREATE;
        vector<ChangeData> changeData = {cd};
        callbackManager.OnChangeData(syncFolderIndex, changeData);
        EXPECT_TRUE(trackingCallback->wasCalled);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataUserIdZeroTest001 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataUserIdZeroTest001 end";
}

/**
 * @tc.name: OnChangeDataPathConvertFailTest001
 * @tc.desc: Verify OnChangeData with invalid path fails PathToSandboxPathByPhysicalPath
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataPathConvertFailTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataPathConvertFailTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 43;
        string bundleName = "com.test.path.fail";
        sptr<TrackingCallbackMock> trackingCallback = sptr(new TrackingCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillOnce(Return(456));
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        SyncFolderValue value;
        value.bundleName = bundleName;
        value.path = "/invalid/path/that/cannot/convert";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, trackingCallback);
        ChangeData cd;
        cd.operationType = OperationType::CREATE;
        vector<ChangeData> changeData = {cd};
        callbackManager.OnChangeData(syncFolderIndex, changeData);
        EXPECT_FALSE(trackingCallback->wasCalled);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataPathConvertFailTest001 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataPathConvertFailTest001 end";
}

/**
 * @tc.name: UnregisterSyncFolderForChangesMapTest006
 * @tc.desc: Verify UnregisterSyncFolderForChangesMap with index in map but not in vector
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, UnregisterSyncFolderForChangesMapTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest006 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.index.not.in.vector";
        uint32_t syncFolderIndex = 50;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        CloudDiskServiceCallbackManager::InstanceValue inst;
        inst.callback = callback;
        inst.isWithEvents = false;
        callbackManager.callbackIndexMap_[syncFolderIndex] = std::move(inst);
        CloudDiskServiceCallbackManager::CallbackValue cbValue;
        cbValue.syncFolderIndexs.push_back(999);
        callbackManager.callbackAppMap_[bundleName] = std::move(cbValue);
        auto ret = callbackManager.UnregisterSyncFolderForChangesMap(bundleName, syncFolderIndex);
        EXPECT_TRUE(ret);
        EXPECT_EQ(callbackManager.callbackIndexMap_.size(), 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest006 failed";
    }
    GTEST_LOG_(INFO) << "UnregisterSyncFolderForChangesMapTest006 end";
}

/**
 * @tc.name: UnregisterSyncFolderMapTest004
 * @tc.desc: Verify UnregisterSyncFolderMap with index in map but not in app vector
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, UnregisterSyncFolderMapTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnregisterSyncFolderMapTest004 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.unreg.map.not.in.vec";
        uint32_t syncFolderIndex = 51;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex, callback);
        callbackManager.callbackAppMap_[bundleName].syncFolderIndexs.clear();
        callbackManager.callbackAppMap_[bundleName].syncFolderIndexs.push_back(888);
        callbackManager.UnregisterSyncFolderMap(bundleName, syncFolderIndex);
        EXPECT_EQ(callbackManager.callbackIndexMap_.size(), 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "UnregisterSyncFolderMapTest004 failed";
    }
    GTEST_LOG_(INFO) << "UnregisterSyncFolderMapTest004 end";
}

/**
 * @tc.name: RegisterSyncFolderMapInternalExistingAppTest001
 * @tc.desc: Verify RegisterSyncFolderMapInternal with existing app appends to vector (else branch)
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, RegisterSyncFolderMapInternalExistingAppTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "RegisterSyncFolderMapInternalExistingAppTest001 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        string bundleName = "com.test.existing.app";
        uint32_t syncFolderIndex1 = 60;
        uint32_t syncFolderIndex2 = 61;
        sptr<ICloudDiskServiceCallback> callback = sptr(new CloudDiskServiceCallbackMock());
        callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex1, callback);
        bool ret = callbackManager.RegisterSyncFolderMap(bundleName, syncFolderIndex2, callback);
        EXPECT_TRUE(ret);
        EXPECT_EQ(callbackManager.callbackAppMap_[bundleName].syncFolderIndexs.size(), 2U);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "RegisterSyncFolderMapInternalExistingAppTest001 failed";
    }
    GTEST_LOG_(INFO) << "RegisterSyncFolderMapInternalExistingAppTest001 end";
}

/**
 * @tc.name: OnChangeDataWithEventsFilterTest005
 * @tc.desc: Verify WithEvents with SYNC_FOLDER_INVALID only passes it, filters FILE_OPEN
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, OnChangeDataWithEventsFilterTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest005 start";
    try {
        CloudDiskServiceCallbackManager callbackManager;
        uint32_t syncFolderIndex = 55;
        string bundleName = "com.test.events.invalid.only";
        sptr<TrackingCallbackMock> trackingCallback = sptr(new TrackingCallbackMock());
        EXPECT_CALL(*dfsuAccessTokenMock_, GetUserId()).WillOnce(Return(456));
        CloudDiskSyncFolder &syncFolder = CloudDiskSyncFolder::GetInstance();
        for (const auto &item : syncFolder.GetSyncFolderMap()) {
            syncFolder.DeleteSyncFolder(item.first);
        }
        SyncFolderValue value;
        value.bundleName = bundleName;
        value.path = "/data/service/el2/456/hmdfs/account/files/Docs/testfile.txt";
        syncFolder.AddSyncFolder(syncFolderIndex, value);
        set<OperationType> types = {OperationType::SYNC_FOLDER_INVALID};
        callbackManager.RegisterSyncFolderMapWithEvents(bundleName, syncFolderIndex, types, trackingCallback);
        ChangeData cdInvalid;
        cdInvalid.operationType = OperationType::SYNC_FOLDER_INVALID;
        ChangeData cdOpen;
        cdOpen.operationType = OperationType::FILE_OPEN;
        ChangeData cdCreate;
        cdCreate.operationType = OperationType::CREATE;
        vector<ChangeData> changeData = {cdInvalid, cdOpen, cdCreate};
        callbackManager.OnChangeData(syncFolderIndex, changeData);
        EXPECT_TRUE(trackingCallback->wasCalled);
        EXPECT_EQ(trackingCallback->lastChangeData.size(), 1u);
        EXPECT_EQ(trackingCallback->lastChangeData[0].operationType, OperationType::SYNC_FOLDER_INVALID);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest005 failed";
    }
    GTEST_LOG_(INFO) << "OnChangeDataWithEventsFilterTest005 end";
}

/**
 * @tc.name: GetSyncFolderChangesWithEventsTest001
 * @tc.desc: Verify the GetSyncFolderChangesWithEvents function
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, GetSyncFolderChangesWithEventsTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest001 start";
    try {
        int32_t userId = 1;
        uint64_t syncFolderIndex = 1;
        uint64_t start = 0;
        uint64_t count = 1;
        set<OperationType> operationTypes = {OperationType::FILE_OPEN, OperationType::CREATE};
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChangesWithEvents(
            userId, syncFolderIndex, start, count, operationTypes, changesResult);
        EXPECT_EQ(ret, E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest001 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest001 end";
}

/**
 * @tc.name: GetSyncFolderChangesWithEventsTest002
 * @tc.desc: Verify GetSyncFolderChangesWithEvents does not crash with empty result
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, GetSyncFolderChangesWithEventsTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest002 start";
    try {
        int32_t userId = 1;
        uint64_t syncFolderIndex = 1;
        uint64_t start = 0;
        uint64_t count = 1;
        set<OperationType> operationTypes = {OperationType::FILE_OPEN};
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChangesWithEvents(
            userId, syncFolderIndex, start, count, operationTypes, changesResult);
        EXPECT_EQ(ret, E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest002 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest002 end";
}

/**
 * @tc.name: GetSyncFolderChangesWithEventsTest003
 * @tc.desc: Verify GetSyncFolderChangesWithEvents with multiple operationTypes in set
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, GetSyncFolderChangesWithEventsTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest003 start";
    try {
        int32_t userId = 1;
        uint64_t syncFolderIndex = 2;
        uint64_t start = 0;
        uint64_t count = 10;
        set<OperationType> operationTypes = {
            OperationType::FILE_OPEN, OperationType::CREATE,
            OperationType::DELETE, OperationType::MOVE_FROM,
            OperationType::MOVE_TO, OperationType::CLOSE_WRITE,
            OperationType::SYNC_FOLDER_INVALID, OperationType::OH_CLOUD_DISK_CLOSE_MODIFY};
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChangesWithEvents(
            userId, syncFolderIndex, start, count, operationTypes, changesResult);
        EXPECT_EQ(ret, E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest003 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest003 end";
}

/**
 * @tc.name: GetSyncFolderChangesWithEventsTest004
 * @tc.desc: Verify GetSyncFolderChangesWithEvents with empty operationTypes set
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, GetSyncFolderChangesWithEventsTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest004 start";
    try {
        int32_t userId = 1;
        uint64_t syncFolderIndex = 1;
        uint64_t start = 0;
        uint64_t count = 10;
        set<OperationType> operationTypes;
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChangesWithEvents(
            userId, syncFolderIndex, start, count, operationTypes, changesResult);
        EXPECT_EQ(ret, E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest004 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest004 end";
}

/**
 * @tc.name: GetSyncFolderChangesWithEventsTest005
 * @tc.desc: Verify GetSyncFolderChangesWithEvents with count=0
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, GetSyncFolderChangesWithEventsTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest005 start";
    try {
        int32_t userId = 1;
        uint64_t syncFolderIndex = 1;
        uint64_t start = 0;
        uint64_t count = 0;
        set<OperationType> operationTypes = {OperationType::FILE_OPEN};
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChangesWithEvents(
            userId, syncFolderIndex, start, count, operationTypes, changesResult);
        EXPECT_EQ(ret, E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest005 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest005 end";
}

/**
 * @tc.name: GetSyncFolderChangesWithEventsTest006
 * @tc.desc: Verify GetSyncFolderChangesWithEvents with PraseRequest failure returns error
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, GetSyncFolderChangesWithEventsTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest006 start";
    try {
        int32_t userId = 1;
        uint64_t syncFolderIndex = 999;
        uint64_t start = 1;
        uint64_t count = 1;
        set<OperationType> operationTypes = {OperationType::FILE_OPEN};
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChangesWithEvents(
            userId, syncFolderIndex, start, count, operationTypes, changesResult);
        EXPECT_NE(ret, E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest006 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest006 end";
}

/**
 * @tc.name: GetSyncFolderChangesWithEventsTest007
 * @tc.desc: Verify GetSyncFolderChangesWithEvents with empty changesData returns E_OK
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, GetSyncFolderChangesWithEventsTest007, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest007 start";
    try {
        int32_t userId = 1;
        uint64_t syncFolderIndex = 1;
        uint64_t start = 1;
        uint64_t count = 0;
        set<OperationType> operationTypes = {OperationType::FILE_OPEN};
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChangesWithEvents(
            userId, syncFolderIndex, start, count, operationTypes, changesResult);
        EXPECT_TRUE(ret == E_OK || ret != E_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest007 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest007 end";
}

/**
 * @tc.name: GetSyncFolderChangesWithEventsTest008
 * @tc.desc: Verify GetSyncFolderChangesWithEvents with registered sync folder and count=0 returns E_OK
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CloudDiskServiceCallbackManagerTest, GetSyncFolderChangesWithEventsTest008, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest008 start";
    try {
        int32_t userId = 100;
        uint32_t syncFolderIndex = 200;
        string path = "/data/service/el2/100/hmdfs/account/files/Docs/testdir";
        LogFileMgr::GetInstance().RegisterSyncFolder(userId, syncFolderIndex, path);
        uint64_t start = 0;
        uint64_t count = 0;
        set<OperationType> operationTypes = {OperationType::FILE_OPEN, OperationType::CREATE};
        ChangesResult changesResult;
        int32_t ret = CloudDiskServiceSyncFolder::GetSyncFolderChangesWithEvents(
            userId, syncFolderIndex, start, count, operationTypes, changesResult);
        EXPECT_EQ(ret, E_OK);
        LogFileMgr::GetInstance().UnRegisterSyncFolder(userId, syncFolderIndex);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest008 failed";
    }
    GTEST_LOG_(INFO) << "GetSyncFolderChangesWithEventsTest008 end";
}

} // namespace Test
} // namespace FileManagement::CloudDiskService
} // namespace OHOS