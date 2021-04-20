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
class MySumProvider : public tl::provider<MySumProvider> {
 private:
  void prod(const tl::request& req, int x, int y) {
    std::cout << "Computing " << x << "*" << y << std::endl;
    req.respond(x + y);
  }

  int sum(int x, int y) {
    std::cout << "Computing " << x << "+" << y << std::endl;
    return x + y;
  }

  void hello(const std::string& name) {
    std::cout << "Hello, " << name << std::endl;
  }

  size_t print(const std::string& word) {
    std::cout << "Printing " << word << std::endl;
    return word.size();
  }

 public:
  MySumProvider(tl::engine& e, uint16_t providerId = 1)
      : tl::provider<MySumProvider>(e, providerId) {
    define("prod", &MySumProvider::prod);
    define("sum", &MySumProvider::sum);
    define("hello", &MySumProvider::hello);
    define("print", &MySumProvider::print, tl::ignore_return_value());
  }

  ~MySumProvider() override { get_engine().wait_for_finalize(); }
};

auto main(int argc, char** argv) -> int {
  cxxopts::Options options("hello_thallium_server", "Hello Thallium Server");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("a,addr", "Address", cxxopts::value<std::string>()->default_value("tcp"))
    ("s,server", "Server", cxxopts::value<bool>())
    ("c,client", "Client", cxxopts::value<bool>())
    ("p,provider", "Provider ID", cxxopts::value<uint16_t>()->default_value("1"))
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
    auto providerId = result["provider"].as<uint16_t>();

    tl::engine myEngine(result["addr"].as<std::string>(), THALLIUM_SERVER_MODE);
    std::cout << "Server running at address " << myEngine.self()
              << " with provider id " << providerId << std::endl;
    MySumProvider myProvider(myEngine, providerId);

  } else {
    auto protocol = ht::protocol(result["addr"].as<std::string>());
    auto providerId = result["provider"].as<uint16_t>();

    // client
    tl::engine myEngine(protocol, THALLIUM_CLIENT_MODE);
    tl::remote_procedure sum = myEngine.define("sum");
    tl::remote_procedure prod = myEngine.define("prod");
    tl::remote_procedure hello = myEngine.define("hello").disable_response();
    tl::remote_procedure print = myEngine.define("print").disable_response();
    tl::endpoint serverEndpoint =
        myEngine.lookup(result["addr"].as<std::string>());
    tl::provider_handle ph(serverEndpoint, providerId);
    int ret = sum.on(ph)(42, 63);
    std::cout << "(sum) Server answered " << ret << std::endl;
    ret = prod.on(ph)(42, 63);
    std::cout << "(prod) Server answered " << ret << std::endl;
    std::string name("Matthieu");
    hello.on(ph)(name);
    print.on(ph)(name);
  }

  return 0;
}
