#pragma once

#ifdef __INTELLISENSE__

// Get rid of predefined MSVC macros:
#undef _WIN32
#undef _WIN64
                
#undef _ATL_VER
#undef __CLR_VER
#undef _M_CEE
#undef _M_CEE_PURE
#undef _M_CEE_SAFE
#undef _MANAGED       
#undef __cplusplus_cli

#undef _M_IX86
#undef _M_IX86_FP
#undef _M_AMD64
#undef _M_X64
#undef _MSC_VER       
#undef _MSC_FULL_VER
#undef _MSC_BUILD
#undef _MSC_EXTENSIONS

#undef _CHAR_UNSIGNED
#undef _INTEGRAL_MAX_BITS

// These are defined in gcc_defines too:
#undef __cpp_constexpr
#undef __cplusplus
#undef __has_include
#undef __cpp_attributes
#undef __cpp_static_assert
#undef __cpp_binary_literals
#undef __cpp_variadic_templates
#undef __STDC_HOSTED__
#undef __cpp_raw_strings
#undef __cpp_lambdas
#undef __cpp_unicode_characters
#undef __cpp_unicode_literals
#undef __EXCEPTIONS
#undef __cpp_decltype
#undef __CHAR32_TYPE__
#undef __cpp_user_defined_literals
#undef __cpp_init_captures

#include "gcc_defines.h"

// These were defined in gcc_defines, redefine them to be intellisense compatible:
#undef __SIZE_TYPE__
#define __SIZE_TYPE__ size_t
using size_t = size_t;  // yes...

// GCC extensions:

#define __float128 long double
#define __null 0

#endif
