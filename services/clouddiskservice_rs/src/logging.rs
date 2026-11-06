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

//! Local logging entry point shared by every framework module.

// Untagged arguments are private, matching HiLog. Use @public(...) for values
// that should remain visible regardless of the platform privacy switch.
macro_rules! service_log {
    (@emit $level:ident, $($arg:tt)*) => {{
        use hilog_rust::{hilog, HiLogLabel, LogType};
        use std::ffi::{c_char, CString};
        const LABEL: HiLogLabel = HiLogLabel {
            log_type: LogType::LogCore,
            domain: 0xD004308,
            tag: "CLOUDDISKSERVICE_SA_RS",
        };
        // HiLog uses printf formatting; keep '%' in parameter values literal.
        let message = format!($($arg)*).replace('%', "%%");
        hilog_rust::$level!(LABEL, "{}", @public(message));
    }};
    (@args $level:ident, $fmt:literal, $private:ident; ($($parsed:expr,)*);) => {{
        service_log!(@emit $level, $fmt $(, $parsed)*);
    }};
    (@args $level:ident, $fmt:literal, $private:ident; ($($parsed:expr,)*);
        @private($arg:expr) $(, $($rest:tt)*)?) => {{
        if $private {
            service_log!(@args $level, $fmt, $private;
                ($($parsed,)* "<private>",); $($($rest)*)?);
        } else {
            service_log!(@args $level, $fmt, $private;
                ($($parsed,)* $arg,); $($($rest)*)?);
        }
    }};
    (@args $level:ident, $fmt:literal, $private:ident; ($($parsed:expr,)*);
        @public($arg:expr) $(, $($rest:tt)*)?) => {{
        service_log!(@args $level, $fmt, $private;
            ($($parsed,)* $arg,); $($($rest)*)?);
    }};
    (@args $level:ident, $fmt:literal, $private:ident; ($($parsed:expr,)*);
        $arg:expr $(, $($rest:tt)*)?) => {{
        service_log!(@args $level, $fmt, $private;
            ($($parsed,)*); @private($arg) $(, $($rest)*)?);
    }};
    ($level:ident, $fmt:literal $(,)?) => {{
        service_log!(@emit $level, $fmt);
    }};
    ($level:ident, $fmt:literal, $($arg:tt)+) => {{
        // Match hilog_rust: debug mode or a disabled privacy switch shows values.
        // SAFETY: Both C ABI functions take no arguments and return bool flags.
        // HiLog owns the property state, so neither call needs caller-provided memory.
        let _private = unsafe {
            hilog_rust::IsPrivateSwitchOn() && !hilog_rust::IsDebugOn()
        };
        service_log!(@args $level, $fmt, _private; (); $($arg)+);
    }};
}

#[allow(unused_macros)]
macro_rules! debug { ($($arg:tt)*) => {{ service_log!(debug, $($arg)*); }}; }
macro_rules! info { ($($arg:tt)*) => {{ service_log!(info, $($arg)*); }}; }
#[allow(unused_macros)]
macro_rules! warn { ($($arg:tt)*) => {{ service_log!(warn, $($arg)*); }}; }
macro_rules! error { ($($arg:tt)*) => {{ service_log!(error, $($arg)*); }}; }
#[allow(unused_macros)]
macro_rules! fatal { ($($arg:tt)*) => {{ service_log!(fatal, $($arg)*); }}; }
