#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <utility>

#define MAX(a, b) ((a) < (b) ? (b) : (a))

// A non resizable vector
// Use it as an array, but malloced
template <class T> class dyn_array {
  size_t _size;
  size_t _capacity;
  union T_opt {
    std::byte b[sizeof(T)];
    T t;

    // SHOULD BE MANAGED BY DYN_ARRAY DIRECTLY
    T_opt(){};
    ~T_opt(){};
  };
  T_opt *_data;

public:
  dyn_array(const dyn_array &) = delete;
  dyn_array &operator=(const dyn_array &) = delete;

  dyn_array(dyn_array &&other) noexcept
      : _size(0), _capacity(0), _data(nullptr) {
    *this = std::move(other);
  }
  dyn_array &operator=(dyn_array &&other) noexcept {
    if (&other == this) {
      return *this;
    }
    delete[] _data;
    _size = std::exchange(other._size, 0);
    _capacity = std::exchange(other._capacity, 0);
    _data = std::exchange(other._data, nullptr);
    return *this;
  };

  dyn_array(const size_t cap) : _capacity(cap), _size(0) {
    _data = new T_opt[cap]();
  }

  T *data() { return &_data->t; }
  T *begin() { return data(); }
  T *end() { return data() + size(); }
  const T *data() const { return &_data->t; }
  const T *begin() const { return data(); }
  const T *end() const { return data() + size(); }

  void push(T t) {
    assert(_size != _capacity);
    _data[_size].t = t;
    _size++;
  }

  template <class... Args> void emplace(Args &&...args) {
    assert(_size != _capacity);
    new (&_data[_size].t) T(std::forward<Args>(args)...);
    _size++;
  }

  void pop() {
    assert(_size > 0);
    _size--;

    // TODO: if not trivially destructible
    _data[_size].t.~T();
  }

  size_t size() const { return _size; }
  size_t capacity() const { return _capacity; }

  const T &operator[](size_t index) const {
    assert(index < _size);
    return _data[index].t;
  }
  T &operator[](size_t index) {
    assert(index < _size);
    return _data[index].t;
  }

  ~dyn_array() {
    // TODO: if not trivially destructible
    for (size_t i = 0; i < _size; i++) {
      _data[i].t.~T();
    }

    // if moved, data is nullptr that can be deleted
    delete[] _data;
  }
};

template class dyn_array<size_t>;

// #define ASSERT_HEAP_COSTLY(a) assert(a)
#define ASSERT_HEAP_COSTLY(a)

// A dyn array but deletion without swap is possible and keep handles stable
// Thx to a free list
template <class T> class stable_dyn_array {
  // We can add a sentinel so that the array can be iterable
  // Or a bit vector
  struct I {
    bool occupied;
    union {
      T t;
      size_t next_free;
    };
    template <class... Args>
    I(Args &&...args) : occupied(true), t(std::forward<Args>(args)...) {}
    ~I() {
      if (occupied) {
        t.~T();
      }
    }
  };

  dyn_array<I> items;

  static constexpr size_t NO_MORE_ITEM_IN_FREE_LIST =
      std::numeric_limits<size_t>::max();
  size_t next_free = NO_MORE_ITEM_IN_FREE_LIST;

public:
  stable_dyn_array(size_t size) : items(size) {}
  template <class... Args> size_t emplace(Args &&...args) {
    if (next_free != NO_MORE_ITEM_IN_FREE_LIST) {
      size_t index = next_free;
      next_free = items[index].next_free;

      new (&items[index]) I(std::forward<Args>(args)...);
      return index;
    } else {
      items.emplace(std::forward<Args>(args)...);
      return items.size() - 1;
    }
  }

  T &operator[](size_t index) {
    I &item = items[index];
    assert(item.occupied);
    return item.t;
  }

  const T &operator[](size_t index) const {
    const I &item = items[index];
    assert(item.occupied);
    return item.t;
  }

  void remove(size_t index) {
    I &item = items[index];
    assert(item.occupied);

    item.t.~T();
    item.occupied = false;
    item.next_free = next_free;

    next_free = index;
  }
};

