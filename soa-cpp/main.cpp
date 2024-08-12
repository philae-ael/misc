// requires that data
#include <array>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <tuple>
#include <utility>

template <class T, size_t idx> struct TYindex {
  using type = T;
  static constexpr std::size_t index = idx;
};

template <class... Ts> struct ListOfTypes {
  template <template <class... Tss> typename Templa>
  using apply_to = Templa<Ts...>;

  template <class F, std::size_t... Is>
  inline static void map_apply_(F f,
                                std::integer_sequence<std::size_t, Is...>) {
    (f(TYindex<Ts, Is>{}), ...);
  }

  template <class F> inline static void map(F f) {
    map_apply_(f, std::make_integer_sequence<std::size_t, sizeof...(Ts)>{});
  }
};

template <class T> struct SOA_Traits {};

namespace detail {
template <class... Ts> using Storage = std::tuple<std::array<Ts, 500>...>;
} // namespace detail

template <class T> class SOA {
  using Traits = SOA_Traits<T>;
  using Storage = Traits::types::template apply_to<detail::Storage>;
  Storage s;
  size_t size_ = 0;

public:
  size_t insert(T t) {
    Traits::types::map([&t, this]<class Ti, size_t idx>(TYindex<Ti, idx>) {
      std::get<idx>(s)[size_] = *reinterpret_cast<Ti *>(
          reinterpret_cast<std::byte *>(&t) + Traits::offsets[idx]);
    });
    return size_++;
  }
  T get(size_t index) {
    alignas(T) std::byte res[sizeof(T)];

    Traits::types::map(
        [&res, &index, this]<class Ti, size_t idx>(TYindex<Ti, idx>) {
          std::byte *dst = res + Traits::offsets[idx];
          std::memcpy(dst, &std::get<idx>(s)[index], sizeof(Ti));
        });

    return *reinterpret_cast<T *>(res);
  }
};

struct A {
  int a;
  int b;
  int c;
};

template <> struct SOA_Traits<A> {
  using types = ListOfTypes<int, int, int>;
  static constexpr std::array offsets{
      offsetof(A, a),
      offsetof(A, b),
      offsetof(A, c),
  };
};

template <class T> void blackbox(T &t) {
  T *v = &t;
  asm volatile("nop" : "+g"(v));
}

int main(int argc, char *argv[]) {
  SOA<A> soa;
  size_t idx = soa.insert({1, 2, 3});
  blackbox(soa);
  auto b = soa.get(idx);
  assert(b.a == 1);
  assert(b.b == 2);
  assert(b.c == 3);
  return 0;
}
