#ifndef INCLUDE_CORE_PLATFORM_H_
#define INCLUDE_CORE_PLATFORM_H_

#include "fwd.h"

/// ** Compiler **
namespace core {

#if defined(__clang__)
#define CLANG 1
#elif defined(__GNUC__) || defined(__GNUG__)
#define GCC 1
#elif defined(_MSC_VER)
#define MSCV 1
#else
#error Unknown Compiler
#endif

/// ** OS **
#if defined(__linux__)
#define LINUX 1
#elif defined(__WIN32)
#define WINDOWS 1
#else
#error Unknown OS
#endif

enum class Os { Windows, Linux };
str8 to_str8(Os os);
template <> Maybe<Os> from_hstr8<Os>(hstr8 h);

// ** Architecture **

#if defined(__x86_64__)
#define X86_64 1
#else
#error Unknown OS
#endif

enum class Architecture { Windows, Linux };

#ifndef GCC
#define GCC 0
#endif
#ifndef CLANG
#define CLANG 0
#endif

#ifndef MSVC
#define MSCV 0
#endif

#ifndef WINDOW
#define WINDOWS 0
#endif

#ifndef LINUX
#define LINUX 0
#endif

#ifndef X86_64
#define X86_64 0
#endif

} // namespace core
#endif // INCLUDE_CORE_PLATFORM_H_
