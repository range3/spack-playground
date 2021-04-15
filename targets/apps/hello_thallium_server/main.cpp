#include <chrono>
#include <cxxopts.hpp>
#include <iostream>
#include <thallium.hpp>
#include <thallium/request.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thread>

namespace tl = thallium;

void hello(const tl::request& req) {
  std::cout << "Hello World!" << std::endl;
}

auto main(int argc, char** argv) -> int {
  cxxopts::Options options("hello_thallium_server", "Hello Thallium Server");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("a,addr", "Address", cxxopts::value<std::string>()->default_value("sockets"))
  ;
  // clang-format on
  auto result = options.parse(argc, argv);
  if (result.count("help") != 0U) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  tl::engine myEngine(result["addr"].as<std::string>(), THALLIUM_SERVER_MODE);
  std::cout << "Server running at address " << myEngine.self() << std::endl;

  myEngine
      .define("hello",
              [](const tl::request& req, const std::string& name) {
                std::cout << "Hello " << name << std::endl;
              })
      .disable_response();

  std::function<void(const tl::request&, int, int)> sum =
      [](const tl::request& req, int x, int y) {
        std::cout << "Computing " << x << "+" << y << std::endl;
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        req.respond(x + y);
      };

  myEngine.define("sum", sum);

  myEngine
      .define("lambda",
              [](const tl::request& req, int x) {
                std::cout << "Lambda test" << x << std::endl;
              })
      .disable_response();

  myEngine
      .define("shutdown",
              [&myEngine](const tl::request& req) {
                std::cout << "server shutdown" << std::endl;
                myEngine.finalize();
              })
      .disable_response();

  return 0;
}