// 6.1 of the book
// Heavily updated for it to be addressable by using a stable_dyn_array
template <class T, class Cmp = std::less_equal<T>> class heap {
  // be wary, me, heaps use indices 1..n and not 0..n
  // invariant:  h(floor(j/2)) <= h(j)
public:
  struct heap_handle {
    size_t handle;
  };

private:
  struct item {
    size_t heap_index;
    T t;
  };

  stable_dyn_array<item> ts;
  dyn_array<size_t> inner_heap;
  Cmp _cmp;

public:
  heap(size_t size, Cmp cmp) : ts(size), inner_heap(size), _cmp(cmp) {}
  heap(size_t size) : heap(size, {}) {}

  T &minimum() {
    assert(inner_heap.size() > 0);
    return ts[inner_heap[0]].t;
  }
  heap_handle minimum_handle() {
    assert(inner_heap.size() > 0);
    return {inner_heap[0]};
  }

  heap_handle insert(T &&t) {
    assert(inner_heap.size() < inner_heap.capacity());
    size_t idx = ts.emplace(inner_heap.size(), std::move(t));
    inner_heap.push(idx);
    siftup(size());

    return {idx};
  }
  size_t size() const { return inner_heap.size(); }
  size_t capacity() const { return inner_heap.capacity(); }

  void remove_minimum() {
    assert(inner_heap.size() > 0);
    size_t item_idx = inner_heap[0];
    swap(1, size());

    ts.remove(item_idx);
    inner_heap.pop();

    siftdown(1);
    ASSERT_HEAP_COSTLY(is_heap(1, size()));
  }

  // Bc addressable
  void update(heap_handle handle, auto Fn) {
    Fn(ts[handle.handle].t);

    siftup(siftdown(ts[handle.handle].heap_index + 1));
  }
  T &operator[](heap_handle handle) { return ts[handle.handle].t; }
  const T &operator[](heap_handle handle) const { return ts[handle.handle].t; }

private:
  void swap(size_t i, size_t j) {
    ASSERT_HEAP_COSTLY(handles_are_valid());

    std::swap(ts[inner_heap[i - 1]].heap_index,
              ts[inner_heap[j - 1]].heap_index);
    std::swap(inner_heap[i - 1], inner_heap[j - 1]);

    ASSERT_HEAP_COSTLY(handles_are_valid());
  }
  bool handles_are_valid() const {
    for (size_t i = 0; i < size(); i++) {
      if (i != ts[inner_heap[i]].heap_index) {
        return false;
      }
    }
    return true;
  }
  bool cmp(size_t i, size_t j) const {
    return _cmp(ts[inner_heap[i - 1]].t, ts[inner_heap[j - 1]].t);
  }

  // NOTE: INDEXED FROM 1 IN THOSE FUNCTION
  size_t siftup(size_t i) {
    ASSERT_HEAP_COSTLY(is_heap(1, i - 1));

    size_t parent_index = i / 2;

    if (i == 1 || cmp(parent_index, i)) {
      return i;
    }
    swap(parent_index, i);
    return siftup(parent_index);
  }

  // Sift down is not optimal an should be improved
  size_t siftdown(size_t i) {
    ASSERT_HEAP_COSTLY(is_heap(2 * i, inner_heap.size()));
    ASSERT_HEAP_COSTLY(is_heap(2 * i + 1, inner_heap.size()));

    if (2 * i > inner_heap.size()) {
      return i;
    }

    size_t m = 2 * i + 1;
    if (2 * i + 1 > inner_heap.size() || cmp(2 * i, 2 * i + 1)) {
      m = 2 * i;
    }
    size_t new_i = i;
    if (!cmp(i, m)) {
      swap(i, m);
      siftdown(m);
      new_i = m;
    }

    ASSERT_HEAP_COSTLY(is_heap(i, inner_heap.size()));
    return new_i;
  }

  bool is_heap(size_t root, size_t upto) const {
    if (2 * root <= upto) {
      if (!cmp(root, 2 * root) || !is_heap(2 * root, upto)) {
        return false;
      }
    }
    if (2 * root + 1 <= upto) {
      if (!cmp(root, 2 * root + 1) || !is_heap(2 * root + 1, upto)) {
        return false;
      }
    }
    return true;
  }
};

