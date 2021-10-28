/*
* Copyright (c) 2021 Huawei Device Co., Ltd.
d * you may not use this file except in compliance with the License.
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

#include "utils_mount_argument.h"

#include <sstream>
#include <sys/mount.h>

namespace OHOS {
namespace DistributedFile {
namespace Utils {
using namespace std;

string MountArgument::GetFullSrc() const
{
    stringstream ss;
    ss << "/data/misc_ce/" << userId_ << "/hmdfs/storage";
    return ss.str();
}

string MountArgument::GetFullDst() const
{
    stringstream ss;
    ss << "/mnt/hmdfs/" << userId_ << "/";
    return ss.str();
}

string MountArgument::GetCachePath() const
{
    stringstream ss;
    ss << "/data/misc_ce/" << userId_ << "/hmdfs/cache/";
    return ss.str();
}

static uint64_t MocklispHash(const string &str)
{
    uint64_t res = 0;
    constexpr int mocklispHashPos = 5;
    /* Mocklisp hash function. */
    for (auto ch : str) {
        res = (res << mocklispHashPos) - res + (uint64_t)ch;
    }
    return res;
}

string MountArgument::GetCtrlPath() const
{
    auto dst = GetFullDst();
    auto res = MocklispHash(dst);
    stringstream ss;
    ss << "/sys/fs/hmdfs/" << res << "/cmd";
    return ss.str();
}

string MountArgument::OptionsToString() const
{
    stringstream ss;
    ss << "local_dst=" << GetFullDst();
    if (useCache_) {
        ss << ",cache_dir=" << GetCachePath();
    }
    if (caseSensitive_) {
        ss << ",sensitive";
    }
    if (enableMergeView_) {
        ss << ",merge";
    }
    if (enableFixupOwnerShip_) {
        ss << ",fixupownership";
    }
    if (!enableOfflineStash_) {
        ss << ",no_offline_stash";
    }
    if (externalFS_) {
        ss << ",external_fs";
    }
    return ss.str();
}

unsigned long MountArgument::GetFlags() const
{
    return 0;
}

MountArgument MountArgumentDescriptors::Alpha(int userId)
{
    MountArgument mountArgument = {
        .account_ = "default",
        .needInitDir_ = true,
        .useCache_ = true,
        .enableMergeView_ = true,
        .enableFixupOwnerShip_ = true,
        .enableOfflineStash_ = true,
        .externalFS_ = false,
    };
    mountArgument.userId_ = userId;
    return mountArgument;
}
} // namespace Utils
} // namespace DistributedFile
} // namespace OHOS