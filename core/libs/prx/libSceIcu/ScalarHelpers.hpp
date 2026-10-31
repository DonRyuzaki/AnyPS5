#pragma once
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>

extern "C" {
char16_t APS5_VABI u_asciiToUpper_nid_postfix(char16_t value);
char APS5_VABI uprv_asciitolower_nid_postfix(char value);
std::int8_t APS5_VABI uprv_isASCIILetter_nid_postfix(char value);
char APS5_VABI uprv_toupper_nid_postfix(char value);
std::int32_t APS5_VABI uprv_uint16Comparator_nid_postfix(const void* context, const void* left, const void* right);
std::int32_t APS5_VABI uprv_uint32Comparator_nid_postfix(const void* context, const void* left, const void* right);
int APS5_VABI uprv_stricmp_nid_postfix(const char* first, const char* second);
int APS5_VABI uprv_strnicmp_nid_postfix(const char* first, const char* second, std::uint32_t count);
std::int32_t APS5_VABI uprv_compareASCIIPropertyNames_nid_postfix(const char* first, const char* second);
std::int32_t APS5_VABI ustr_hashUCharsN_nid_postfix(const char16_t* text, std::int32_t length);
}
