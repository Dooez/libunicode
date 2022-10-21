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
#include <unicode/codepoint_properties_loader.h>
#include <unicode/ucd_enums.h>
#include <unicode/ucd_fmt.h>

#include <fmt/format.h>

#include <cassert>
#include <chrono>
#include <iostream>
#include <optional>
#include <regex>
#include <string_view>
#include <utility>

// {{{ fmtlib formatters
namespace fmt
{
template <>
struct formatter<unicode::codepoint_properties>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(unicode::codepoint_properties const& value, FormatContext& ctx)
    {
        return format_to(ctx.out(),
                         "({}, {}, {}, {}, {})",
                         value.emoji() ? "Emoji" : "Text",
                         value.east_asian_width,
                         value.script,
                         value.general_category,
                         value.grapheme_cluster_break);
    }
};
} // namespace fmt
// }}}

using namespace std;
using namespace std::string_view_literals;

namespace unicode
{

namespace
{
    // {{{ string-to-enum convert helper
    constexpr optional<unicode::General_Category> make_general_category(string_view value) noexcept
    {
        auto /*static*/ constexpr mappings = array {
            pair { "Cn"sv, General_Category::Unassigned },

            pair { "Lu"sv, General_Category::Uppercase_Letter },
            pair { "Ll"sv, General_Category::Lowercase_Letter },
            pair { "Lt"sv, General_Category::Titlecase_Letter },
            pair { "Lm"sv, General_Category::Modifier_Letter },
            pair { "Lo"sv, General_Category::Other_Letter },

            pair { "Mn"sv, General_Category::Nonspacing_Mark },
            pair { "Me"sv, General_Category::Enclosing_Mark },
            pair { "Mc"sv, General_Category::Spacing_Mark },

            pair { "Nd"sv, General_Category::Decimal_Number },
            pair { "Nl"sv, General_Category::Letter_Number },
            pair { "No"sv, General_Category::Other_Number },

            pair { "Zs"sv, General_Category::Space_Separator },
            pair { "Zl"sv, General_Category::Line_Separator },
            pair { "Zp"sv, General_Category::Paragraph_Separator },

            pair { "Cc"sv, General_Category::Control },
            pair { "Cf"sv, General_Category::Format },
            pair { "Co"sv, General_Category::Private_Use },
            pair { "Cs"sv, General_Category::Surrogate },

            pair { "Pd"sv, General_Category::Dash_Punctuation },
            pair { "Ps"sv, General_Category::Open_Punctuation },
            pair { "Pe"sv, General_Category::Close_Punctuation },
            pair { "Pc"sv, General_Category::Connector_Punctuation },
            pair { "Po"sv, General_Category::Other_Punctuation },

            pair { "Sm"sv, General_Category::Math_Symbol },
            pair { "Sc"sv, General_Category::Currency_Symbol },
            pair { "Sk"sv, General_Category::Modifier_Symbol },
            pair { "So"sv, General_Category::Other_Symbol },

            pair { "Pi"sv, General_Category::Initial_Punctuation },
            pair { "Pf"sv, General_Category::Final_Punctuation },
        };

        for (auto const& mapping: mappings)
            if (mapping.first == value)
                return { mapping.second };

        return nullopt;
    }

