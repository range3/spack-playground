#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <chrono>
#include <cxxopts.hpp>
#include <hello_thallium.hpp>
#include <iostream>
#include <ostream>
#include <thallium.hpp>
#include <thallium/provider.hpp>
#include <thallium/request.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thread>
#include "hello_thallium/utils.hpp"

namespace tl = thallium;
namespace ht = hello_thallium;

auto main(int argc, char** argv) -> int {
  cxxopts::Options options("thallium_with_argobot_pools",
                           "Argobot pools with thallium RPC");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("a,addr", "Address", cxxopts::value<std::string>()->default_value("tcp"))
    ("S,server", "Server", cxxopts::value<bool>()->default_value("false"))
  ;
  // clang-format on

  auto result = options.parse(argc, argv);
  if (result.count("help") != 0U) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  if (result["server"].as<bool>()) {
    // server
    // https://mochi.readthedocs.io/en/latest/thallium/12_rpc_pool.html
    // > This feature requires to provide a non-zero provider id (passed to the
    // > define call) when defining the RPC (here 1). Hence you also need to use
    // > provider handles on clients, even if you do not define a provider
    // > class.

    tl::abt scope;
    tl::engine myEngine(result["addr"].as<std::string>(), THALLIUM_SERVER_MODE);
    std::vector<tl::managed<tl::xstream>> ess;
    tl::managed<tl::pool> myPool = tl::pool::create(tl::pool::access::spmc);
    for (size_t i = 0; i < 4; i++) {
      tl::managed<tl::xstream> es =
          tl::xstream::create(tl::scheduler::predef::deflt, *myPool);
      ess.push_back(std::move(es));
    }

    std::cout << "Server running at address " << myEngine.self() << std::endl;

    myEngine.define(
        "sum",
        [](const tl::request& req, int x, int y) {
          fmt::print("Computing {} + {}\n", x, y);
          std::flush(std::cout);
          req.respond(x + y);
        },
        0, *myPool);  // provider_id == 0 ? but it works.

    myEngine.wait_for_finalize();

    for (size_t i = 0; i < 4; i++) {
      ess[i]->join();
    }

  } else {
    const auto protocol = ht::protocol(result["addr"].as<std::string>());
    tl::engine myEngine(protocol, THALLIUM_CLIENT_MODE);
    tl::remote_procedure sum = myEngine.define("sum");
    tl::endpoint server = myEngine.lookup(result["addr"].as<std::string>());
    int ret = sum.on(server)(42, 63);
    std::cout << "Server answered " << ret << std::endl;
  }

  return 0;
}
