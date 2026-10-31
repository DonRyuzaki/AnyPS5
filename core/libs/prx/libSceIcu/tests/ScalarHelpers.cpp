#include "prx/libSceIcu/libSceIcu.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr auto Maximum = std::numeric_limits<std::int32_t>::max();
constexpr auto Minimum = std::numeric_limits<std::int32_t>::min();

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::fprintf(stderr, "ICU scalar helper check failed at line %u\n", location.line());
    std::abort();
}

template<typename TAction>
void RequireInvalidArgument(TAction action, std::source_location location = std::source_location::current()) {
    try { action(); } catch (const std::invalid_argument&) { return; }
    Require(false, location);
}

template<typename TAction>
void RequireOverflow(TAction action, std::source_location location = std::source_location::current()) {
    try { action(); } catch (const std::overflow_error&) { return; }
    Require(false, location);
}

std::uint32_t Lower(std::uint32_t unit) {
    constexpr std::string_view uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    constexpr std::string_view lowercase = "abcdefghijklmnopqrstuvwxyz";
    if (unit > 0x7f) return unit;
    const auto position = uppercase.find(static_cast<char>(unit));
    return position == uppercase.npos ? unit : static_cast<std::uint8_t>(lowercase[position]);
}

std::uint32_t Upper(std::uint32_t unit) {
    constexpr std::string_view uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    constexpr std::string_view lowercase = "abcdefghijklmnopqrstuvwxyz";
    if (unit > 0x7f) return unit;
    const auto position = lowercase.find(static_cast<char>(unit));
    return position == lowercase.npos ? unit : static_cast<std::uint8_t>(uppercase[position]);
}

void CharacterHelpers() {
    for (std::uint32_t unit = 0; unit <= 0xffff; ++unit)
        Require(u_asciiToUpper_nid_postfix(static_cast<char16_t>(unit)) == Upper(unit));
    constexpr std::string_view letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (std::uint32_t unit = 0; unit <= 0xff; ++unit) {
        const auto byte = static_cast<char>(unit);
        Require(static_cast<std::uint8_t>(uprv_asciitolower_nid_postfix(byte)) == Lower(unit));
        Require(static_cast<std::uint8_t>(uprv_toupper_nid_postfix(byte)) == Upper(unit));
        Require(uprv_isASCIILetter_nid_postfix(byte) == (letters.find(byte) == letters.npos ? 0 : 1));
    }
}

void Comparators() {
    const std::uint64_t context = 0x123456789abcdef0;
    for (std::uint32_t value = 0; value <= 0xffff; ++value) {
        for (const std::uint16_t other : {std::uint16_t{0}, std::uint16_t{0x7fff}, std::uint16_t{0x8000}, std::uint16_t{0xffff}}) {
            const std::array<std::uint16_t, 3> first {0x1234, static_cast<std::uint16_t>(value), 0xabcd};
            const std::array<std::uint16_t, 3> second {0x5678, other, 0xef01};
            const auto expected = static_cast<std::int32_t>(value) - static_cast<std::int32_t>(other);
            Require(uprv_uint16Comparator_nid_postfix(nullptr, &first[1], &second[1]) == expected);
            Require(uprv_uint16Comparator_nid_postfix(&context, &second[1], &first[1]) == -expected);
            Require(first == std::array<std::uint16_t, 3>{0x1234, static_cast<std::uint16_t>(value), 0xabcd});
            Require(second == std::array<std::uint16_t, 3>{0x5678, other, 0xef01});
        }
    }
    auto check32 = [&](std::uint32_t left, std::uint32_t right) {
        const std::array<std::uint32_t, 3> first {0x12345678, left, 0xabcdef01};
        const std::array<std::uint32_t, 3> second {0x23456789, right, 0xbcdef012};
        const auto expected = left == right ? 0 : left < right ? -1 : 1;
        Require(uprv_uint32Comparator_nid_postfix(nullptr, &first[1], &second[1]) == expected);
        Require(uprv_uint32Comparator_nid_postfix(&context, &second[1], &first[1]) == -expected);
        Require(first == std::array<std::uint32_t, 3>{0x12345678, left, 0xabcdef01});
        Require(second == std::array<std::uint32_t, 3>{0x23456789, right, 0xbcdef012});
    };
    constexpr std::uint32_t boundaries[] {0, 1, 0x7fff, 0xffff, 0x10000, 0x7fffffff, 0x80000000, 0xfffffffe, 0xffffffff};
    for (const auto left : boundaries) for (const auto right : boundaries) check32(left, right);
    std::uint32_t state = 0x4567abcd;
    for (int sample = 0; sample < 65536; ++sample) {
        state = state * 1664525u + 1013904223u;
        const auto left = state;
        state = state * 1664525u + 1013904223u;
        check32(left, state);
    }
    const std::uint32_t value = 0;
    for (const auto compare : {uprv_uint16Comparator_nid_postfix, uprv_uint32Comparator_nid_postfix}) {
        RequireInvalidArgument([&] { compare(&context, nullptr, &value); });
        RequireInvalidArgument([&] { compare(&context, &value, nullptr); });
        RequireInvalidArgument([&] { compare(&context, nullptr, nullptr); });
    }
}

