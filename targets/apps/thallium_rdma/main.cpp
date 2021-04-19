#include <chrono>
#include <cxxopts.hpp>
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
    ("s,server", "Server", cxxopts::value<bool>())
    ("c,client", "Client", cxxopts::value<bool>())
  ;
  // clang-format on
  auto result = options.parse(argc, argv);
  if (result.count("help") != 0U) {
    std::cout << options.help() << std::endl;
    return 0;
  }
  if ((result.count("server") == 0U) && (result.count("client") == 0U)) {
    std::cerr << "Error: need --server or --client option" << std::endl;
    std::cerr << options.help() << std::endl;
    return -1;
  }

  if (result.count("server") != 0U) {
    // server
    tl::engine myEngine(result["addr"].as<std::string>(), THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << myEngine.self() << std::endl;

    std::function<void(const tl::request&, tl::bulk&)> f =
        [&myEngine](const tl::request& req, tl::bulk& b) {
          tl::endpoint ep = req.get_endpoint();
          std::vector<char> v(6);
          std::vector<std::pair<void*, std::size_t>> segments(1);
          segments[0].first = static_cast<void*>(&v[0]);
          segments[0].second = v.size();
          tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);
          b.on(ep) >> local;
          std::cout << "Server received bulk: ";
          for (auto c : v) {
            std::cout << c;
          }
          std::cout << std::endl;
        };
    myEngine.define("do_rdma", f).disable_response();

  } else {
    auto provider = ht::provider(result["addr"].as<std::string>());
    // client
    tl::engine myEngine("tcp", MARGO_CLIENT_MODE);
    tl::remote_procedure remoteDoRdma =
        myEngine.define("do_rdma").disable_response();
    tl::endpoint serverEndpoint =
        myEngine.lookup(result["addr"].as<std::string>());

    std::string buffer = "Matthieu";
    std::vector<std::pair<void*, std::size_t>> segments(1);
    segments[0].first = static_cast<void*>(&buffer[0]);
    segments[0].second = buffer.size() + 1;

    tl::bulk myBulk = myEngine.expose(segments, tl::bulk_mode::read_only);

    remoteDoRdma.on(serverEndpoint)(myBulk);
  }

  return 0;
}
