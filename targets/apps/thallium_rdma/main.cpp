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

  const size_t payload_size = result["size"].as<size_t>();
  if (result["server"].as<bool>()) {
    // server
    tl::engine my_engine(result["addr"].as<std::string>(), THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << my_engine.self() << std::endl;

    std::function<void(const tl::request&, tl::bulk&)> f =
        [payload_size, &my_engine](const tl::request& req, tl::bulk& b) {
          tl::endpoint ep = req.get_endpoint();
          std::vector<char> in(payload_size);
          std::vector<std::pair<void*, std::size_t>> segments_in(1);
          segments_in[0].first = static_cast<void*>(&in[0]);
          segments_in[0].second = in.size();
          et::ElapsedTime t;
          tl::bulk local_write =
              my_engine.expose(segments_in, tl::bulk_mode::write_only);
          b.on(ep) >> local_write;
          t.freeze();

          fmt::print("Received bulk size: {:5} bytes, ", payload_size);
          fmt::print("Elapsed time: {0:.5f} msecs\n", t.msec());
          std::cout << std::flush;
        };
    my_engine.define("do_rdma", f).disable_response();

  } else {
    const std::string payload =
        grs::generateRandomAlphanumericString(result["size"].as<size_t>());
    auto protocol = ht::protocol(result["addr"].as<std::string>());
    // client
    tl::engine my_engine(protocol, MARGO_CLIENT_MODE);
    tl::remote_procedure remote_do_rdma =
        my_engine.define("do_rdma").disable_response();
    tl::endpoint server_endpoint =
        my_engine.lookup(result["addr"].as<std::string>());

    std::vector<std::pair<void*, std::size_t>> segments(1);
    segments[0].first =
        const_cast<void*>(static_cast<const void*>(&payload[0]));
    segments[0].second = payload.size() + 1;

    tl::bulk my_bulk = my_engine.expose(segments, tl::bulk_mode::read_only);

    remote_do_rdma.on(server_endpoint)(my_bulk);
  }

  return 0;
}
