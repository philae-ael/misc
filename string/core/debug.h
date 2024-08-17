#ifndef INCLUDE_CORE_ASSERT_H_
#define INCLUDE_CORE_ASSERT_H_

#include <source_location>

#define CLANG 1

#if GCC || CLANG
#define TRAP asm volatile("int $3")
#else
#error NOT IMPLEMENTED
#endif

#define STRINGIFY_(c) #c
#define STRINGIFY(c) STRINGIFY_(c)
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT(a, b)

#define ASSERT(cond, ...)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      core::panic("assertion " STRINGIFY(cond) " failed");                     \
    }                                                                          \
  } while (0)

#ifdef DEBUG
#define DEBUG_ASSERT(...) ASSERT(__VA_ARGS)
#else
#define DEBUG_ASSERT(...)
#endif

#ifdef DEBUG
#define DEBUG_STMT(f) f
#else
#define DEBUG_STMT(f)
#endif

#define todo(...) core::panic(__VA_ARGS__);

#define NORETURN [[noreturn]]

namespace core {

NORETURN void panic(const char *msg,
                    std::source_location loc = std::source_location::current());

void dump_backtrace(int skip = 1);

} // namespace core

#endif // INCLUDE_CORE_ASSERT_H_