template <class K, class V> class LRU {
public:
  struct I {
    size_t t;
    K k;
    V v;
    friend bool operator<=(const I &a, const I &b) { return a.t >= b.t; }
  };
  heap<I> h;
  using handle_t = heap<I>::heap_handle;
  std::unordered_map<K, handle_t> key2handle;
  size_t time = 0;

  LRU(size_t size) : h(size) { key2handle.reserve(4 * size); }

  std::optional<std::reference_wrapper<V>> find(K k) {
    auto it = key2handle.find(k);
    if (it != key2handle.end()) {
      handle_t handle = it->second;
      h.update(handle, [this](I &i) { i.t = time++; });
      return h[handle].v;
    }
    return {};
  }

  V &insert(K k, V v) {
    handle_t handle;

    I new_i{time++, k, v};
    if (h.size() == h.capacity()) {
      key2handle.erase(h.minimum().k);

      handle = h.minimum_handle();
      h.update(handle, [&new_i](I &i) { i = std::move(new_i); });
    } else {
      handle = h.insert(std::move(new_i));
    }
    key2handle.emplace(k, handle);
    return h[handle].v;
  }

  V &find_or_insert(K k, auto Fn) {
    auto a = find(k);
    if (a) {
      return a.value();
    };
    auto v = Fn();
    return insert(k, v);
  }
};

template class LRU<int, std::string>;

void test_heap() {
  heap<size_t> s(5);
  s.insert(1);
  s.insert(3);
  s.insert(2);
  s.insert(0);
  assert(s.size() == 4);
  assert(s.minimum() == 0);
  s.remove_minimum();
  assert(s.size() == 3);
  assert(s.minimum() == 1);
  s.remove_minimum();
  assert(s.size() == 2);
  assert(s.minimum() == 2);
  s.remove_minimum();
  assert(s.size() == 1);
  assert(s.minimum() == 3);
  s.remove_minimum();
  assert(s.size() == 0);

  struct NewType {
    size_t i;
    size_t v;
  };

  auto cmp = [](const NewType &a, const NewType &b) { return a.i < b.i; };
  heap<NewType, decltype(cmp)> s2(2, cmp);
  s2.insert(NewType{1, 1});
  auto h = s2.insert(NewType{0, 2});
  s2.update(h, [](NewType &n) {
    assert(n.i == 0);
    assert(n.v == 2);
    n.i = 3;
  });
  assert(s2.size() == 2);
  assert(s2.minimum().i == 1);
  assert(s2.minimum().v == 1);
  s2.remove_minimum();
  assert(s2.size() == 1);
  assert(s2.minimum().i == 3);
  assert(s2.minimum().v == 2);
  s2.remove_minimum();
  assert(s2.size() == 0);

  printf("Heap seems to work!\n");
}

void test_lru() {
  LRU<int, std::string> lru(2);
  {
    auto &v = lru.find_or_insert(0, []() { return "1234"; });
    assert(v == "1234");
  }
  {
    auto &v = lru.find_or_insert(0, []() {
      assert(false);
      return "";
    });
    assert(v == "1234");
  }

  {
    bool did_insert = false;
    auto &v = lru.find_or_insert(1, [&did_insert]() {
      did_insert = true;
      return "pi";
    });
    assert(did_insert);
    assert(v == "pi");
  }
  {
    auto &v = lru.find_or_insert(1, []() {
      assert(false);
      return "";
    });
    assert(v == "pi");
  }

  lru.find_or_insert(0, []() { return "213"; });
  lru.find_or_insert(2, []() { return "213"; });
  lru.find_or_insert(1, []() {
    assert(false);
    return "213";
  });
  {
    bool did_insert = false;
    lru.find_or_insert(0, [&did_insert]() {
      did_insert = true;
      return "213";
    });
    assert(did_insert);
  }
  printf("LRU seems to work!\n");
}

