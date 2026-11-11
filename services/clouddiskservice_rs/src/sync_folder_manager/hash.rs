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

/// CloudFileUtils::DentryHash with its default caseSense = false.
pub(crate) fn dentry_hash(input: impl AsRef<[u8]>) -> u32 {
    let input = input.as_ref();
    if input == b"." || input == b".." {
        return 0;
    }
    let mut state = [0x67452301_u32, 0xefcdab89, 0x98badcfe, 0x10325476];
    let mut remaining = input;
    // C++ transforms one block even for an empty input.
    loop {
        let words = str2hash_buf(remaining);
        tea_transform(&mut state, &words);
        if remaining.len() <= 16 {
            break;
        }
        remaining = &remaining[16..];
    }
    // The C++ mask clears bit 63 of a u64, so all 32 result bits are retained.
    state[0]
}

fn str2hash_buf(input: &[u8]) -> [u32; 4] {
    // Padding uses the remaining byte length, before limiting this block to 16.
    let length = input.len() as u32;
    let mut pad = length | (length << 8);
    pad |= pad << 16;
    let mut words = [pad; 4];
    for (index, byte) in input.iter().take(16).enumerate() {
        let word = &mut words[index / 4];
        *word = (*word << 8).wrapping_add(u32::from(byte.to_ascii_lowercase()));
    }
    words
}

fn tea_transform(state: &mut [u32; 4], input: &[u32; 4]) {
    const DELTA: u32 = 0x9e3779b9;
    let [a, b, c, d] = *input;
    let mut b0 = state[0];
    let mut b1 = state[1];
    let mut sum = 0_u32;
    for _ in 0..16 {
        sum = sum.wrapping_add(DELTA);
        b0 = b0.wrapping_add(
            (b1 << 4).wrapping_add(a) ^ b1.wrapping_add(sum) ^ (b1 >> 5).wrapping_add(b),
        );
        b1 = b1.wrapping_add(
            (b0 << 4).wrapping_add(c) ^ b0.wrapping_add(sum) ^ (b0 >> 5).wrapping_add(d),
        );
    }
    state[0] = state[0].wrapping_add(b0);
    state[1] = state[1].wrapping_add(b1);
}
