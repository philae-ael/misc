// requires that data
#include <array>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// Macro magic  start
#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define PARENS ()
#define FOR_EACH2(macro, ...)                                                  \
  __VA_OPT__(EXPAND(FOR_EACH2_HELPER(macro, __VA_ARGS__)))
#define FOR_EACH2_HELPER(macro, a1, a2, ...)                                   \
  macro(a1, a2) __VA_OPT__(FOR_EACH2_AGAIN PARENS(macro, __VA_ARGS__))
#define FOR_EACH2_AGAIN() FOR_EACH2_HELPER
// Macro magic  end

#define _gen_field(type, name) type name;
#define _gen_fields(...) FOR_EACH2(_gen_field, __VA_ARGS__)

#define _gen_SOA_struct(name, ...)                                             \
  struct name {                                                                \
    _gen_fields(__VA_ARGS__)                                                   \
  };

#define _gen_type(type, name) , type
#define _gen_lot(type, name, ...)                                              \
  type __VA_OPT__(FOR_EACH2(_gen_type, __VA_ARGS__))

#define _gen_offset(type, name) offsetof(T, name),
#define _gen_offsets(...) FOR_EACH2(_gen_offset, __VA_ARGS__)
#define _gen_SOA_traits(name, ...)                                             \
  template <> struct SOA_Traits<name> {                                        \
    using T = name;                                                            \
    using types = ListOfTypes<_gen_lot(__VA_ARGS__)>;                          \
    static constexpr std::array offsets{_gen_offsets(__VA_ARGS__)};            \
  };

#define SOA_Type(name, ...)                                                    \
  _gen_SOA_struct(name, __VA_ARGS__) _gen_SOA_traits(name, __VA_ARGS__)

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

template <class T>
  requires std::is_trivial_v<T>
class SOA {
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

SOA_Type(A, int, a, int, b, int, c);

template <class T> void blackbox(T &t) {
  T *v = &t;
  asm volatile("" : "+g"(v));
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