    constexpr optional<unicode::Script> make_script(string_view value) noexcept
    {
        auto /*static*/ constexpr mappings = array {
            pair { "Adlam"sv, Script::Adlam },
            pair { "Ahom"sv, Script::Ahom },
            pair { "Anatolian_Hieroglyphs"sv, Script::Anatolian_Hieroglyphs },
            pair { "Arabic"sv, Script::Arabic },
            pair { "Armenian"sv, Script::Armenian },
            pair { "Avestan"sv, Script::Avestan },
            pair { "Balinese"sv, Script::Balinese },
            pair { "Bamum"sv, Script::Bamum },
            pair { "Bassa_Vah"sv, Script::Bassa_Vah },
            pair { "Batak"sv, Script::Batak },
            pair { "Bengali"sv, Script::Bengali },
            pair { "Bhaiksuki"sv, Script::Bhaiksuki },
            pair { "Bopomofo"sv, Script::Bopomofo },
            pair { "Brahmi"sv, Script::Brahmi },
            pair { "Braille"sv, Script::Braille },
            pair { "Buginese"sv, Script::Buginese },
            pair { "Buhid"sv, Script::Buhid },
            pair { "Canadian_Aboriginal"sv, Script::Canadian_Aboriginal },
            pair { "Carian"sv, Script::Carian },
            pair { "Caucasian_Albanian"sv, Script::Caucasian_Albanian },
            pair { "Chakma"sv, Script::Chakma },
            pair { "Cham"sv, Script::Cham },
            pair { "Cherokee"sv, Script::Cherokee },
            pair { "Chorasmian"sv, Script::Chorasmian },
            pair { "Common"sv, Script::Common },
            pair { "Coptic"sv, Script::Coptic },
            pair { "Cuneiform"sv, Script::Cuneiform },
            pair { "Cypriot"sv, Script::Cypriot },
            pair { "Cypro_Minoan"sv, Script::Cypro_Minoan },
            pair { "Cyrillic"sv, Script::Cyrillic },
            pair { "Deseret"sv, Script::Deseret },
            pair { "Devanagari"sv, Script::Devanagari },
            pair { "Dives_Akuru"sv, Script::Dives_Akuru },
            pair { "Dogra"sv, Script::Dogra },
            pair { "Duployan"sv, Script::Duployan },
            pair { "Egyptian_Hieroglyphs"sv, Script::Egyptian_Hieroglyphs },
            pair { "Elbasan"sv, Script::Elbasan },
            pair { "Elymaic"sv, Script::Elymaic },
            pair { "Ethiopic"sv, Script::Ethiopic },
            pair { "Georgian"sv, Script::Georgian },
            pair { "Glagolitic"sv, Script::Glagolitic },
            pair { "Gothic"sv, Script::Gothic },
            pair { "Grantha"sv, Script::Grantha },
            pair { "Greek"sv, Script::Greek },
            pair { "Gujarati"sv, Script::Gujarati },
            pair { "Gunjala_Gondi"sv, Script::Gunjala_Gondi },
            pair { "Gurmukhi"sv, Script::Gurmukhi },
            pair { "Han"sv, Script::Han },
            pair { "Hangul"sv, Script::Hangul },
            pair { "Hanifi_Rohingya"sv, Script::Hanifi_Rohingya },
            pair { "Hanunoo"sv, Script::Hanunoo },
            pair { "Hatran"sv, Script::Hatran },
            pair { "Hebrew"sv, Script::Hebrew },
            pair { "Hiragana"sv, Script::Hiragana },
            pair { "Imperial_Aramaic"sv, Script::Imperial_Aramaic },
            pair { "Inherited"sv, Script::Inherited },
            pair { "Inscriptional_Pahlavi"sv, Script::Inscriptional_Pahlavi },
            pair { "Inscriptional_Parthian"sv, Script::Inscriptional_Parthian },
            pair { "Javanese"sv, Script::Javanese },
            pair { "Kaithi"sv, Script::Kaithi },
            pair { "Kannada"sv, Script::Kannada },
            pair { "Katakana"sv, Script::Katakana },
            pair { "Kawi"sv, Script::Kawi },
            pair { "Kayah_Li"sv, Script::Kayah_Li },
            pair { "Kharoshthi"sv, Script::Kharoshthi },
            pair { "Khitan_Small_Script"sv, Script::Khitan_Small_Script },
            pair { "Khmer"sv, Script::Khmer },
            pair { "Khojki"sv, Script::Khojki },
            pair { "Khudawadi"sv, Script::Khudawadi },
            pair { "Lao"sv, Script::Lao },
            pair { "Latin"sv, Script::Latin },
            pair { "Lepcha"sv, Script::Lepcha },
            pair { "Limbu"sv, Script::Limbu },
            pair { "Linear_A"sv, Script::Linear_A },
            pair { "Linear_B"sv, Script::Linear_B },
            pair { "Lisu"sv, Script::Lisu },
            pair { "Lycian"sv, Script::Lycian },
            pair { "Lydian"sv, Script::Lydian },
            pair { "Mahajani"sv, Script::Mahajani },
            pair { "Makasar"sv, Script::Makasar },
            pair { "Malayalam"sv, Script::Malayalam },
            pair { "Mandaic"sv, Script::Mandaic },
            pair { "Manichaean"sv, Script::Manichaean },
            pair { "Marchen"sv, Script::Marchen },
            pair { "Masaram_Gondi"sv, Script::Masaram_Gondi },
            pair { "Medefaidrin"sv, Script::Medefaidrin },
            pair { "Meetei_Mayek"sv, Script::Meetei_Mayek },
            pair { "Mende_Kikakui"sv, Script::Mende_Kikakui },
            pair { "Meroitic_Cursive"sv, Script::Meroitic_Cursive },
            pair { "Meroitic_Hieroglyphs"sv, Script::Meroitic_Hieroglyphs },
            pair { "Miao"sv, Script::Miao },
            pair { "Modi"sv, Script::Modi },
            pair { "Mongolian"sv, Script::Mongolian },
            pair { "Mro"sv, Script::Mro },
            pair { "Multani"sv, Script::Multani },
            pair { "Myanmar"sv, Script::Myanmar },
            pair { "Nabataean"sv, Script::Nabataean },
            pair { "Nag_Mundari"sv, Script::Nag_Mundari },
            pair { "Nandinagari"sv, Script::Nandinagari },
            pair { "New_Tai_Lue"sv, Script::New_Tai_Lue },
            pair { "Newa"sv, Script::Newa },
            pair { "Nko"sv, Script::Nko },
            pair { "Nushu"sv, Script::Nushu },
            pair { "Nyiakeng_Puachue_Hmong"sv, Script::Nyiakeng_Puachue_Hmong },
            pair { "Ogham"sv, Script::Ogham },
            pair { "Ol_Chiki"sv, Script::Ol_Chiki },
            pair { "Old_Hungarian"sv, Script::Old_Hungarian },
            pair { "Old_Italic"sv, Script::Old_Italic },
            pair { "Old_North_Arabian"sv, Script::Old_North_Arabian },
            pair { "Old_Permic"sv, Script::Old_Permic },
            pair { "Old_Persian"sv, Script::Old_Persian },
            pair { "Old_Sogdian"sv, Script::Old_Sogdian },
            pair { "Old_South_Arabian"sv, Script::Old_South_Arabian },
            pair { "Old_Turkic"sv, Script::Old_Turkic },
            pair { "Old_Uyghur"sv, Script::Old_Uyghur },
            pair { "Oriya"sv, Script::Oriya },
            pair { "Osage"sv, Script::Osage },
            pair { "Osmanya"sv, Script::Osmanya },
            pair { "Pahawh_Hmong"sv, Script::Pahawh_Hmong },
            pair { "Palmyrene"sv, Script::Palmyrene },
            pair { "Pau_Cin_Hau"sv, Script::Pau_Cin_Hau },
            pair { "Phags_Pa"sv, Script::Phags_Pa },
            pair { "Phoenician"sv, Script::Phoenician },
            pair { "Psalter_Pahlavi"sv, Script::Psalter_Pahlavi },
            pair { "Rejang"sv, Script::Rejang },
            pair { "Runic"sv, Script::Runic },
            pair { "Samaritan"sv, Script::Samaritan },
            pair { "Saurashtra"sv, Script::Saurashtra },
            pair { "Sharada"sv, Script::Sharada },
            pair { "Shavian"sv, Script::Shavian },
            pair { "Siddham"sv, Script::Siddham },
            pair { "SignWriting"sv, Script::SignWriting },
            pair { "Sinhala"sv, Script::Sinhala },
            pair { "Sogdian"sv, Script::Sogdian },
            pair { "Sora_Sompeng"sv, Script::Sora_Sompeng },
            pair { "Soyombo"sv, Script::Soyombo },
            pair { "Sundanese"sv, Script::Sundanese },
            pair { "Syloti_Nagri"sv, Script::Syloti_Nagri },
            pair { "Syriac"sv, Script::Syriac },
            pair { "Tagalog"sv, Script::Tagalog },
            pair { "Tagbanwa"sv, Script::Tagbanwa },
            pair { "Tai_Le"sv, Script::Tai_Le },
            pair { "Tai_Tham"sv, Script::Tai_Tham },
            pair { "Tai_Viet"sv, Script::Tai_Viet },
            pair { "Takri"sv, Script::Takri },
            pair { "Tamil"sv, Script::Tamil },
            pair { "Tangsa"sv, Script::Tangsa },
            pair { "Tangut"sv, Script::Tangut },
            pair { "Telugu"sv, Script::Telugu },
            pair { "Thaana"sv, Script::Thaana },
            pair { "Thai"sv, Script::Thai },
            pair { "Tibetan"sv, Script::Tibetan },
            pair { "Tifinagh"sv, Script::Tifinagh },
            pair { "Tirhuta"sv, Script::Tirhuta },
            pair { "Toto"sv, Script::Toto },
            pair { "Ugaritic"sv, Script::Ugaritic },
            pair { "Vai"sv, Script::Vai },
            pair { "Vithkuqi"sv, Script::Vithkuqi },
            pair { "Wancho"sv, Script::Wancho },
            pair { "Warang_Citi"sv, Script::Warang_Citi },
            pair { "Yezidi"sv, Script::Yezidi },
            pair { "Yi"sv, Script::Yi },
            pair { "Zanabazar_Square"sv, Script::Zanabazar_Square },
        };

        for (auto const& mapping: mappings)
            if (mapping.first == value)
                return { mapping.second };

        return nullopt;
    }

