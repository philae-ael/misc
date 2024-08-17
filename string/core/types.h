// IWYU pragma: private, include "core.h"

#ifndef INCLUDE_CORE_TYPE_H_
#define INCLUDE_CORE_TYPE_H_

#include <stddef.h>
#include <stdint.h>

#define FWD(t) decltype(t)(t)

#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#define MIN(a, b) ((a) < (b)) ? (a) : (b)

#define KB(x) ((x) * 1024)
#define MB(x) ((x) * 1024 * 1024)
#define GB(x) ((x) * 1024 * 1024 * 1024)

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8 = uint8_t;
using s16 = uint16_t;
using s32 = uint32_t;
using usize = size_t;
using uptr = uintptr_t;

namespace core {

template <class T> struct Maybe {
  enum class Discriminant : u8 { None, Some } d;
  union {
    T t;
  };

  Maybe(T t) : d(Discriminant::Some), t(t) {}
  Maybe() : d(Discriminant::None) {}

  inline bool is_some() { return d == Discriminant::Some; }
  inline bool is_none() { return d == Discriminant::None; }
  inline T &value() { return t; }
};

} // namespace core

#endif // INCLUDE_CORE_TYPE_H_
