#pragma once

namespace hello_thallium {

struct Point {
  double x;
  double y;
  double z;

 public:
  Point(double a = 0.0, double b = 0.0, double c = 0.0) : x(a), y(b), z(c) {}

  template <typename A>
  void serialize(A& ar) {
    ar& x;
    ar& y;
    ar& z;
  }
};

}  // namespace hello_thallium
