#ifndef INCLUDE_COMMON_H_
#define INCLUDE_COMMON_H_

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <utility>

using usize = std::size_t;

template <class A, class B> struct pair {
  A a;
  B b;
};

template <class T> void swap(T &t1, T &t2) { std::swap(t1, t2); }

template <class T> struct span {
  T *_data;
  std::size_t _size;

  T &operator[](std::size_t index) { return _data[index]; }
  template <const std::size_t N> span(T (&arr)[N]) : _data(arr), _size(N) {}

  span(T *data, std::size_t size) : _data(data), _size(size) {}
  span(span &) = default;
  span &operator=(span &) = default;
  span(span &&) = default;
  span &operator=(span &&) = default;

  T *begin() { return _data; }
  T *end() { return _data + _size; }

  bool empty() { return _size == 0; }
  usize size() { return _size; }
  span slice(std::size_t size, std::size_t offset = 0) {
    assert(size + offset <= this->_size);
    return {_data + offset, size};
  }

  void print() {
    printf("{ ");
    for (auto i : *this) {
      printf("%d, ", i);
    }
    printf("}\n");
  }

  bool operator==(span other) {
    if (_size != other._size) {
      return false;
    }

    for (int i = 0; i < _size; i++) {
      if ((*this)[i] != other[i]) {
        return false;
      }
    }

    return true;
  }
};

#endif // INCLUDE_COMMON_H_
