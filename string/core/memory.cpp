#include "core.h"
#include "debug.h"
#include <atomic>
#include <bit>
#include <cstdio>
#include <cstdlib>

#ifdef SCRATCH_DEBUG
#define SCRATCH_DEBUG_STMT(f) f
#else
#define SCRATCH_DEBUG_STMT(f)
#endif

#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#define ASAN_POISON_MEMORY_REGION(addr, size)                                  \
  __asan_poison_memory_region((addr), (size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size)                                \
  __asan_unpoison_memory_region((addr), (size))
#else
#define ASAN_POISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#endif

namespace core {

#ifdef ARENA_DEBUG
#define ARENA_DEBUG_STMT(f) DEBUG_STMT(f)
#else
#define ARENA_DEBUG_STMT(f)
#endif

void *Arena::allocate(usize size, usize alignement) {
  ARENA_DEBUG_STMT(fprintf(stderr, "allocating %zu\n", size));
  ASSERT(std::popcount(alignement) == 1);

  if (size == 0) {
    return nullptr;
  }

  u8 *allocated = (u8 *)ALIGN_UP(mem, alignement);
  ASSERT(base + capacity >= allocated + size);

  ARENA_DEBUG_STMT(fprintf(stderr, "old mem %p, new mem %p, size: %zu\n", mem,
                           allocated + size, size));
  mem = allocated + size;

  ASAN_UNPOISON_MEMORY_REGION(allocated, size);
  return allocated;
}

void *Arena::try_grow(void *ptr, usize cur_size, usize new_size) {
  if (ptr < base || ptr >= mem) {
    return nullptr;
  }

  if ((uptr)ptr + cur_size != (uptr)mem) {
    return nullptr;
  }

  if (new_size >= cur_size) {
    allocate(new_size - cur_size, 1);
  } else {
    mem = mem + new_size - cur_size;
    ASAN_POISON_MEMORY_REGION(mem, capacity - (mem - base));
  }

  return ptr;
}

Arena *arena_alloc(usize capacity) {
  u8 *memory = (u8 *)::calloc(sizeof(Arena) + capacity, 1);
  ASSERT(memory != nullptr);

  Arena *arena = (Arena *)memory;
  arena->capacity = capacity;
  arena->mem = arena->base = memory + sizeof(Arena);

  ASAN_POISON_MEMORY_REGION(arena->mem, arena->capacity);
  return arena;
}

void arena_dealloc(Arena *arena) { free(arena); }

void Arena::pop_pos(u64 pos) {
  mem = (u8 *)pos;
  ASAN_POISON_MEMORY_REGION(mem, capacity - (mem - base));
}
u64 Arena::pos() { return (u64)mem; }
void ArenaTemp::retire() {
  arena->pop_pos(old_pos);
  arena = nullptr;
}
ArenaTemp Arena::make_temp() {
  return ArenaTemp{
      this,
      pos(),
  };
}

struct ScratchArenaCleaner {
  std::atomic<Arena *> inner{};

  std::atomic<Arena *> &operator*() { return inner; }
  std::atomic<Arena *> *operator->() { return &inner; }
  ~ScratchArenaCleaner() {
    Arena *arena = inner.exchange(nullptr);
    if (arena != nullptr) {
      arena_dealloc(arena);

      SCRATCH_DEBUG_STMT(
          printf("scratch: thread %zu destroying arena \n", thread_id()));
    }
  }
};

thread_local ScratchArenaCleaner scratch_storage[SCRATCH_ARENA_AMOUNT]{};

ArenaScratch scratch_get() {
  if (*scratch_storage[0] == nullptr) {
    for (usize i = 0; i < SCRATCH_ARENA_AMOUNT; i++) {
      Arena *arena = arena_alloc();
      Arena *expected = nullptr;
      if (!scratch_storage[i]->compare_exchange_strong(expected, arena)) {
        arena_dealloc(arena);
      }
    }
  }

  for (usize i = 0; i < SCRATCH_ARENA_AMOUNT; i++) {
    usize j = SCRATCH_ARENA_AMOUNT - 1 - i;
    Arena *arena;
    while ((arena = *scratch_storage[j]) != nullptr) {
      if (!scratch_storage[j]->compare_exchange_strong(arena, nullptr)) {
        continue;
      }

      SCRATCH_DEBUG_STMT(printf("scratch: thread %zu, got scratch %zu at %p\n",
                                thread_id(), j, arena->base));

      return ArenaScratch{.arena = arena, .old_pos = arena->pos()};
    }
  }
  panic("no scratch arena available");
}

void scratch_retire(ArenaScratch arena) {
  arena->pop_pos(arena.old_pos);

  for (usize i = 0; i < SCRATCH_ARENA_AMOUNT; i++) {
    while (*scratch_storage[i] == nullptr) {
      Arena *expected = nullptr;
      if (!scratch_storage[i]->compare_exchange_strong(expected, arena.arena)) {
        continue;
      }

      SCRATCH_DEBUG_STMT(
          printf("scratch: thread %zu, retire scratch %zu at %p\n", thread_id(),
                 i, arena->base));
      return;
    }
  }

  arena_dealloc(arena.arena);
}

std::atomic<usize> thread_counter;

thread_local usize thread_id_ = (usize)-1;

usize thread_id() {
  if (thread_id_ == (usize)-1) {
    thread_id_ = thread_counter.fetch_add(1);
  }
  return thread_id_;
}

} // namespace core
