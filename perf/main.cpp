#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>

#define NO_INLINE __attribute__((noinline))
inline void blackbox() { asm volatile(""); }

uint64_t bias = 0;
float nano_per_cycle = 0;

/// Serialize all memory read / writes
void fences() { __asm__ volatile("mfence\n\tlfence"); }

uint64_t rdtsc() {
  uint64_t hi, lo;
  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)lo) | (((uint64_t)hi) << 32);
}

/// compute the time added by the sequence
/// a = RDTSC()
/// fences()
/// fences()
/// b = RDTSC()
///
/// ~40 cycles on my machine (due to fences?!)
void compute_bias() {
  size_t RETRY = 10'000'000;
  printf("computing bias and timings\n");

  struct timespec tstart = {0, 0}, tend = {0, 0};
  clock_gettime(CLOCK_MONOTONIC, &tstart);
  uint64_t rstart = rdtsc();
  {
    uint64_t sum = 0;
    for (size_t i = 0; i < RETRY; i++) {
      uint64_t begining = rdtsc();
      fences();
      fences();
      uint64_t end = rdtsc();

      sum += end - begining;
    }
    float cycle_per_call = (float)sum / (float)RETRY;
    bias = (uint64_t)cycle_per_call;
  }

  // Is order optimal ?!
  clock_gettime(CLOCK_MONOTONIC, &tend);
  uint64_t rend = rdtsc();

  assert(tstart.tv_sec == tend.tv_sec);
  nano_per_cycle =
      (float)(tend.tv_nsec - tstart.tv_nsec) / (float)(rend - rstart);

  printf("Biais: %lu, nano per sec %f\n", bias, nano_per_cycle);
}

template <class F, class... Args>
void bench(const char *name, size_t retry, F f, Args... args) {
  printf("Bench %s\n", name);

  float mean = 0;
  float var = 0;

  for (size_t i = 1; i < retry + 1; i++) {
    uint64_t begining = rdtsc();
    fences();
    f(args...);
    fences();
    uint64_t end = rdtsc();

    uint64_t dr = end - begining;
    float x = (float)dr - (float)bias;

    float delta1 = x - mean;
    mean += (x - mean) / (float)i;
    float delta2 = x - mean;
    var = (float)(i * (i - 1)) * var + delta1 * delta2;
  }

  float nanos = mean * nano_per_cycle;

  printf("Took %.2f cycles or %.1f ns, var = %.f", mean, nanos, var);
}

NO_INLINE void f() { blackbox(); }

int main() {
  compute_bias();
  bench("call", 1'000, f);
}