    constexpr optional<unicode::East_Asian_Width> make_width(string_view value) noexcept
    {
        auto /*static*/ constexpr mappings = array {
            pair { "A"sv, unicode::East_Asian_Width::Ambiguous },
            pair { "F"sv, unicode::East_Asian_Width::Fullwidth },
            pair { "H"sv, unicode::East_Asian_Width::Halfwidth },
            pair { "N"sv, unicode::East_Asian_Width::Neutral },
            pair { "Na"sv, unicode::East_Asian_Width::Narrow },
            pair { "W"sv, unicode::East_Asian_Width::Wide },
        };

        for (auto const& mapping: mappings)
            if (mapping.first == value)
                return mapping.second;

        return nullopt;
    }

    constexpr optional<unicode::Grapheme_Cluster_Break> make_gb(string_view value) noexcept
    {
        auto /*static*/ constexpr mappings = array {
            pair { "LV"sv, unicode::Grapheme_Cluster_Break::LV },
            pair { "Undefined"sv, unicode::Grapheme_Cluster_Break::Undefined },
            pair { "CR"sv, unicode::Grapheme_Cluster_Break::CR },
            pair { "Control"sv, unicode::Grapheme_Cluster_Break::Control },
            pair { "E_Base"sv, unicode::Grapheme_Cluster_Break::E_Base },
            pair { "E_Base_GAZ"sv, unicode::Grapheme_Cluster_Break::E_Base_GAZ },
            pair { "E_Modifier"sv, unicode::Grapheme_Cluster_Break::E_Modifier },
            pair { "Extend"sv, unicode::Grapheme_Cluster_Break::Extend },
            pair { "Glue_After_Zwj"sv, unicode::Grapheme_Cluster_Break::Glue_After_Zwj },
            pair { "L"sv, unicode::Grapheme_Cluster_Break::L },
            pair { "LF"sv, unicode::Grapheme_Cluster_Break::LF },
            pair { "LV"sv, unicode::Grapheme_Cluster_Break::LV },
            pair { "LVT"sv, unicode::Grapheme_Cluster_Break::LVT },
            pair { "Other"sv, unicode::Grapheme_Cluster_Break::Other },
            pair { "Prepend"sv, unicode::Grapheme_Cluster_Break::Prepend },
            pair { "Regional_Indicator"sv, unicode::Grapheme_Cluster_Break::Regional_Indicator },
            pair { "SpacingMark"sv, unicode::Grapheme_Cluster_Break::SpacingMark },
            pair { "T"sv, unicode::Grapheme_Cluster_Break::T },
            pair { "V"sv, unicode::Grapheme_Cluster_Break::V },
            pair { "ZWJ"sv, unicode::Grapheme_Cluster_Break::ZWJ },
        };

        for (auto const& mapping: mappings)
            if (mapping.first == value)
                return mapping.second;

        return nullopt;
    }
    // }}}

