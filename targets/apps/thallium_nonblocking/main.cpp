#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <cxxopts.hpp>
#include <hello_thallium.hpp>
#include <iostream>
#include <ostream>
#include <thallium.hpp>
#include <thallium/provider.hpp>
#include <thallium/request.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thread>
#include "hello_thallium/point.hpp"
#include "hello_thallium/unused.hpp"
#include "hello_thallium/utils.hpp"

namespace tl = thallium;
namespace ht = hello_thallium;

auto main(int argc, char** argv) -> int {
  cxxopts::Options options("hello_thallium_server", "Hello Thallium Server");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("a,addr", "Address", cxxopts::value<std::string>()->default_value("tcp://127.0.0.1:1234"))
    ("S,server", "Server", cxxopts::value<bool>()->default_value("false"))
    ("s,size", "Payload size(byte)", cxxopts::value<size_t>()->default_value("1"))
  ;
  // clang-format on

  auto result = options.parse(argc, argv);
  if (result.count("help") != 0U) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  if (result["server"].as<bool>()) {
    // server
    tl::engine my_engine(result["addr"].as<std::string>(), THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << my_engine.self() << std::endl;
    my_engine.define("sum", [](const tl::request& req, int x, int y) {
      fmt::print("Computing {} + {}\n", x, y);
      std::flush(std::cout);
      req.respond(x + y);
    });

  } else {
    // client
    tl::engine my_engine(ht::protocol(result["addr"].as<std::string>()),
                        THALLIUM_CLIENT_MODE);
    tl::remote_procedure sum = my_engine.define("sum");
    tl::endpoint server = my_engine.lookup(result["addr"].as<std::string>());
    auto request = sum.on(server).async(42, 63); //NOLINT
    // do something else ...
    // check if request completed
    bool completed = request.received();
    UNUSED(completed);
    // ...
    // actually wait on the request and get the result out of it
    int ret = request.wait();
    std::cout << "Server answered " << ret << std::endl;
  }

  return 0;
}
