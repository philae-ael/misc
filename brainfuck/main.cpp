#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <optional>
#include <string_view>
#include <sys/types.h>
#include <vector>

enum class Op {
  IncPtr,
  DecPtr,
  IncData,
  DecData,
  PrintData,
};

template <class... F> struct visitor : F... {
  using F::operator()...;
};

template <const char c> struct constchar {};

namespace bf {

struct string_view {
  const char *_data;
  size_t _size;

  constexpr string_view() : _data(nullptr), _size(0) {}
  constexpr string_view(const char *data, size_t size)
      : _data(data), _size(size) {}

  template <const size_t N>
  constexpr string_view(const char (&data)[N]) : _data(data), _size(N - 1) {}
  constexpr string_view(std::string_view sv)
      : _data(sv.data()), _size(sv.size()) {}

  constexpr char operator[](size_t index) const { return _data[0]; }

  constexpr string_view substr(size_t index) const {
    return {_data + index, _size - index};
  }

  constexpr size_t size() const { return _size; }
  constexpr const char *data() const { return _data; }
};

constexpr const char hello_world[]{
    "++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<++++++"
    "+++++++++.>.+++.------.--------.>+.>."};
} // namespace bf

constexpr auto parse_once(bf::string_view sv)
    -> std::pair<std::optional<Op>, bf::string_view> {
  using enum Op;
  if (sv.size() == 0)
    return {std::nullopt, sv};

  return {[sv] -> std::optional<Op> {
            switch (sv[0]) {
            case '+':
              return IncData;
            case '-':
              return DecData;
            case '<':
              return IncPtr;
            case '>':
              return DecPtr;
            case '.':
              return PrintData;
            default:
              return std::nullopt;
            }
          }(),
          sv.substr(1)};
}

consteval void run(bf::string_view sv) {
  std::optional<Op> op;
  for (;;) {
    std::tie(op, sv) = parse_once(sv);

    if (!op.has_value()) {
      break;
    }
  }
}

int main(int argc, char *argv[]) { return 0; }
