#include <cxxopts.hpp>
#include <iostream>
#include <thallium.hpp>

namespace tl = thallium;

auto main(int argc, char** argv) -> int {
  cxxopts::Options options("hello_thallium_server", "Hello Thallium Server");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("a,addr", "Address", cxxopts::value<std::string>()->default_value("na+sm"))
  ;
  // clang-format on
  auto result = options.parse(argc, argv);
  if (result.count("help") != 0U) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  tl::engine myEngine(result["addr"].as<std::string>(), THALLIUM_SERVER_MODE);
  std::cout << "Server running at address " << myEngine.self() << std::endl;

  return 0;
}