void blackbox(auto &r) { __asm__ volatile("" : "+g"(r) : :); }

int do_bench(LRU<unsigned int, unsigned int> &lru,
             std::span<unsigned int> data) {
  int misses = 0;
  for (size_t i = 0; i < data.size(); i++) {
    auto k = data[i];

    auto u = lru.find(k);
    int v;
    if (u) {
      v = u.value();
    } else {
      misses += 1;
      v = lru.insert(k, 0);
    }
    blackbox(v);
  }
  return misses;
}

struct rng_lehmer64 {
  __uint128_t g_lehmer64_state;
  rng_lehmer64(uint64_t seed) : g_lehmer64_state(seed) {}

  uint64_t operator()() {
    g_lehmer64_state *= 0xda942042e4dd58b5;
    return g_lehmer64_state >> 64;
  }
};

#define printf(...)

std::pair<int, float> bench_lru(size_t item_count, size_t lru_size,
                                size_t iter_count, bool prefill) {
  LRU<unsigned int, unsigned int> lru(lru_size);

  dyn_array<unsigned int> data(iter_count);
  {
    printf("generating random data...\n");
    auto before = std::chrono::steady_clock::now();
    size_t size = 0;
    rng_lehmer64 rng(6);
    for (size_t i = 0; i < iter_count; i++) {
      unsigned int k = rng() % item_count;
      data.push(k);
      if (prefill) {
        lru.find_or_insert(k, []() { return 0; });
      }
    }

    int msec = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - before)
                   .count();
    printf("Done in %dms, msec!\n", msec);
  }

  float expected = float(item_count) / float(lru_size);
  printf("fill rate: %.2f, expected: %.2f\n",
         float(lru.h.size()) / float(lru.h.capacity()),
         expected > 1.0 ? 1.0 : expected);

  auto before = std::chrono::steady_clock::now();
  auto miss = do_bench(lru, data);
  int msec = std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::steady_clock::now() - before)
                 .count();

  printf("Took %dms\n", msec);
  return {msec, float(miss) / float(iter_count)};
}

int main(int argc, char *argv[]) {
  test_heap();
  test_lru();

  // np.logspace(1, 4, 10)
  std::array item_counts{16384};
  std::array lru_sizes{2,    4,     8,     16,    32,    64,    128,  256,
                       512,  1024,  1184,  1371,  1586,  1835,  2124, 2457,
                       2843, 3290,  3807,  4406,  5098,  5899,  6826, 7898,
                       9139, 10575, 12236, 14159, 16384, 18000, 20000};

  std::array iter_counts{100,   3252,  6405,  9557,  12710, 15863, 19015,
                         22168, 25321, 28473, 31626, 34778, 37931, 41084,
                         44236, 47389, 50542, 53694, 56847, 60000, 100000};

  auto f = fopen("out.csv", "w");
  fprintf(f, "iter_count;item_count;lru_size;time;missrate\n");
  bool prefill = false;
  for (auto item_count : item_counts) {
    for (auto lru_size : lru_sizes) {
      for (auto iter_count : iter_counts) {
        printf("iter_count: %d, item_count:  %d, lru_size: %d\n", iter_count,
               item_count, lru_size);
        auto [t, missrate] =
            bench_lru(item_count, lru_size, iter_count, prefill);
        fprintf(f, "%d;%d;%d;%d;%f\n", iter_count, item_count, lru_size, t,
                missrate);
      }
    }
  }
  fclose(f);
  return 0;
}
