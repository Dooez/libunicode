// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string_view>

// clang-format off
#if __has_include(<experimental/simd>) && defined(LIBUNICODE_USE_STD_SIMD) && !defined(__APPLE__) && !defined(__FreeBSD__)
    #define USE_STD_SIMD
    #include <experimental/simd>
    namespace stdx = std::experimental;
#elif __has_include(<simd>) && defined(LIBUNICODE_USE_STD_SIMD)
    #define USE_STD_SIMD
    #include <simd>
    namespace stdx = std;
#elif defined(LIBUNICODE_USE_INTRINSICS)
    #include "intrinsics.h"
#endif
// clang-format on
namespace unicode::detail
{
template <size_t SimdBitWidth>
size_t scan_for_text_ascii_simd(std::string_view text, size_t maxColumnCount) noexcept
{
    [[maybe_unused]] constexpr int simd_size = SimdBitWidth / 8;
    auto input = text.data();
    auto const end = text.data() + std::min(text.size(), maxColumnCount);

#if defined(USE_STD_SIMD)
    auto simd_text = stdx::fixed_size_simd<char, simd_size> {};
    while (input < end - simd_size)
    {
        simd_text.copy_from(input, stdx::element_aligned);
        auto const is_control_mask = simd_text < 0x20;
        auto const is_complex_mask = (simd_text & 0x80) == 0x80;
        auto const ctrl_or_complex_mask = is_control_mask || is_complex_mask;
        if (stdx::any_of(ctrl_or_complex_mask))
        {
            input += stdx::find_first_set(ctrl_or_complex_mask);
            break;
        }
        input += simd_size;
    }
#elif defined(LIBUNICODE_USE_INTRINSICS)
    constexpr auto trailing_zero_count = []<typename T>(T value) noexcept {
        // clang-format off
        if constexpr (std::same_as<std::remove_cvref_t<T>, uint32_t>)
        {
            #if defined(_WIN32)
                // return _tzcnt_u32(value);
                // Don't do _tzcnt_u32, because that's only available on x86-64, but not on ARM64.
                unsigned long r = 0;
                _BitScanForward(&r, value);
                return r;
            #else
                return __builtin_ctz(value);
            #endif
        }
        else
        {
            #if defined(_WIN32)
                unsigned long r = 0;
                _BitScanForward64(&r, value);
                return r;
            #else
                return __builtin_ctzl(value);
            #endif
        }
        // clang-format on
    };
    using intrinsics = intrinsics<SimdBitWidth>;
    auto const vec_control = intrinsics::set1_epi8(0x20); // 0..0x1F
    auto const vec_complex = intrinsics::set1_epi8(-128); // equals to 0x80 (0b1000'0000)

    while (input < end - simd_size)
    {
        auto const batch = intrinsics::load(input);
        auto const is_control_mask = intrinsics::less(batch, vec_control);
        auto const is_complex_mask = intrinsics::equal(intrinsics::and_vec(batch, vec_complex), vec_complex);
        auto const ctrl_or_complex_mask = intrinsics::or_mask(is_control_mask, is_complex_mask);
        if (ctrl_or_complex_mask)
        {
            int const advance = trailing_zero_count(intrinsics::to_unsigned(ctrl_or_complex_mask));
            input += advance;
            break;
        }
        input += sizeof(simd_size);
    }
#endif

    constexpr auto is_ascii = [](char ch) noexcept {
        auto const is_control = static_cast<uint8_t>(ch) < 0x20;
        auto const is_complex = static_cast<uint8_t>(ch) & 0x80;
        return !is_control && !is_complex;
    };
    while (input != end && is_ascii(*input))
        ++input;

    return static_cast<size_t>(std::distance(text.data(), input));
}
} // namespace unicode::detail
