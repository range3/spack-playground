#include <cxxopts.hpp>
#include <thallium.hpp>

namespace tl = thallium;

auto main(int argc, char** argv) -> int {
  cxxopts::Options options("hello_thallium_client", "Hello Thallium Client");
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

  tl::engine myEngine(result["addr"].as<std::string>(), THALLIUM_CLIENT_MODE);

  return 0;
}
