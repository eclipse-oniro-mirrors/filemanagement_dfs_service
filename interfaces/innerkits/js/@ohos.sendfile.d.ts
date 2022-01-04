/*
* Copyright (C) 2021 Huawei Device Co., Ltd.
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

import {AsyncCallback, Callback} from "./basic";

declare namespace SendFile {
   function SendFile(deviceId: number, sourPath: Array<string>, destPath: Array<string>, fileCount: number, callback: AsyncCallback<number>);
   function SendFile(deviceId: number, sourPath: Array<string>, destPath: Array<string>, fileCount: number): Promise<number>;

   /**
     * Send and recv file event loop type
     */
   export interface CloseOptions {
    sendFile: number;
    recvFile: number;
   };

   on(type: 'finished', CloseOptions: options, callback: AsyncCallback<Object>): void;
   off(type: 'finished', callback?: AsyncCallback<Object>): void;
}

export default SendFile;