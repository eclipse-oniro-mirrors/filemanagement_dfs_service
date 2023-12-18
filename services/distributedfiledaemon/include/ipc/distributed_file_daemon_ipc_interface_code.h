/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

/* SAID:5201 */
namespace OHOS {
namespace Storage {
namespace DistributedFile {
    enum class DistributedFileDaemonInterfaceCode {
        DISTRIBUTED_FILE_OPEN_P2P_CONNECTION = 0,
        DISTRIBUTED_FILE_CLOSE_P2P_CONNECTION,
        DISTRIBUTED_FILE_PREPARE_SESSION,
        DISTRIBUTED_FILE_REQUEST_SEND_FILE,
    };
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS