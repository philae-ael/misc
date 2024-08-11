#ifndef INCLUDE_SORTED_SEQENCES_BST_H_
#define INCLUDE_SORTED_SEQENCES_BST_H_

// Implicit
// Binary search tree

#include <bit>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <format>
#include <optional>
#include <ostream>
#include <print>
#include <type_traits>

namespace detail {
template <class T, class Inner> struct handle_impl {
  enum class handle : Inner;
};
} // namespace detail

template <class T, class Inner = std::size_t>
using handle_t = typename detail::handle_impl<T, Inner>::handle;

#include <vector>

struct entry {
  size_t k, v;
};

class binary_search_tree {
public:
  using T = int;
  using handle_t = handle_t<T>;
  std::vector<T> inner;

  // VECTOR OF BOOL OH MAMA
  std::vector<bool> _exists;

  handle_t _find(T t) const {
    // Here we are 1 indexed!
    size_t i = 1;

    while (true) {
      auto v = get(handle_t{i - 1});
      if (!v) {
        break;
      }

      if (t == *v) {
        return handle_t{i - 1};
      }

      i = 2 * i + (t > *v);
    }

    return handle_t{i - 1};
  }

  std::optional<handle_t> find(T t) {
    const handle_t handle = _find(t);
    if (exist(handle)) {
      return {handle};
    } else {
      return {};
    };
  }

  size_t depth() const { return std::countr_zero(1 + inner.size()); }

  void grow() {
    auto d = depth();
    auto new_size = (1 << (d + 1)) - 1;
    inner.resize(new_size);
    _exists.resize(new_size);
    assert(d + 1 == depth());
  }

  handle_t insert(T t) {
    const handle_t handle = _find(t);
    const auto index = static_cast<size_t>(handle);

    if (index >= inner.size()) {
      grow();
    }

    assert(index < inner.size());
    inner[index] = t;
    _exists[index] = true;
    return handle;
  }

  bool exist(handle_t handle) const {
    const auto index = static_cast<size_t>(handle);
    if (index >= inner.size()) {
      return {};
    }

    return _exists[index];
  }

  std::optional<T> get(handle_t handle) const {
    if (exist(handle)) {
      return inner[static_cast<size_t>(handle)];
    }
    return {};
  }
};

static void test_bst() {
  binary_search_tree bst;
  bst.insert(17);
  bst.insert(10);
  bst.insert(3);
  bst.insert(2);
  bst.insert(5);
  bst.insert(11);
  bst.insert(13);
  bst.insert(19);

  std::print("{{");
  for (auto i : bst.inner) {
    std::print("{: >5}, ", i);
  }
  std::print("}}\n{{");
  for (bool i : bst._exists) {
    std::print("{: >5}, ", i);
  }
  std::print("}}\n");
}

#endif // INCLUDE_SORTED_SEQENCES_BST_H_