    struct scoped_timer
    {
        std::chrono::time_point<std::chrono::steady_clock> _start;
        std::ostream* _output;
        std::string _message;

        scoped_timer(std::ostream* output, std::string message):
            _start { std::chrono::steady_clock::now() }, _output { output }, _message { std::move(message) }
        {
            if (_output)
                *_output << _message << " ...\n";
        }

        ~scoped_timer()
        {
            if (!_output)
                return;
            auto const finish = std::chrono::steady_clock::now();
            auto const diff = finish - _start;
            *_output << _message << " " << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()
                     << " ms\n";
        }
    };

    inline EmojiSegmentationCategory toEmojiSegmentationCategory(char32_t codepoint,
                                                                 codepoint_properties const& props) noexcept
    {
        if (codepoint == 0x20e3)
            return EmojiSegmentationCategory::CombiningEnclosingKeyCap;
        if (codepoint == 0x20e0)
            return EmojiSegmentationCategory::CombiningEnclosingCircleBackslash;
        if (codepoint == 0x200d)
            return EmojiSegmentationCategory::ZWJ;
        if (codepoint == 0xfe0e)
            return EmojiSegmentationCategory::VS15;
        if (codepoint == 0xfe0f)
            return EmojiSegmentationCategory::VS16;
        if (codepoint == 0x1f3f4)
            return EmojiSegmentationCategory::TagBase;
        if ((codepoint >= 0xE0030 && codepoint <= 0xE0039) || (codepoint >= 0xE0061 && codepoint <= 0xE007A))
            return EmojiSegmentationCategory::TagSequence;
        if (codepoint == 0xE007F)
            return EmojiSegmentationCategory::TagTerm;

        if (props.emoji_modifier_base())
            return EmojiSegmentationCategory::EmojiModifierBase;
        if (props.emoji_modifier())
            return EmojiSegmentationCategory::EmojiModifier;
        if (props.grapheme_cluster_break == Grapheme_Cluster_Break::Regional_Indicator)
            return EmojiSegmentationCategory::RegionalIndicator;
        if (('0' <= codepoint && codepoint <= '9') || codepoint == '#' || codepoint == '*')
            return EmojiSegmentationCategory::KeyCapBase;
        if (props.emoji_presentation())
            return EmojiSegmentationCategory::EmojiEmojiPresentation;
        if (props.emoji() && !props.emoji_presentation())
            return EmojiSegmentationCategory::EmojiTextPresentation;
        if (props.emoji())
            return EmojiSegmentationCategory::Emoji;

        return EmojiSegmentationCategory::Invalid;
    }
    class codepoint_properties_loader
    {
      public:
        static codepoint_properties_table load_from_directory(string const& ucdDataDirectory,
                                                              std::ostream* log = nullptr);

