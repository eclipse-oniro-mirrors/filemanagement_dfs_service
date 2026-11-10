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

#ifndef DECOMPRESS_MOCK_H
#define DECOMPRESS_MOCK_H

#include <cstdint>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace DecompressMock {

struct State {
    bool openEnabled = false;
    int openReturnFd = -1;
    int openErrno = 0;
    int openCallCount = 0;
    int openFailAfterCount = -1;

    bool closeEnabled = false;
    int closeReturnVal = 0;

    bool ioctlEnabled = false;
    int ioctlReturnVal = -1;
    int ioctlErrno = 0;
    int ioctlCallCount = 0;
    int ioctlSuccessAfterCount = -1;
    int ioctlRetryErrno = 0;

    bool realpathEnabled = false;
    bool realpathFail = false;

    bool dlopenEnabled = false;
    bool dlopenSucceed = false;
    void *dlopenHandle = reinterpret_cast<void *>(0x12345678);

    bool dlsymEnabled = false;
    bool dlsymSucceed = false;
    int32_t unsupportedListRet = 0;
    std::vector<std::string> unsupportedList;
    int32_t systemFeatureRet = 0;
    bool systemFeatureValue = false;
    int32_t crossFsConfigRet = 0;
    bool crossFsConfigValue = false;

    bool statEnabled = false;
    bool statFail = false;
    dev_t statHapDev = 1;
    dev_t statDstDev = 1;
    int statCallCount = 0;

    bool statfsEnabled = false;
    bool statfsFail = false;
    unsigned long statfsType = 0xF2F52010; /* HMFS_SUPER_MAGIC */
    uint64_t statfsFrsize = 0;
    uint64_t statfsBsize = 4096;
    uint64_t statfsBlocks = 0;
    uint64_t statfsBavail = 0;

    bool accessEnabled = false;
    bool accessFail = false;

    bool mkdirEnabled = false;
    bool mkdirFail = false;

    bool chownEnabled = false;
    bool chownFail = false;

    bool fchownEnabled = false;
    bool fchownFail = false;

    bool flockEnabled = false;
    bool flockFail = false;

    bool lseekEnabled = false;
    bool lseekFail = false;

    bool ftruncateEnabled = false;
    bool ftruncateFail = false;

    bool writeEnabled = false;
    int writeFailAfterCount = -1;
    int writeCallCount = 0;
    int writeIntrBeforeCount = -1;

    bool fopenEnabled = false;
    bool fopenSucceed = false;
    bool fcloseFail = false;
    FILE *fopenHandle = reinterpret_cast<FILE *>(0xDEADBEEF);

    bool getlineEnabled = false;
    bool getlineSucceed = false;
    ssize_t getlineRetVal = 1;
    char getlineContent[256] = "1\n";

    bool fgetsEnabled = false;
    bool fgetsSucceed = true;
    char fgetsContent[256] = "test_proc\n";
};

extern State g_mockState;

void Reset();

} // namespace DecompressMock

#endif // DECOMPRESS_MOCK_H