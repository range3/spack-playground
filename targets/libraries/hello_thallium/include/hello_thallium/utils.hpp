#pragma once

#include <string>
#define UNUSED(...) (void)(__VA_ARGS__)

namespace hello_thallium {
inline auto provider(const std::string& addr) -> std::string {
  return addr.substr(0, addr.find_first_of("://"));
}
}  // namespace hello_thallium
