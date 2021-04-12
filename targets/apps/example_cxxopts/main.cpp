#include <cxxopts.hpp>
#include <iostream>

auto main(int argc, char** argv) -> int {
  cxxopts::Options options("example_cxxopts", "cxxopts example");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
  ;
  // clang-format on

  auto result = options.parse(argc, argv);
  if (result.count("help") != 0U) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  return 0;
}
