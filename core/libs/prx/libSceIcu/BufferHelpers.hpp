#pragma once
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>

extern "C" {
std::int32_t APS5_VABI u_terminateChars_nid_postfix(char* destination, std::int32_t capacity, std::int32_t length, std::int32_t* status);
std::int32_t APS5_VABI u_terminateUChars_nid_postfix(char16_t* destination, std::int32_t capacity, std::int32_t length, std::int32_t* status);
std::int32_t APS5_VABI u_terminateUChar32s_nid_postfix(std::int32_t* destination, std::int32_t capacity, std::int32_t length, std::int32_t* status);
std::int8_t APS5_VABI uprv_isInvariantString_nid_postfix(const char* text, std::int32_t length);
std::int8_t APS5_VABI uprv_isInvariantUString_nid_postfix(const char16_t* text, std::int32_t length);
}
