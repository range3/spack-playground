#pragma once

#include <iostream>
#include <string>

namespace hello_header_only {

inline void doubleHello(const std::string& hello = "hello") {
  std::cout << hello << " " << hello << std::endl;
}

}  // namespace hello_header_only
