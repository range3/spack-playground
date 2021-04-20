#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <random>
#include <string>

namespace grs {

template <typename T = std::mt19937>
inline auto randomGenerator() -> T {
  auto constexpr seedBytes = sizeof(typename T::result_type) * T::state_size;
  auto constexpr seedLen = seedBytes / sizeof(std::seed_seq::result_type);
  auto seed = std::array<std::seed_seq::result_type, seedLen>();
  auto dev = std::random_device();
  std::generate_n(begin(seed), seedLen, std::ref(dev));
  auto seedSeq = std::seed_seq(begin(seed), end(seed));
  return T{seedSeq};
}

inline auto generateRandomAlphanumericString(std::size_t len) -> std::string {
  static constexpr auto chars =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  thread_local auto rng = randomGenerator<>();
  auto dist = std::uniform_int_distribution{{}, std::strlen(chars) - 1};
  auto result = std::string(len, '\0');
  std::generate_n(begin(result), len, [&]() { return chars[dist(rng)]; });
  return result;
}

}  // namespace grs
