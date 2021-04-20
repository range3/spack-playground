#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <chrono>
#include <cxxopts.hpp>
#include <elapsedtime.hpp>
#include <gen_random_string.hpp>
#include <hello_thallium.hpp>
#include <iostream>
#include <ostream>
#include <thallium.hpp>
#include <thallium/request.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thread>
#include "hello_thallium/point.hpp"
#include "hello_thallium/utils.hpp"

namespace tl = thallium;
namespace ht = hello_thallium;

auto main(int argc, char** argv) -> int {
  cxxopts::Options options("hello_thallium_server", "Hello Thallium Server");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("a,addr", "Address", cxxopts::value<std::string>()->default_value("tcp"))
    ("S,server", "Server", cxxopts::value<bool>()->default_value("false"))
    ("s,size", "Payload size(byte)", cxxopts::value<size_t>()->default_value("1"))
  ;

  // clang-format on
  auto result = options.parse(argc, argv);
  if (result.count("help") != 0U) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  const size_t payloadSize = result["size"].as<size_t>();
  if (result["server"].as<bool>()) {
    // server
    tl::engine myEngine(result["addr"].as<std::string>(), THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << myEngine.self() << std::endl;

    std::function<void(const tl::request&, tl::bulk&)> f =
        [=, &myEngine](const tl::request& req, tl::bulk& b) {
          tl::endpoint ep = req.get_endpoint();
          std::vector<char> in(payloadSize);
          std::vector<std::pair<void*, std::size_t>> segmentsIn(1);
          segmentsIn[0].first = static_cast<void*>(&in[0]);
          segmentsIn[0].second = in.size();
          et::ElapsedTime t;
          tl::bulk localWrite =
              myEngine.expose(segmentsIn, tl::bulk_mode::write_only);
          b.on(ep) >> localWrite;
          t.freeze();

          fmt::print("Received bulk size: {:5} bytes, ", payloadSize);
          fmt::print("Elapsed time: {0:.5f} msecs\n", t.msec());
          std::cout << std::flush;
        };
    myEngine.define("do_rdma", f).disable_response();

  } else {
    const std::string payload =
        grs::generateRandomAlphanumericString(result["size"].as<size_t>());
    auto protocol = ht::protocol(result["addr"].as<std::string>());
    // client
    tl::engine myEngine(protocol, MARGO_CLIENT_MODE);
    tl::remote_procedure remoteDoRdma =
        myEngine.define("do_rdma").disable_response();
    tl::endpoint serverEndpoint =
        myEngine.lookup(result["addr"].as<std::string>());

    std::vector<std::pair<void*, std::size_t>> segments(1);
    segments[0].first =
        const_cast<void*>(static_cast<const void*>(&payload[0]));
    segments[0].second = payload.size() + 1;

    tl::bulk myBulk = myEngine.expose(segments, tl::bulk_mode::read_only);

    remoteDoRdma.on(serverEndpoint)(myBulk);
  }

  return 0;
}