void CaseComparison() {
    Require(uprv_stricmp_nid_postfix(nullptr, nullptr) == 0);
    Require(uprv_stricmp_nid_postfix(nullptr, "") == -1);
    Require(uprv_stricmp_nid_postfix("", nullptr) == 1);
    for (const auto count : {0u, 1u, 0xffffffffu}) {
        Require(uprv_strnicmp_nid_postfix(nullptr, nullptr, count) == 0);
        Require(uprv_strnicmp_nid_postfix(nullptr, "", count) == -1);
        Require(uprv_strnicmp_nid_postfix("", nullptr, count) == 1);
    }
    for (std::uint32_t left = 0; left <= 0xff; ++left) {
        for (std::uint32_t right = 0; right <= 0xff; ++right) {
            const std::array<char, 3> first {static_cast<char>(left), 0, 'x'};
            const std::array<char, 3> second {static_cast<char>(right), 0, 'y'};
            const auto expected = left == 0 ? (right == 0 ? 0 : -1) : right == 0 ? 1 :
                static_cast<int>(Lower(left)) - static_cast<int>(Lower(right));
            Require(uprv_stricmp_nid_postfix(first.data(), second.data()) == expected);
            for (const auto count : {1u, 0xffffffffu})
                Require(uprv_strnicmp_nid_postfix(first.data(), second.data(), count) == expected);
            Require(uprv_strnicmp_nid_postfix(first.data(), second.data(), 0) == 0);
            Require(first == std::array<char, 3>{static_cast<char>(left), 0, 'x'});
            Require(second == std::array<char, 3>{static_cast<char>(right), 0, 'y'});
        }
    }
    Require(uprv_stricmp_nid_postfix("pREFIXaBcZ", "PrefixAbCa") == 'z' - 'a');
    Require(uprv_stricmp_nid_postfix("a", "Abc") == -1);
    Require(uprv_stricmp_nid_postfix("Abc", "a") == 1);
    Require(uprv_strnicmp_nid_postfix("pREFIXaBcZ", "PrefixAbCa", 9) == 0);
    Require(uprv_strnicmp_nid_postfix("pREFIXaBcZ", "PrefixAbCa", 10) == 'z' - 'a');
    const char first[] {'A', 'b'};
    const char second[] {'a', 'B'};
    Require(uprv_strnicmp_nid_postfix(first, second, 2) == 0);
    const char embeddedFirst[] {'a', 0, 'x', 0};
    const char embeddedSecond[] {'A', 0, 'y', 0};
    Require(uprv_stricmp_nid_postfix(embeddedFirst, embeddedSecond) == 0);
    Require(uprv_strnicmp_nid_postfix(embeddedFirst, embeddedSecond, 0xffffffffu) == 0);
}

std::vector<std::uint8_t> PropertyName(const char* text) {
    std::vector<std::uint8_t> result;
    constexpr std::string_view ignored = "\t\n\v\f\r -_";
    for (; *text; ++text) {
        const auto unit = static_cast<std::uint8_t>(*text);
        if (ignored.find(*text) == ignored.npos) result.push_back(static_cast<std::uint8_t>(Lower(unit)));
    }
    return result;
}

int PropertyDifference(const char* left, const char* right) {
    auto first = PropertyName(left);
    auto second = PropertyName(right);
    first.push_back(0);
    second.push_back(0);
    for (std::size_t index = 0;; ++index) {
        if (first[index] != second[index]) return static_cast<int>(first[index]) - second[index];
        if (first[index] == 0) return 0;
    }
}

