// IWYU pragma: private, include "core.h"

#ifndef INCLUDE_CORE_MEMORY_H_
#define INCLUDE_CORE_MEMORY_H_

#include "types.h"

#ifndef SCRATCH_ARENA_AMOUNT
#define SCRATCH_ARENA_AMOUNT 3
#endif

#ifndef DEFAULT_ARENA_CAPACITY
#define DEFAULT_ARENA_CAPACITY KB(5)
#endif

#ifndef ARENA_BLOCK_ALIGNEMENT
#define ARENA_BLOCK_ALIGNEMENT 64
#endif

#define ALIGN_MASK_DOWN(x, mask) ((x) & ~(mask))
#define ALIGN_DOWN(x, AMOUNT) ALIGN_MASK_DOWN((uptr)x, AMOUNT - 1)

#define ALIGN_MASK_UP(x, mask) (((x) + (mask)) & (~(mask)))
#define ALIGN_UP(x, AMOUNT) ALIGN_MASK_UP((uptr)x, AMOUNT - 1)

namespace core {
struct ArenaTemp;
struct Arena {
  u8 *base;
  u8 *mem;
  usize capacity;

  void *allocate(usize size, usize alignement = ARENA_BLOCK_ALIGNEMENT);
  void *try_grow(void *ptr, usize cur_size, usize new_size);

  template <class T> T *allocate() {
    return static_cast<T *>(allocate(sizeof(T), alignof(T)));
  }

  u64 pos();
  void pop_pos(u64 pos);

  ArenaTemp make_temp();
};

Arena *arena_alloc(usize capacity = DEFAULT_ARENA_CAPACITY);
void arena_dealloc(Arena *arena);

struct ArenaTemp {
  Arena *arena;
  u64 old_pos;

  void retire();

  Arena *operator->() { return arena; }
  Arena &operator*() { return *arena; }
};

struct ArenaScratch {
  Arena *arena;
  u64 old_pos;

  Arena *operator->() { return arena; }
  Arena &operator*() { return *arena; }
};

ArenaScratch scratch_get();
void scratch_retire(ArenaScratch);

// TODO: MOVE THAT
usize thread_id();

} // namespace core
#endif // INCLUDE_CORE_MEMORY_H_
