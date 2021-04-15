#include <cxxopts.hpp>
#include <thallium.hpp>
#include <thallium/engine.hpp>

namespace tl = thallium;

auto main(int argc, char** argv) -> int {
  cxxopts::Options options("hello_thallium_client", "Hello Thallium Client");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("t,type", "Address Type", cxxopts::value<std::string>()->default_value("sockets"))
    ("a,addr", "Address", cxxopts::value<std::string>())
  ;
  // clang-format on

  auto result = options.parse(argc, argv);
  if (result.count("help") != 0U) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  tl::engine myEngine(result["type"].as<std::string>(), THALLIUM_CLIENT_MODE);
  tl::remote_procedure hello = myEngine.define("hello").disable_response();
  tl::endpoint server = myEngine.lookup(result["addr"].as<std::string>());
  hello.on(server)();

  return 0;
}
