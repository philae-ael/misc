#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <thread>

#define NO_INLINE __attribute__((noinline))
inline void blackbox() { asm volatile(""); }

// From google benchmark
template <class Tp> inline void DoNotOptimize(Tp const &value) {
  asm volatile("" : : "r,m"(value) : "memory");
}

static uint64_t bias = 0;
static float nano_per_cycle = 0;

/// Serialize all memory read / writes
inline void fences() { __asm__ volatile("mfence\n\tlfence"); }

inline uint64_t rdtsc() {
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
static void compute_bias() {
  size_t RETRY = 1'000'000;
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

  assert(tstart.tv_sec == tend.tv_sec && "not supported, yet");
  nano_per_cycle =
      (float)(tend.tv_nsec - tstart.tv_nsec) / (float)(rend - rstart);

  printf("Biais: %lu, nano per cycle %.1f | cycle per nano %.1f\n", bias,
         nano_per_cycle, 1.0f / nano_per_cycle);
}

struct bench_res {
  float cycles;
  float cycles_var;
  float ns;
};

template <class F, class... Args>
bench_res bench(const char *name, const size_t retry, F f, Args... args) {
  printf("Bench %s\n", name);

  float mean = 0;
  float m2 = 0;
  float count = 0;
  size_t outlier = 0;

  for (size_t i = 0; i < retry; i++) {
    uint64_t begining = rdtsc();
    fences();
    f(args...);
    fences();
    uint64_t end = rdtsc();

    uint64_t dr = end - begining;
    float x = (float)dr - (float)bias;

    if (count >= 2 && fabs((x - mean) / sqrt(m2 / (count - 1))) > 3.0) {
      // outlier!, skip it
      outlier += 1;
      continue;
    }

    count += 1;
    float mean_next = mean + (x - mean) / count;
    float m2_next = m2 + (x - mean) * (x - mean_next);

    m2 = m2_next;
    mean = mean_next;
  }

  float var = m2 / (count - 1);
  float nanos = mean * nano_per_cycle;

  printf("Took %.2f cycles or %.1f ns, var = %f (%f%% of samples were "
         "skipped))\n",
         mean, nanos, std::sqrt(var), (float)outlier / (float)retry);
  return {
      mean,
      var,
      nanos,
  };
}

static void setup_monothreaded() {
  cpu_set_t cpuset{};
  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);

  pthread_t current_thread = pthread_self();
  pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}
