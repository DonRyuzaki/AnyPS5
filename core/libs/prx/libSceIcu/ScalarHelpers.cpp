#include "prx/libSceIcu/ScalarHelpers.hpp"
#include <bit>
#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace {

unsigned char Lower(unsigned char value) {
    return value >= 'A' && value <= 'Z' ? static_cast<unsigned char>(value + ('a' - 'A')) : value;
}

template<typename TValue>
TValue Read(const void* address) {
    if (!address) throw std::invalid_argument("ICU comparator: null operand");
    TValue value;
    std::memcpy(&value, address, sizeof(value));
    return value;
}

template<bool Counted>
int CompareStrings(const char* first, const char* second, std::uint32_t count = 0) {
    if (!first) return second ? -1 : 0;
    if (!second) return 1;
    while (!Counted || count != 0) {
        const auto left = static_cast<unsigned char>(*first);
        const auto right = static_cast<unsigned char>(*second);
        if (left == 0 || right == 0) return (left != 0) - (right != 0);
        const int difference = static_cast<int>(Lower(left)) - static_cast<int>(Lower(right));
        if (difference != 0) return difference;
        ++first;
        ++second;
        if constexpr (Counted) --count;
    }
    return 0;
}

struct PropertyCharacter {
    unsigned char value;
    std::size_t advance;
};

PropertyCharacter NextPropertyCharacter(const char* text) {
    std::size_t advance = 0;
    unsigned char value;
    do {
        value = static_cast<unsigned char>(text[advance++]);
    } while (value == '-' || value == '_' || value == ' ' || (value >= 0x09 && value <= 0x0d));
    return {Lower(value), advance};
}

} // namespace

extern "C" {

char16_t APS5_VABI u_asciiToUpper_nid_postfix(char16_t value) {
    return value >= u'a' && value <= u'z' ? static_cast<char16_t>(value + (u'A' - u'a')) : value;
}

char APS5_VABI uprv_asciitolower_nid_postfix(char value) {
    return value >= 'A' && value <= 'Z' ? static_cast<char>(value + ('a' - 'A')) : value;
}

std::int8_t APS5_VABI uprv_isASCIILetter_nid_postfix(char value) {
    return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z');
}

char APS5_VABI uprv_toupper_nid_postfix(char value) {
    return value >= 'a' && value <= 'z' ? static_cast<char>(value + ('A' - 'a')) : value;
}

std::int32_t APS5_VABI uprv_uint16Comparator_nid_postfix(const void*, const void* left, const void* right) {
    return static_cast<std::int32_t>(Read<std::uint16_t>(left)) - static_cast<std::int32_t>(Read<std::uint16_t>(right));
}

std::int32_t APS5_VABI uprv_uint32Comparator_nid_postfix(const void*, const void* left, const void* right) {
    const auto first = Read<std::uint32_t>(left);
    const auto second = Read<std::uint32_t>(right);
    return (first > second) - (first < second);
}

int APS5_VABI uprv_stricmp_nid_postfix(const char* first, const char* second) {
    return CompareStrings<false>(first, second);
}

int APS5_VABI uprv_strnicmp_nid_postfix(const char* first, const char* second, std::uint32_t count) {
    return CompareStrings<true>(first, second, count);
}

std::int32_t APS5_VABI uprv_compareASCIIPropertyNames_nid_postfix(const char* first, const char* second) {
    if (!first || !second) throw std::invalid_argument("ICU property comparison: null name");
    constexpr auto maxAdvance = static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max() >> 8);
    for (;;) {
        const auto left = NextPropertyCharacter(first);
        const auto right = NextPropertyCharacter(second);
        if (left.value != right.value) return static_cast<std::int32_t>(left.value) - static_cast<std::int32_t>(right.value);
        if (left.value == 0) return 0;
        if (left.advance > maxAdvance || right.advance > maxAdvance) {
            throw std::overflow_error("ICU property comparison: packed advancement exceeds its signed range");
        }
        first += left.advance;
        second += right.advance;
    }
}

std::int32_t APS5_VABI ustr_hashUCharsN_nid_postfix(const char16_t* text, std::int32_t length) {
    if (!text) return 0;
    if (length < 0) throw std::invalid_argument("ICU string hash: negative length");
    const auto count = static_cast<std::uint32_t>(length);
    const std::uint32_t stride = count < 32 ? 1 : count / 32;
    std::uint32_t hash = 0;
    for (std::uint32_t index = 0; index < count; index += stride) hash = hash * 37 + static_cast<std::uint32_t>(text[index]);
    return std::bit_cast<std::int32_t>(hash);
}

}
