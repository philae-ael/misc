#include <cstddef>

// The higher order moments can also be computed this way, but it may be harder, don't know
struct stat_monoid {
  void push(float v) {
    _count += 1;
    _sum += v;
    _square_sum += v * v;
  }
  stat_monoid operator+(const stat_monoid &other) {
    return {
        _count + other._count,
        _sum + other._sum,
        _square_sum + other._square_sum,
    };
  }

  float count() const { return _count; }
  float sum() const { return _sum; }
  float mean() const { return sum() / count(); }
  float squared_sum() const { return _square_sum; }
  float variance() const {
    float m = mean();
    return _square_sum / count() - m * m;
  }

  size_t _count = 0;
  float _sum = 0;
  float _square_sum = 0;
};
