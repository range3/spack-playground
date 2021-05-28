#include <chrono>
#include <cxxopts.hpp>
#include <hello_thallium.hpp>
#include <iostream>
#include <thallium.hpp>
#include <thallium/request.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thread>
#include "hello_thallium/point.hpp"
#include "hello_thallium/utils.hpp"

namespace tl = thallium;
namespace ht = hello_thallium;

void hello(const tl::request& req) {
  UNUSED(req);
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

  tl::engine my_engine(result["addr"].as<std::string>(), THALLIUM_SERVER_MODE);
  std::cout << "Server running at address " << my_engine.self() << std::endl;

  my_engine
      .define("hello",
              [](const tl::request& req, const std::string& name) {
                UNUSED(req);
                std::cout << "Hello " << name << std::endl;
              })
      .disable_response();

  std::function<void(const tl::request&, int, int)> sum =
      [](const tl::request& req, int x, int y) {
        std::cout << "Computing " << x << "+" << y << std::endl;
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        req.respond(x + y);
      };

  my_engine.define("sum", sum);

  my_engine
      .define("lambda",
              [](const tl::request& req, int x) {
                UNUSED(req);
                std::cout << "Lambda test" << x << std::endl;
              })
      .disable_response();

  my_engine.define("user_class",
                  [](const tl::request& req, const ht::Point& point) {
                    std::cout << "Point class" << point.x << ":" << point.y
                              << ":" << point.z << std::endl;
                    req.respond(point);
                  });

  my_engine
      .define("shutdown",
              [&my_engine](const tl::request& req) {
                UNUSED(req);
                std::cout << "server shutdown" << std::endl;
                my_engine.finalize();
              })
      .disable_response();

  return 0;
}