void PropertyComparison() {
    for (std::uint32_t left = 0; left <= 0xff; ++left) {
        for (std::uint32_t right = 0; right <= 0xff; ++right) {
            const char first[] {static_cast<char>(left), 0};
            const char second[] {static_cast<char>(right), 0};
            Require(uprv_compareASCIIPropertyNames_nid_postfix(first, second) == PropertyDifference(first, second));
        }
    }
    const char* names[] {"", " -_\t\n\v\f\r", "a", "A", "aZ", "a_b", " -\tLatin__CAPITAL\nLetter-\rA ",
        "latin capital letter a", "lowercase", "lowercasea", "!", "\x7f", "\x80", "\xff"};
    for (const auto first : names) for (const auto second : names)
        Require(uprv_compareASCIIPropertyNames_nid_postfix(first, second) == PropertyDifference(first, second));
    Require(uprv_compareASCIIPropertyNames_nid_postfix("a", "Ab") == -'b');
    Require(uprv_compareASCIIPropertyNames_nid_postfix("Ab", "a") == 'b');
    const char first[] {'A', 0, 'z', 0};
    const char second[] {'a', 0, 'b', 0};
    Require(uprv_compareASCIIPropertyNames_nid_postfix(first, second) == 0);
    RequireInvalidArgument([&] { uprv_compareASCIIPropertyNames_nid_postfix(nullptr, ""); });
    RequireInvalidArgument([&] { uprv_compareASCIIPropertyNames_nid_postfix("", nullptr); });
    RequireInvalidArgument([&] { uprv_compareASCIIPropertyNames_nid_postfix(nullptr, nullptr); });
    constexpr std::size_t maxAdvance = static_cast<std::size_t>(Maximum >> 8);
    std::string longName(maxAdvance - 1, ' ');
    longName += 'A';
    Require(uprv_compareASCIIPropertyNames_nid_postfix(longName.c_str(), "a") == 0);
    longName.insert(longName.begin(), ' ');
    RequireOverflow([&] { uprv_compareASCIIPropertyNames_nid_postfix(longName.c_str(), "a"); });
    RequireOverflow([&] { uprv_compareASCIIPropertyNames_nid_postfix("a", longName.c_str()); });
    Require(uprv_compareASCIIPropertyNames_nid_postfix(longName.c_str(), "b") == -1);
    longName.back() = ' ';
    Require(uprv_compareASCIIPropertyNames_nid_postfix(longName.c_str(), "") == 0);
}

std::int32_t Hash(const char16_t* text, std::size_t length) {
    const auto stride = length < 32 ? 1 : length / 32;
    std::uint32_t result = 0;
    for (std::size_t index = 0; index < length; index += stride)
        result = result * 37u + static_cast<std::uint16_t>(text[index]);
    return std::bit_cast<std::int32_t>(result);
}

void StringHash() {
    for (const auto length : {Minimum, -2, -1, 0, 1, Maximum})
        Require(ustr_hashUCharsN_nid_postfix(nullptr, length) == 0);
    const char16_t single = 0xffff;
    Require(ustr_hashUCharsN_nid_postfix(&single, 0) == 0);
    Require(ustr_hashUCharsN_nid_postfix(&single, 1) == 65535);
    for (const auto length : {Minimum, -2, -1})
        RequireInvalidArgument([&] { ustr_hashUCharsN_nid_postfix(&single, length); });
    const char16_t embedded[] {u'a', 0, u'b'};
    Require(ustr_hashUCharsN_nid_postfix(embedded, 3) == u'a' * 37 * 37 + u'b');
    std::array<char16_t, 4096> text;
    std::uint32_t state = 0x3210abcd;
    for (auto& unit : text) {
        state = state * 1664525u + 1013904223u;
        unit = static_cast<char16_t>(state >> 16);
    }
    const auto before = text;
    for (std::size_t length = 0; length <= text.size(); ++length)
        Require(ustr_hashUCharsN_nid_postfix(text.data(), static_cast<std::int32_t>(length)) == Hash(text.data(), length));
    Require(text == before);
    for (const std::int32_t length : {31, 32, 33, 63, 64, 65, 95, 96, 97, 127, 128, 129}) {
        std::array<char16_t, 129> zeros{};
        zeros[static_cast<std::size_t>(length - 1)] = 0x7fff;
        const auto stride = length < 32 ? 1 : length / 32;
        const auto expected = (length - 1) % stride == 0 ? 0x7fff : 0;
        Require(ustr_hashUCharsN_nid_postfix(zeros.data(), length) == expected);
    }
    std::array<char16_t, 64> sampled{};
    Require(ustr_hashUCharsN_nid_postfix(sampled.data(), 64) == 0);
    sampled[63] = 0xffff;
    Require(ustr_hashUCharsN_nid_postfix(sampled.data(), 64) == 0);
    sampled[62] = 0x1234;
    Require(ustr_hashUCharsN_nid_postfix(sampled.data(), 64) == 0x1234);
}

}

int main() {
    CharacterHelpers();
    Comparators();
    CaseComparison();
    PropertyComparison();
    StringHash();
    std::puts("ICU scalar helper tests passed");
    return 0;
}
