#include "../bench.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <primesieve.hpp>
#include <span>
#include <vector>

// Layout sorted
template <class I> NO_INLINE I find_sorted_naive(std::span<I> s, I x) {
  size_t lo = 0;
  size_t hi = s.size() - 1;

  while (lo < hi) {
    size_t mid = (hi + lo) / 2;
    I mid_v = s[mid];

    if (mid_v > x) {
      hi = mid;
    } else if (mid_v < x) {
      lo = mid + 1;
    } else {
      return mid;
    }
  }
  return hi;
}

template <class I>
NO_INLINE I find_sorted_naive_w_prefetching(std::span<I> s, I x) {
  size_t lo = 0;
  size_t hi = s.size() - 1;

  while (lo < hi) {
    size_t mid = (hi + lo) / 2;
    I mid_v = s[mid];

    __builtin_prefetch(&s[lo + mid / 2]);
    __builtin_prefetch(&s[mid + mid / 2]);
    if (mid_v > x) {
      hi = mid;
    } else if (mid_v < x) {
      lo = mid + 1;
    } else {
      return mid;
    }
  }
  return hi;
}

template <class I>
NO_INLINE size_t find_sorted_kindabranchless1(std::span<I> s, I x) {
  const I *base = s.data();
  size_t n = s.size();

  while (n > 1) {
    size_t half = n / 2;
    // MEH i can't succeed to make a cmov
    // Or event a mul + add
    base += (base[half] < x) * half;
    n -= half;
  }
  return (*base <= x) + (base - s.data());
}

template <class I>
NO_INLINE size_t find_sorted_kindabranchless2(std::span<I> s, I x) {
  size_t lo = 0;
  size_t hi = s.size() - 1;

  while (lo < hi) {
    size_t mid = (hi + lo) / 2;
    I mid_v = s[mid];
    if (mid_v == x) { // BAAAH
      return mid;
    }

    hi = mid_v > x ? mid : hi;
    lo = mid_v < x ? mid + 1 : lo;
  }
  return hi;
}

struct res {
  bench_res sorted_naive;
  bench_res sorted_branchless1;
  bench_res sorted_branchless2;
  bench_res sorted_naive_w_prefetching;
};

res dobench(const size_t PRIME_COUNT, const size_t RETRY) {
  std::vector<uint64_t> primes;
  primes.reserve(PRIME_COUNT);
  primesieve::generate_n_primes(PRIME_COUNT, &primes);
  printf("Computed first %lu primes.\n", PRIME_COUNT);

  uint64_t n = 4057;

  uint64_t expected = find_sorted_naive<uint64_t>(primes, n);
  assert(find_sorted_kindabranchless1<uint64_t>(primes, n) == expected);
  assert(find_sorted_kindabranchless2<uint64_t>(primes, n) == expected);

  return {
      bench("sorted naive", RETRY, find_sorted_naive<uint64_t>, primes, n),
      bench("sorted branchless 1", RETRY,
            find_sorted_kindabranchless1<uint64_t>, primes, n),
      bench("sorted branchless 2", RETRY,
            find_sorted_kindabranchless2<uint64_t>, primes, n),
      bench("naive w prefetching", RETRY,
            find_sorted_naive_w_prefetching<uint64_t>, primes, n),
  };
}

int main(int argc, char *argv[]) {
  setup_monothreaded();
  FILE *f = fopen("res.csv", "w");
  fprintf(f, "N;sorted naive;sorted branchless1;sorted branchless2;sorted "
             "naive w prefetching\n");

  compute_bias();
  const size_t RETRY = 1'000'000;
  for (size_t n = 10; n < 20; n++) {
    printf("PRIME COUNT = 1 << %zu\n", n);
    res r = dobench(1 << n, RETRY);

    fprintf(f, "%d;%f;%f;%f;%f\n", 1 << n, r.sorted_naive.ns,
            r.sorted_branchless1.ns, r.sorted_branchless2.ns,
            r.sorted_naive_w_prefetching.ns);
  }

  fclose(f);
  return 0;
}
