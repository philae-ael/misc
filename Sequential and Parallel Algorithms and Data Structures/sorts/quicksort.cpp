#include "sortcommon.h"
#include <cstdio>

pair<span<int>, span<int>> partition(span<int> s) {
  int pivot = s[s.size() - 1];

  usize sizep1 = 0;
  for (usize i = 0; i < s.size(); i++) {
    if (s[i] < pivot) {
      swap(s[i], s[sizep1]);
      sizep1++;
    }
  }

  swap(s[s.size() - 1], s[sizep1]);

  return {
      s.slice(sizep1, 0),
      s.slice(s.size() - sizep1 - 1, sizep1 + 1),
  };
}

void quicksort(span<int> s) {
  if (s.size() < 2) {
    return;
  }

  auto [p1, p2] = partition(s);
  quicksort(p1);
  quicksort(p2);
}

int main(int argc, char *argv[]) {
  int arr[]{7, 8, 9, 5, 1, 6, 8, 4, 0};
  span arrs{arr};
  quicksort(arrs);
  arrs.print();

  int expected[]{0, 1, 4, 5, 6, 7, 8, 8, 9};
  span expecteds{expected};
  expecteds.print();
  assert(arrs == expecteds);
  printf("OK!");

  return 0;
}