      private:
        auto static constexpr block_size = codepoint_properties::tables_view::block_size;
        using tables_view = codepoint_properties::tables_view;
        using stage1_element_type = tables_view::stage1_element_type;
        using stage2_element_type = tables_view::stage2_element_type;

        codepoint_properties_loader(string ucdDataDirectory, std::ostream* log = nullptr);

        void load();
        void create_multistage_tables();
        [[nodiscard]] optional<size_t> find_same_block(size_t baseCodepoint) const noexcept;
        [[nodiscard]] bool is_same_block(size_t a, size_t b) const noexcept;
        [[nodiscard]] stage1_element_type get_or_create_index_to_stage2_block(char32_t blockStart);
        [[nodiscard]] stage2_element_type get_or_create_index_to_property(char32_t codepoint);

#if !defined(NDEBUG)
        void verify();
        void verify_block(uint32_t blockNumber);
#endif

        [[nodiscard]] codepoint_properties& properties(char32_t codepoint) noexcept
        {
            return _codepoints[static_cast<size_t>(codepoint)];
        }

        template <typename T>
        void process_properties(string const& filePathSuffix, T callback)
        {
            auto const _ = scoped_timer { _log, fmt::format("Loading file {}", filePathSuffix) };

            // clang-format off
            auto const singleCodepointPattern = regex(R"(^([0-9A-F]+)\s*;\s*([A-Za-z_]+))");
            auto const codepointRangePattern = regex(R"(^([0-9A-F]+)\.\.([0-9A-F]+)\s*;\s*([A-Za-z_]+))");
            // clang-format on

            auto f = ifstream(_ucdDataDirectory + "/" + filePathSuffix);
            while (f.good())
            {
                string line;
                getline(f, line);
                auto sm = smatch {};
                if (regex_search(line, sm, singleCodepointPattern))
                {
                    auto const codepoint = static_cast<char32_t>(stoul(sm[1], nullptr, 16));
                    callback(codepoint, sm.str(2));
                }
                else if (regex_search(line, sm, codepointRangePattern))
                {
                    auto const first = static_cast<char32_t>(stoul(sm[1], nullptr, 16));
                    auto const last = static_cast<char32_t>(stoul(sm[2], nullptr, 16));
                    for (auto codepoint = first; codepoint <= last; ++codepoint)
                        callback(codepoint, sm.str(3));
                }
            }
        }

