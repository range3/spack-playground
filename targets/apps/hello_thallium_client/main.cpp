#include <chrono>
#include <cxxopts.hpp>
#include <hello_thallium.hpp>
#include <thallium.hpp>
#include <thallium/engine.hpp>
#include <thallium/remote_procedure.hpp>
#include <thallium/serialization/stl/string.hpp>
#include "hello_thallium/point.hpp"

namespace tl = thallium;
namespace ht = hello_thallium;

auto main(int argc, char** argv) -> int {
  cxxopts::Options options("hello_thallium_client", "Hello Thallium Client");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("p,provider", "Provider", cxxopts::value<std::string>()->default_value("sockets"))
    ("a,addr", "Address", cxxopts::value<std::string>())
  ;
  // clang-format on

  auto result = options.parse(argc, argv);
  if (result.count("help") != 0U) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  tl::engine myEngine(result["provider"].as<std::string>(),
                      THALLIUM_CLIENT_MODE);
  tl::endpoint server = myEngine.lookup(result["addr"].as<std::string>());
  auto hello = myEngine.define("hello").disable_response();
  hello.on(server)(std::string("Ichiro"));

  tl::remote_procedure sum = myEngine.define("sum");
  int ret = sum.on(server).timed(std::chrono::milliseconds(1), 42, 63);
  std::cout << "Server answered: " << ret << std::endl;

  tl::remote_procedure lambda = myEngine.define("lambda").disable_response();
  lambda.on(server)(777);

  auto userClass = myEngine.define("user_class");
  ht::Point retPoint = userClass.on(server)(ht::Point(10, 20, 30));
  std::cout << "Returned Point class = " << retPoint.x << ":" << retPoint.y
            << ":" << retPoint.z << std::endl;

  tl::remote_procedure shutdown =
      myEngine.define("shutdown").disable_response();
  shutdown.on(server)();

  return 0;
}
