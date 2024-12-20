/**
 * This file is part of the "libunicode" project
 *   Copyright (c) 2023 Christian Parpart <christian@parpart.family>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <cstdint>
#if defined(__x86_64__) || defined(_M_AMD64)
    #include <immintrin.h>
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
    #include <arm_neon.h>
#endif

namespace unicode
{

template <size_t SimdBitWidth, typename = void>
struct intrinsics;

#if defined(__GNUC__) && defined(__x86_64__)
    // For some reason, GCC associates attributes with __m128i that are not obvious (alignment),
    // and then complains about it when used below.
    #pragma GCC diagnostic ignored "-Wignored-attributes"
#endif

#if defined(__x86_64__) || defined(_M_AMD64) // {{{

template <typename T>
struct intrinsics<128, T>
{
    using vec_t = __m128i;

    using mask_t = int;

    static inline vec_t setzero() noexcept { return _mm_setzero_si128(); }

    static inline vec_t set1_epi8(signed char w) { return _mm_set1_epi8(w); }

    static inline vec_t xor_vec(vec_t a, vec_t b) noexcept { return _mm_xor_si128(a, b); }

    static inline vec_t and_vec(vec_t a, vec_t b) noexcept { return _mm_and_si128(a, b); }

    static inline vec_t or_vec(vec_t a, vec_t b) { return _mm_or_si128(a, b); }

    static inline vec_t load(const char* p) noexcept { return _mm_loadu_si128(reinterpret_cast<const vec_t*>(p)); }

    static inline bool equal(vec_t a, vec_t b) noexcept { return _mm_movemask_epi8(_mm_cmpeq_epi32(a, b)) == 0xFFFF; }

    static inline mask_t less(vec_t a, vec_t b) noexcept { return _mm_movemask_epi8(_mm_cmplt_epi8(a, b)); }

    static inline mask_t greater(vec_t a, vec_t b) noexcept { return _mm_movemask_epi8(_mm_cmpgt_epi8(a, b)); }

    static inline mask_t and_mask(mask_t a, mask_t b) noexcept { return a & b; }

    static inline mask_t or_mask(mask_t a, mask_t b) noexcept { return a | b; }

    static inline mask_t xor_mask(mask_t a, mask_t b) noexcept { return a ^ b; }

    static inline uint32_t to_unsigned(mask_t a) noexcept { return static_cast<uint32_t>(a); }
};

template <typename T>
struct intrinsics<256, T>
{
    using vec_t = __m256i;

    using mask_t = int;

    static inline vec_t setzero() noexcept { return _mm256_setzero_si256(); }

    static inline vec_t set1_epi8(signed char w) { return _mm256_set1_epi8(w); }

    static inline vec_t xor_vec(vec_t a, vec_t b) noexcept { return _mm256_xor_si256(a, b); }

    static inline vec_t and_vec(vec_t a, vec_t b) noexcept { return _mm256_and_si256(a, b); }

    static inline vec_t or_vec(vec_t a, vec_t b) { return _mm256_or_si256(a, b); }

    static inline vec_t load(const char* p) noexcept { return _mm256_loadu_si256(reinterpret_cast<const vec_t*>(p)); }

    static inline bool equal(vec_t a, vec_t b) noexcept { return _mm256_movemask_epi8(_mm256_cmpeq_epi32(a, b)) == 0xFFFF; }

    static inline mask_t less(vec_t a, vec_t b) noexcept { return _mm256_movemask_epi8(_mm256_cmpgt_epi8(b, a)); }

    static inline mask_t greater(vec_t a, vec_t b) noexcept { return _mm256_movemask_epi8(_mm256_cmpgt_epi8(a, b)); }

    static inline auto movemask_epi8(vec_t a) noexcept { return _mm256_movemask_epi8(a); }

    static inline mask_t and_mask(mask_t a, mask_t b) noexcept { return a & b; }

    static inline mask_t or_mask(mask_t a, mask_t b) noexcept { return a | b; }

    static inline mask_t xor_mask(mask_t a, mask_t b) noexcept { return a ^ b; }

    static inline uint32_t to_unsigned(mask_t a) noexcept { return static_cast<uint32_t>(a); }
};

template <typename T>
struct intrinsics<512, T>
{
    using vec_t = __m512i;

    using mask_t = __mmask64;

    static inline vec_t setzero() noexcept { return _mm512_setzero_si512(); }

    static inline vec_t set1_epi8(signed char w) { return _mm512_set1_epi8(w); }

    static inline vec_t xor_vec(vec_t a, vec_t b) noexcept { return _mm512_xor_si512(a, b); }

    static inline vec_t and_vec(vec_t a, vec_t b) noexcept { return _mm512_and_si512(a, b); }

    static inline vec_t or_vec(vec_t a, vec_t b) { return _mm512_or_si512(a, b); }

    static inline vec_t load(const char* p) noexcept { return _mm512_loadu_si512(reinterpret_cast<const vec_t*>(p)); }

    static inline bool equal(vec_t a, vec_t b) noexcept { return _mm512_cmpeq_epi8_mask(a, b) == 0xFFFFFFFF; }

    static inline mask_t less(vec_t a, vec_t b) noexcept { return _mm512_cmplt_epi8_mask(a, b); }

    static inline mask_t greater(vec_t a, vec_t b) noexcept { return _mm512_cmpgt_epi8_mask(a, b); }

    static inline mask_t and_mask(mask_t a, mask_t b) noexcept { return _kand_mask64(a, b); }

    static inline mask_t or_mask(mask_t a, mask_t b) noexcept { return _kor_mask64(a, b); }

    static inline mask_t xor_mask(mask_t a, mask_t b) noexcept { return _kxor_mask64(a, b); }

    static inline uint64_t to_unsigned(mask_t a) noexcept { return static_cast<uint64_t>(a); }
};

#endif
// }}}

#if defined(__aarch64__) || defined(_M_ARM64) // {{{

template <typename T>
struct intrinsics<128, T>
{
    // The following inline functions (in its initial version) were borrowed from:
    // https://github.com/f1ed/emp/blob/master/emp-tool/utils/block.h

    using vec_t = int64x2_t;

    using mask_t = int;

    static inline vec_t setzero() noexcept { return vreinterpretq_s64_s32(vdupq_n_s32(0)); }

    static inline vec_t set1_epi8(signed char w) { return vreinterpretq_s64_s8(vdupq_n_s8(w)); }

    static inline vec_t xor_vec(vec_t a, vec_t b) noexcept
    {
        // Computes the bitwise XOR of the 128-bit value in a and the 128-bit value in
        // b.  https://msdn.microsoft.com/en-us/library/fzt08www(v=vs.100).aspx
        return vreinterpretq_s64_s32(veorq_s32(vreinterpretq_s32_s64(a), vreinterpretq_s32_s64(b)));
    }

    static inline vec_t and_vec(vec_t a, vec_t b) noexcept
    {
        return vreinterpretq_s64_s32(vandq_s32(vreinterpretq_s32_s64(a), vreinterpretq_s32_s64(b)));
    }

    static inline vec_t or_vec(vec_t a, vec_t b)
    {
        return vreinterpretq_s64_s32(vorrq_s32(vreinterpretq_s32_s64(a), vreinterpretq_s32_s64(b)));
    }

    static inline vec_t load(const char* p) noexcept
    {
        return vreinterpretq_s64_s32(vld1q_s32(reinterpret_cast<const int32_t*>(p)));
    }

    static inline bool equal(vec_t a, vec_t b) noexcept
    {
        return movemask_epi8(vreinterpretq_s64_u32(vceqq_s32(vreinterpretq_s32_s64(a), vreinterpretq_s32_s64(b)))) == 0xFFFF;
    }

    static inline mask_t less(vec_t a, vec_t b) noexcept
    {
        return movemask_epi8(vreinterpretq_s64_u8(vcltq_s8(vreinterpretq_s8_s64(a), vreinterpretq_s8_s64(b))));
    }

    static inline mask_t greater(vec_t a, vec_t b) noexcept { return less(b, a); }

    static inline mask_t and_mask(mask_t a, mask_t b) noexcept { return a & b; }

    static inline mask_t or_mask(mask_t a, mask_t b) noexcept { return a | b; }

    static inline mask_t xor_mask(mask_t a, mask_t b) noexcept { return a ^ b; }

    static inline uint32_t to_unsigned(mask_t a) noexcept { return static_cast<uint32_t>(a); }

    static inline mask_t movemask_epi8(vec_t a)
    {
        // Use increasingly wide shifts+adds to collect the sign bits
        // together.
        // Since the widening shifts would be rather confusing to follow in little
        // endian, everything will be illustrated in big endian order instead. This
        // has a different result - the bits would actually be reversed on a big
        // endian machine.

        // Starting input (only half the elements are shown):
        // 89 ff 1d c0 00 10 99 33
        uint8x16_t input = vreinterpretq_u8_s64(a);

        // Shift out everything but the sign bits with an unsigned shift right.
        //
        // Bytes of the vector::
        // 89 ff 1d c0 00 10 99 33
        // \  \  \  \  \  \  \  \    high_bits = (uint16x4_t)(input >> 7)
        //  |  |  |  |  |  |  |  |
        // 01 01 00 01 00 00 01 00
        //
        // Bits of first important lane(s):
        // 10001001 (89)
        // \______
        //        |
        // 00000001 (01)
        uint16x8_t high_bits = vreinterpretq_u16_u8(vshrq_n_u8(input, 7));

        // Merge the even lanes together with a 16-bit unsigned shift right + add.
        // 'xx' represents garbage data which will be ignored in the final result.
        // In the important bytes, the add functions like a binary OR.
        //
        // 01 01 00 01 00 00 01 00
        //  \_ |  \_ |  \_ |  \_ |   paired16 = (uint32x4_t)(input + (input >> 7))
        //    \|    \|    \|    \|
        // xx 03 xx 01 xx 00 xx 02
        //
        // 00000001 00000001 (01 01)
        //        \_______ |
        //                \|
        // xxxxxxxx xxxxxx11 (xx 03)
        uint32x4_t paired16 = vreinterpretq_u32_u16(vsraq_n_u16(high_bits, high_bits, 7));

        // Repeat with a wider 32-bit shift + add.
        // xx 03 xx 01 xx 00 xx 02
        //     \____ |     \____ |  paired32 = (uint64x1_t)(paired16 + (paired16 >>
        //     14))
        //          \|          \|
        // xx xx xx 0d xx xx xx 02
        //
        // 00000011 00000001 (03 01)
        //        \\_____ ||
        //         '----.\||
        // xxxxxxxx xxxx1101 (xx 0d)
        uint64x2_t paired32 = vreinterpretq_u64_u32(vsraq_n_u32(paired16, paired16, 14));

        // Last, an even wider 64-bit shift + add to get our result in the low 8 bit
        // lanes. xx xx xx 0d xx xx xx 02
        //            \_________ |   paired64 = (uint8x8_t)(paired32 + (paired32 >>
        //            28))
        //                      \|
        // xx xx xx xx xx xx xx d2
        //
        // 00001101 00000010 (0d 02)
        //     \   \___ |  |
        //      '---.  \|  |
        // xxxxxxxx 11010010 (xx d2)
        uint8x16_t paired64 = vreinterpretq_u8_u64(vsraq_n_u64(paired32, paired32, 28));

        // Extract the low 8 bits from each 64-bit lane with 2 8-bit extracts.
        // xx xx xx xx xx xx xx d2
        //                      ||  return paired64[0]
        //                      d2
        // Note: Little endian would return the correct value 4b (01001011) instead.
        return vgetq_lane_u8(paired64, 0) | ((int) vgetq_lane_u8(paired64, 8) << 8);
    }
};
#endif
// }}}

// #if defined(INTRINSICS_HAS_ARM64_NEON)
// using m128i = int64x2_t; // 128-bit vector containing integers
// #else
// using m128i = __m128i;
// #endif

} // namespace unicode