        string _ucdDataDirectory;
        std::ostream* _log;
        vector<codepoint_properties> _codepoints {}; // Meh!
        codepoint_properties_table _output {};
    };

    codepoint_properties_loader::codepoint_properties_loader(string ucdDataDirectory, std::ostream* log):
        _ucdDataDirectory { std::move(ucdDataDirectory) }, _log { log }
    {
        _codepoints.resize(0x110'000);

        // stage1-table is always fixed size, depending on the block size.
        _output.stage1.resize(0x110'000 / block_size);
    }

    void codepoint_properties_loader::load()
    {
        process_properties("Scripts.txt", [&](char32_t codepoint, string_view value) {
            properties(codepoint).script = make_script(value).value_or(unicode::Script::Invalid);
        });

        process_properties("DerivedCoreProperties.txt", [&](char32_t codepoint, string_view value) {
            // Generically written such that we can easily add more core properties here, once relevant.
            auto static constexpr mappings = array {
                pair { "Grapheme_Extend", codepoint_properties::FlagCoreGraphemeExtend },
            };
            auto const equalName = [=](auto x) {
                return x.first == value;
            };

            if (auto const i = find_if(begin(mappings), end(mappings), equalName); i != end(mappings))
                properties(codepoint).flags |= i->second;
        });


        process_properties("extracted/DerivedGeneralCategory.txt",
                           [&](char32_t codepoint, string_view value) {
                               (void) codepoint;
                               (void) value;
                               properties(codepoint).general_category = make_general_category(value).value();
                           });

        process_properties("auxiliary/GraphemeBreakProperty.txt", [&](char32_t codepoint, string_view value) {
            properties(codepoint).grapheme_cluster_break = make_gb(value).value();
        });

        process_properties("EastAsianWidth.txt", [&](char32_t codepoint, string_view value) {
            properties(codepoint).east_asian_width = make_width(value).value();
        });

        // {{{ fill EmojiSegmentationCategory and other emoji related properties
        // clang-format off
        properties(0x20e3).emoji_segmentation_category = EmojiSegmentationCategory::CombiningEnclosingKeyCap;
        properties(0x20e0).emoji_segmentation_category = EmojiSegmentationCategory::CombiningEnclosingCircleBackslash;
        properties(0x200d).emoji_segmentation_category = EmojiSegmentationCategory::ZWJ;
        properties(0xfe0e).emoji_segmentation_category = EmojiSegmentationCategory::VS15;
        properties(0xfe0f).emoji_segmentation_category = EmojiSegmentationCategory::VS16;

        properties(0x1f3f4).emoji_segmentation_category = EmojiSegmentationCategory::TagBase;
        for (char32_t ch = 0xE0030; ch <= 0xE0039; ++ch)
            properties(ch).emoji_segmentation_category = EmojiSegmentationCategory::TagSequence;
        for (char32_t ch = 0xE0061; ch <= 0xE007A; ++ch)
            properties(ch).emoji_segmentation_category = EmojiSegmentationCategory::TagSequence;
        properties(0xE007F).emoji_segmentation_category = EmojiSegmentationCategory::TagTerm;

        for (char32_t const codepoint: array<char32_t, 12> { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '#', '*' })
            properties(codepoint).emoji_segmentation_category = EmojiSegmentationCategory::KeyCapBase;
        // clang-format on

        process_properties("emoji/emoji-data.txt", [&](char32_t codepoint, string_view value) -> void {
            auto static constexpr mappings = array {
                pair { "Emoji", codepoint_properties::FlagEmoji },
                pair { "Emoji_Component", codepoint_properties::FlagEmojiComponent },
                pair { "Emoji_Modifier", codepoint_properties::FlagEmojiModifier },
                pair { "Emoji_Modifier_Base", codepoint_properties::FlagEmojiModifierBase },
                pair { "Emoji_Presentation", codepoint_properties::FlagEmojiPresentation },
                pair { "Extended_Pictographic", codepoint_properties::FlagExtendedPictographic },
            };
            auto const equalName = [=](auto x) {
                return x.first == value;
            };

            if (auto const i = find_if(begin(mappings), end(mappings), equalName); i != end(mappings))
                properties(codepoint).flags |= i->second;
        });

        {
            auto const _ = scoped_timer { _log, "Assigning EmojiSegmentationCategory" };
            for (char32_t codepoint = 0; codepoint < 0x110'000; ++codepoint)
                properties(codepoint).emoji_segmentation_category =
                    toEmojiSegmentationCategory(codepoint, properties(codepoint));
        }
        // }}}
    }

    codepoint_properties_table codepoint_properties_loader::load_from_directory(
        string const& ucdDataDirectory, std::ostream* log)
    {
        auto loader = codepoint_properties_loader { ucdDataDirectory, log };
        loader.load();
        loader.create_multistage_tables();

#if !defined(NDEBUG)
        loader.verify();
#endif

        return std::move(loader._output);
    }

    bool codepoint_properties_loader::is_same_block(size_t a, size_t b) const noexcept
    {
        assert(a % block_size == 0);
        assert(b % block_size == 0);
        assert(a + block_size <= _codepoints.size());
        assert(b + block_size <= _codepoints.size());

        for (size_t i = 0; i < block_size; ++i)
            if (_codepoints[a + i] != _codepoints[b + i])
                return false;
        return true;
    }

    void codepoint_properties_loader::create_multistage_tables()
    {
        auto const _ = scoped_timer { _log, "Creating multi stage tables" };
        for (char32_t blockStart = 0; blockStart <= 0x110'000 - block_size; blockStart += block_size)
            _output.stage1[blockStart / block_size] = get_or_create_index_to_stage2_block(blockStart);
    }

    codepoint_properties_loader::stage1_element_type codepoint_properties_loader::
        get_or_create_index_to_stage2_block(char32_t blockStart)
    {
        if (auto other_block = find_same_block(static_cast<size_t>(blockStart)))
            return _output.stage1[other_block.value()];

        // Block has not been seen yet. Create a new block.
        auto const stage2Index = _output.stage2.size() / block_size;
        assert(stage2Index < numeric_limits<stage1_element_type>::max());

        for (char32_t codepoint = blockStart; codepoint < blockStart + block_size; ++codepoint)
            _output.stage2.emplace_back(get_or_create_index_to_property(codepoint));

        assert(_output.stage2.size() % block_size == 0);

        return static_cast<stage1_element_type>(stage2Index);
    }

    optional<size_t> codepoint_properties_loader::find_same_block(size_t blockStart) const noexcept
    {
        assert(blockStart % block_size == 0);
        assert(blockStart + block_size <= _codepoints.size());

        for (size_t otherBlockStart = 0; otherBlockStart < blockStart; otherBlockStart += block_size)
            if (is_same_block(otherBlockStart, blockStart))
                return { otherBlockStart / block_size };
        return nullopt;
    }

    codepoint_properties_loader::stage2_element_type codepoint_properties_loader::
        get_or_create_index_to_property(char32_t codepoint)
    {
        auto& properties = _output.properties;
        auto const propertyIterator = find(properties.begin(), properties.end(), _codepoints[codepoint]);
        if (propertyIterator != properties.end())
            return static_cast<stage2_element_type>(distance(properties.begin(), propertyIterator));

        properties.emplace_back(_codepoints[codepoint]);
        auto const resultingIndex = properties.size() - 1;
        assert(resultingIndex < numeric_limits<stage2_element_type>::max());
        return static_cast<stage2_element_type>(resultingIndex);
    }

#if !defined(NDEBUG)
    void codepoint_properties_loader::verify()
    {
        for (uint32_t blockStart = 0; blockStart <= 0x110'000 - block_size; ++blockStart)
            verify_block(blockStart / block_size);
    }

    void codepoint_properties_loader::verify_block(uint32_t blockNumber)
    {
        for (uint32_t codepoint = blockNumber * block_size; codepoint < (blockNumber + 1) * block_size;
             ++codepoint)
        {
            auto const& a = _codepoints[codepoint];
            auto const& b = _output[codepoint];
            if (a != b)
            {
                throw runtime_error(fmt::format("U+{:X} mismatch in properties. "
                                                "Expected : {}; "
                                                "Actual   : {}",
                                                (unsigned) codepoint,
                                                a,
                                                b));
            }
        }
    }
#endif
} // namespace

codepoint_properties_table codepoint_properties_table::load_from_directory(string const& ucdDataDirectory,
                                                                           std::ostream* log)
{
    return codepoint_properties_loader::load_from_directory(ucdDataDirectory, log);
}

} // namespace unicode