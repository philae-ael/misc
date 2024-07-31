#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <immintrin.h>

uint64_t rdtsc() {
  uint64_t hi, lo;
  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)lo) | (((uint64_t)hi) << 32);
}

#define RETRY 5000

extern unsigned char data[];

inline void lowercase_0(char *data, size_t l) {
  char c;
  while ((c = *data) != 0) {
    if ('A' <= c && c <= 'Z') {
      *data = c + 'a' - 'A';
    }
    data++;
  }
}

inline void lowercase_smarter(char *data) {
  char c;
  while ((c = *data) != 0) {
    if ('A' <= c && c <= 'Z') {
      *data = (c | 0b100000);
    }
    data++;
  }
}

inline void lowercase_branchless(char *data) {
  uint8_t c;
  while ((c = *data) != 0) {
    uint8_t f = uint8_t(c + 0xFF - 'Z') / ('A' + 0xFF - 'Z');
    *data = c + f * ('a' - 'A');
    data++;
  }
}

inline void lowercase_table(char *data) {
  static const uint8_t table[256] = {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
      15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
      30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
      45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
      60,  61,  62,  63,  64,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106,
      107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
      122, 91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
      105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
      120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
      135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
      150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
      165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
      180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
      195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
      210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
      225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
      240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
      255,
  };
  uint8_t c;
  while ((c = *data) != 0) {
    *data = table[c];
    data++;
  }
}

#define ALIGN_MASK(x, m) (((x) + (m)) & ~(m))

inline __m256i lower_simd(__m256i in) {
  __m256i u = _mm256_add_epi8(in, _mm256_set1_epi8(char(0xff) - 'Z'));
  __m256i t = _mm256_set1_epi8('A' + char(0xff) - 'Z');

  __mmask32 m = _mm256_cmp_epu8_mask(u, t, _MM_CMPINT_GE);

  return _mm256_mask_add_epi8(in, m, in, _mm256_set1_epi8('a' - 'A'));
}

inline void lowercase_simd(char *data, size_t length) {
  while (length > 256 / 8) {
    __m256i d = _mm256_loadu_epi8(data);

    _mm256_storeu_epi8(data, lower_simd(d));
    length -= 32;
    data += 32;
  }

  lowercase_table(data);
}

int main(int argc, char *argv[]) {
  auto base = strdup((char *)data);
  size_t l = strlen((char *)data);
  lowercase_0(base, l);

  {
    uint64_t sum = 0;

    clock_t before = clock();
    for (size_t r = 0; r < RETRY; r++) {
      auto d = strdup((char *)data);

      uint64_t start = rdtsc();
      lowercase_0(d, l);
      uint64_t end = rdtsc();

      free(d);

      sum += end - start;
    }
    clock_t after = clock();
    float msec = float(after - before) / (float)CLOCKS_PER_SEC;

    printf("lowercase_0 took: %.4f byte per cycle, took %.2f ms\n",
           strlen((char *)data) * RETRY / (float)sum, msec);
  }

  {
    auto d = strdup((char *)data);
    lowercase_smarter(d);
    assert(strcmp(base, d) == 0);
    free(d);

    uint64_t sum = 0;

    clock_t before = clock();
    for (size_t r = 0; r < RETRY; r++) {
      auto d = strdup((char *)data);

      uint64_t start = rdtsc();
      lowercase_smarter(d);
      uint64_t end = rdtsc();

      free(d);

      sum += end - start;
    }
    clock_t after = clock();
    float msec = float(after - before) / (float)CLOCKS_PER_SEC;

    printf("lowercase_smarter took: %.4f byte per cycle, %.2fmsec\n",
           strlen((char *)data) * RETRY / (float)sum, msec);
  }

  {
    auto d = strdup((char *)data);
    lowercase_branchless(d);
    assert(strcmp(base, d) == 0);
    free(d);

    uint64_t sum = 0;

    clock_t before = clock();
    for (size_t r = 0; r < RETRY; r++) {
      auto d = strdup((char *)data);

      uint64_t start = rdtsc();
      lowercase_branchless(d);
      uint64_t end = rdtsc();

      free(d);

      sum += end - start;
    }
    clock_t after = clock();
    float msec = float(after - before) / (float)CLOCKS_PER_SEC;
    printf("lowercase_branchless took: %.4f byte per cycle, %.2fmsec\n",
           strlen((char *)data) * RETRY / (float)sum, msec);
  }
  {
    auto d = strdup((char *)data);
    lowercase_table(d);
    assert(strcmp(base, d) == 0);
    free(d);

    uint64_t sum = 0;

    clock_t before = clock();
    for (size_t r = 0; r < RETRY; r++) {
      auto d = strdup((char *)data);

      uint64_t start = rdtsc();
      lowercase_table(d);
      uint64_t end = rdtsc();

      free(d);

      sum += end - start;
    }
    clock_t after = clock();
    float msec = float(after - before) / (float)CLOCKS_PER_SEC;

    printf("lowercase_table took: %.4f byte per cycle, %.2fmsec\n",
           strlen((char *)data) * RETRY / (float)sum, msec);
  }

  {
    auto d = strdup((char *)data);
    lowercase_simd(d, l);
    assert(strcmp(base, d) == 0);
    free(d);

    uint64_t sum = 0;

    clock_t before = clock();
    for (size_t r = 0; r < RETRY; r++) {
      auto d = strdup((char *)data);

      uint64_t start = rdtsc();
      lowercase_simd(d, l);
      uint64_t end = rdtsc();

      free(d);

      sum += end - start;
    }
    clock_t after = clock();
    float msec = float(after - before) / (float)CLOCKS_PER_SEC;

    printf("lowercase_simd took: %.4f byte per cycle, %.2fmsec\n",
           strlen((char *)data) * RETRY / (float)sum, msec);
  }
}
