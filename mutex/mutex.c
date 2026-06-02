#include <errno.h>
#include <linux/futex.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <stdatomic.h>
#include <stdbool.h>

#include <assert.h>

#include <threads.h>

enum { Mutex_Unlocked, Mutex_Locked, Mutex_Locked_With_Waiter };

typedef struct Mutex {
  _Atomic(uint32_t) futex;
} Mutex;

static_assert(_Alignof(Mutex) >= 4,
              "Mutex should be aligned on a four-byte-boundary");

bool Mutex_lock_timeout(Mutex *m, const struct timespec *_Nullable timeout) {
  uint32_t fut = Mutex_Unlocked;
  if (atomic_compare_exchange_strong_explicit(&m->futex, &fut, Mutex_Locked,
                                              memory_order_acquire,
                                              memory_order_relaxed)) {
    return true;
  }

  for (;;) {
    switch (fut) {
    case Mutex_Unlocked:
    case Mutex_Locked:
      fut = atomic_exchange_explicit(&m->futex, Mutex_Locked_With_Waiter,
                                     memory_order_acquire);
      break;
    case Mutex_Locked_With_Waiter:
      break;
    }

    if (fut == Mutex_Unlocked)
      return true;
    long err = syscall(SYS_futex, &m->futex, FUTEX_WAIT,
                       Mutex_Locked_With_Waiter, timeout);
    if (err < 0) {
      switch (errno) {
      case ETIMEDOUT:
        return false;
      case EINVAL:
        // invalid timespec...
        abort();
      default:
        break;
      }
    }
    fut = atomic_load_explicit(&m->futex, memory_order_relaxed);
  }
}

void Mutex_lock(Mutex *m) { Mutex_lock_timeout(m, NULL); }

void Mutex_unlock(Mutex *m) {
  uint32_t val =
      atomic_exchange_explicit(&m->futex, Mutex_Unlocked, memory_order_release);
  switch (val) {
  case Mutex_Unlocked:
    assert(false && "trying to unlock an already unlocked mutex");
    return;
  case Mutex_Locked:
    return;
  case Mutex_Locked_With_Waiter:
    syscall(SYS_futex, &m->futex, FUTEX_WAKE, 1);
  }
}

Mutex m = {};

#define NUM_THREAD 64
#define SUM_SIZE 100

int thread_func(void *s) {
  uint64_t *sum = (uint64_t *)s;
  for (int i = 0; i < SUM_SIZE; i++) {
    Mutex_lock(&m);

    *sum += i;

    Mutex_unlock(&m);
  }
  return 0;
}

int main() {
  thrd_t other[NUM_THREAD];

  uint64_t sum = 0;

  for (int i = 0; i < NUM_THREAD; i++) {
    thrd_create(&other[i], thread_func, &sum);
  }

  for (int i = 0; i < NUM_THREAD; i++) {
    int res;
    thrd_join(other[i], &res);
  }

  uint64_t expected = NUM_THREAD * SUM_SIZE * (SUM_SIZE - 1) / 2;
  assert(expected == sum);
}
