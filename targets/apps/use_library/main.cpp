#include <iostream>

using namespace std;

#include <hellolib/hellolib.hpp>

auto main() -> int {
  cout << "Hello World" << endl;
  cout << hellolib::say() << endl;
  return 0;
}
