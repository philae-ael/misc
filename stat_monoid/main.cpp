#include "./stat.h"
#include <cstdio>

int main(int argc, char *argv[]) {
  stat_monoid m1;
  m1.push(5.0);
  m1.push(2.0);
  stat_monoid m2;
  m1.push(8.0);
  m1.push(2.0);

  auto s = m1 + m2;
  printf("count, %f, mean %f, variance %f", s.count(), s.mean(), s.variance());

  return 0;
}
