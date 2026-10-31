#include "prx/libSceIcu/BufferHelpers.hpp"
#include <array>
#include <stdexcept>

namespace {

constexpr std::int32_t StringNotTerminated = -124;
constexpr std::int32_t BufferOverflow = 15;
constexpr std::array<std::uint32_t, 4> InvariantChars{0xfffffbff, 0xffffffe5, 0x87fffffe, 0x87fffffe};

template<typename TUnit>
std::int32_t Terminate(TUnit* destination, std::int32_t capacity, std::int32_t length, std::int32_t* status) {
    if (!status || *status > 0 || length < 0) return length;
    if (length < capacity) {
        if (!destination) throw std::invalid_argument("ICU string termination: null destination");
        destination[length] = 0;
        if (*status == StringNotTerminated) *status = 0;
    } else if (length == capacity) {
        *status = StringNotTerminated;
    } else {
        *status = BufferOverflow;
    }
    return length;
}

bool Invariant(std::uint32_t unit) {
    return unit <= 0x7f && (InvariantChars[unit >> 5] & (1u << (unit & 0x1f))) != 0;
}

template<typename TUnit>
std::int8_t IsInvariantString(const TUnit* text, std::int32_t length) {
    if (length == 0) return 1;
    if (!text) throw std::invalid_argument("ICU invariant string: null source");
    while (length != 0) {
        const auto unit = *text++;
        if (length < 0) {
            if (unit == 0) return 1;
        } else {
            --length;
        }
        if (!Invariant(unit)) return 0;
    }
    return 1;
}

} // namespace

extern "C" {

std::int32_t APS5_VABI u_terminateChars_nid_postfix(char* destination, std::int32_t capacity, std::int32_t length, std::int32_t* status) {
    return Terminate(destination, capacity, length, status);
}

std::int32_t APS5_VABI u_terminateUChars_nid_postfix(char16_t* destination, std::int32_t capacity, std::int32_t length, std::int32_t* status) {
    return Terminate(destination, capacity, length, status);
}

std::int32_t APS5_VABI u_terminateUChar32s_nid_postfix(std::int32_t* destination, std::int32_t capacity, std::int32_t length, std::int32_t* status) {
    return Terminate(destination, capacity, length, status);
}

std::int8_t APS5_VABI uprv_isInvariantString_nid_postfix(const char* text, std::int32_t length) {
    return IsInvariantString(reinterpret_cast<const unsigned char*>(text), length);
}

std::int8_t APS5_VABI uprv_isInvariantUString_nid_postfix(const char16_t* text, std::int32_t length) {
    return IsInvariantString(text, length);
}

}
