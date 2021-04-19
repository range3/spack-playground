#pragma once

#include <string>

namespace hello_thallium {
inline auto protocol(const std::string& addr) -> std::string {
  return addr.substr(0, addr.find_first_of("://"));
}
}  // namespace hello_thallium
