/**
 * This file is part of the "libunicode" project
 *   Copyright (c) 2020 Christian Parpart <christian@parpart.family>
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
#include <libunicode/grapheme_segmenter.h>
#include <libunicode/intrinsics.h>
#include <libunicode/scan.h>
#include <libunicode/simd_detector.h>
#include <libunicode/utf8.h>
#include <libunicode/width.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <string_view>

#include "scan_simd_impl.hpp"

using std::distance;
using std::get;
using std::holds_alternative;
using std::max;
using std::min;
using std::string_view;

namespace unicode
{

namespace
{
    template <typename T>
    constexpr bool ascending(T low, T val, T high) noexcept
    {
        return low <= val && val <= high;
    }

    constexpr bool is_control(char ch) noexcept
    {
        return static_cast<uint8_t>(ch) < 0x20;
    }

    // Tests if given UTF-8 byte is part of a complex Unicode codepoint, that is, a value greater than U+7E.
    constexpr bool is_complex(char ch) noexcept
    {
        return static_cast<uint8_t>(ch) & 0x80;
    }
} // namespace

size_t detail::scan_for_text_ascii(string_view text, size_t maxColumnCount) noexcept
{
#if defined(__x86_64__) || defined(_M_AMD64)
    static auto simd_size = max_simd_size();
    if (simd_size == 512)
    {
        return scan_for_text_ascii_512(text, maxColumnCount);
    }
    else if (simd_size == 256)
    {
        return scan_for_text_ascii_256(text, maxColumnCount);
    }
#endif
    return scan_for_text_ascii_simd<128>(text, maxColumnCount);
}

scan_result detail::scan_for_text_nonascii(scan_state& state,
                                           string_view text,
                                           size_t maxColumnCount,
                                           grapheme_cluster_receiver& receiver) noexcept
{
    size_t count = 0;

    char const* start = text.data();
    char const* end = start + text.size();
    char const* input = start;
    char const* clusterStart = start;
    char const* lastCodepointStart = start;

    unsigned byteCount = 0; // bytes consume for the current codepoint

    // TODO: move currentClusterWidth to scan_state.
    size_t currentClusterWidth = 0; // current grapheme cluster's East Asian Width

    char const* resultStart = state.utf8.expectedLength ? start - state.utf8.currentLength : start;
    char const* resultEnd = resultStart;

    while (input != end && count <= maxColumnCount)
    {
        if (is_control(*input) || !is_complex(*input))
        {
            // Incomplete UTF-8 sequence hit. That's invalid as well.
            if (state.utf8.expectedLength)
            {
                ++count;
                receiver.receiveInvalidGraphemeCluster();
                state.utf8 = {};
            }
            state.lastCodepointHint = 0;
            resultEnd = input;
            break;
        }

        auto const result = from_utf8(state.utf8, static_cast<uint8_t>(*input++));
        ++byteCount;

        if (holds_alternative<Incomplete>(result))
            continue;

        if (holds_alternative<Success>(result))
        {
            auto const prevCodepoint = state.lastCodepointHint;
            auto const nextCodepoint = get<Success>(result).value;
            auto const nextWidth = max(currentClusterWidth, static_cast<size_t>(width(nextCodepoint)));
            state.lastCodepointHint = nextCodepoint;
            if (grapheme_segmenter::breakable(prevCodepoint, nextCodepoint))
            {
                // Flush out current grapheme cluster's East Asian Width.
                count += currentClusterWidth;

                if (count + nextWidth > maxColumnCount)
                {
                    // Currently scanned grapheme cluster won't fit. Break at start.
                    currentClusterWidth = 0;
                    input -= byteCount;
                    break;
                }
                receiver.receiveGraphemeCluster(string_view(clusterStart, byteCount), currentClusterWidth);

                // And start a new grapheme cluster.
                currentClusterWidth = nextWidth;
                clusterStart = lastCodepointStart;
                lastCodepointStart = input - byteCount;
                byteCount = 0;
                resultEnd = input;
            }
            else
            {
                resultEnd = input;
                // Increase width on VS16 but do not decrease on VS15.
                if (nextCodepoint == 0xFE0F) // VS16
                {
                    currentClusterWidth = 2;
                    if (count + currentClusterWidth > maxColumnCount)
                    {
                        // Rewinding by {byteCount} bytes (overflow due to VS16).
                        currentClusterWidth = 0;
                        input = clusterStart;
                        break;
                    }
                }

                // Consumed {byteCount} bytes for grapheme cluster.
                lastCodepointStart = input - byteCount;
            }
        }
        else
        {
            assert(holds_alternative<Invalid>(result));
            count++;
            receiver.receiveInvalidGraphemeCluster();
            currentClusterWidth = 0;
            state.lastCodepointHint = 0;
            state.utf8.expectedLength = 0;
            byteCount = 0;
        }
    }
    count += currentClusterWidth;

    assert(resultStart <= resultEnd);

    state.next = input;
    return { count, resultStart, resultEnd };
}

scan_result scan_text(scan_state& state, std::string_view text, size_t maxColumnCount) noexcept
{
    return scan_text(state, text, maxColumnCount, null_receiver::get());
}

scan_result scan_text(scan_state& state,
                      std::string_view text,
                      size_t maxColumnCount,
                      grapheme_cluster_receiver& receiver) noexcept
{
    //       ----(a)--->   A   -------> END
    //                   ^   |
    //                   |   |
    // Start            (a) (b)
    //                   |   |
    //                   |   v
    //       ----(b)--->   B   -------> END

    enum class NextState
    {
        Trivial,
        Complex
    };

    auto result = scan_result { 0, text.data(), text.data() };

    if (state.next == nullptr)
        state.next = text.data();

    // If state indicates that we previously started consuming a UTF-8 sequence but did not complete yet,
    // attempt to finish that one first.
    if (state.utf8.expectedLength != 0)
    {
        result = detail::scan_for_text_nonascii(state, text, maxColumnCount, receiver);
        text = std::string_view(result.end, static_cast<size_t>(std::distance(result.end, text.data() + text.size())));
    }

    if (text.empty())
        return result;

    auto nextState = is_complex(text.front()) ? NextState::Complex : NextState::Trivial;
    while (result.count < maxColumnCount && state.next != (text.data() + text.size()))
    {
        switch (nextState)
        {
            case NextState::Trivial: {
                auto const count = detail::scan_for_text_ascii(text, maxColumnCount - result.count);
                if (!count)
                    return result;
                receiver.receiveAsciiSequence(text.substr(0, count));
                result.count += count;
                state.next += count;
                result.end += count;
                nextState = NextState::Complex;
                text.remove_prefix(count);
                break;
            }
            case NextState::Complex: {
                auto const sub = detail::scan_for_text_nonascii(state, text, maxColumnCount - result.count, receiver);
                if (!sub.count)
                    return result;
                nextState = NextState::Trivial;
                result.count += sub.count;
                result.end = sub.end;
                text.remove_prefix(static_cast<size_t>(std::distance(sub.start, sub.end)));
                break;
            }
        }
    }

    assert(result.start <= result.end);
    assert(result.end <= state.next);

    return result;
}

} // namespace unicode
