#include <chrono>
#include <cxxopts.hpp>
#include <hello_thallium.hpp>
#include <iostream>
#include <ostream>
#include <thallium.hpp>
#include <thallium/request.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thread>

namespace tl = thallium;
namespace ht = hello_thallium;
class MySumProvider : public tl::provider<MySumProvider> {
 private:
  tl::remote_procedure prod_;
  tl::remote_procedure sum_;
  tl::remote_procedure hello_;
  tl::remote_procedure print_;

  void prod(const tl::request& req, int x, int y) {
    std::cout << "Computing " << x << "*" << y << std::endl;
    req.respond(x + y);
  }

  auto sum(int x, int y) -> int {
    std::cout << "Computing " << x << "+" << y << std::endl;
    return x + y;
  }

  void hello(const std::string& name) {
    std::cout << "Hello, " << name << std::endl;
  }

  auto print(const std::string& word) -> size_t {
    std::cout << "Printing " << word << std::endl;
    return word.size();
  }

  MySumProvider(tl::engine& e, uint16_t provider_id = 1)
      : tl::provider<MySumProvider>(e, provider_id),
        // keep the RPCs in remote_procedure objects so we can deregister them.
        prod_(define("prod", &MySumProvider::prod)),
        sum_(define("sum", &MySumProvider::sum)),
        hello_(define("hello", &MySumProvider::hello)),
        print_(
            define("print", &MySumProvider::print, tl::ignore_return_value()))

  {
    // setup a finalization callback for this provider, in case it is
    // still alive when the engine is finalized.
    get_engine().push_finalize_callback(this, [p = this]() { delete p; });
  }

 public:
  // this factory method and the private constructor prevent users
  // from putting an instance  of my_sum_provider on  the stack.
  static auto create(tl::engine& e, uint16_t provider_id = 1) -> MySumProvider* {
    return new MySumProvider(e, provider_id);
  }

  ~MySumProvider() override {
    prod_.deregister();
    sum_.deregister();
    hello_.deregister();
    print_.deregister();
    // pop the finalize callback. If this destructor was called
    // from the finalization callback, there is nothing to pop
    get_engine().pop_finalize_callback(this);
  }
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
    auto provider_id = result["provider"].as<uint16_t>();

    tl::engine my_engine(result["addr"].as<std::string>(), THALLIUM_SERVER_MODE);
    my_engine.enable_remote_shutdown();
    std::cout << "Server running at address " << my_engine.self()
              << " with provider id " << provider_id << " and " << provider_id + 1
              << std::endl;
    // MySumProvider myProvider(myEngine, providerId);
    // create a pointer to the provider instance using the factory methods.
    MySumProvider* my_provider1 = MySumProvider::create(my_engine, provider_id);
    MySumProvider* my_provider2 =
        MySumProvider::create(my_engine, provider_id + 1);
    ALL_UNUSED(my_provider1, my_provider2);

    my_engine.wait_for_finalize();
    // the finalization callbacks will ensure that providers are freed.

  } else {
    auto protocol = ht::protocol(result["addr"].as<std::string>());
    auto provider_id = result["provider"].as<uint16_t>();

    // client
    tl::engine my_engine(protocol, THALLIUM_CLIENT_MODE);
    tl::remote_procedure sum = my_engine.define("sum");
    tl::remote_procedure prod = my_engine.define("prod");
    tl::remote_procedure hello = my_engine.define("hello").disable_response();
    tl::remote_procedure print = my_engine.define("print").disable_response();
    tl::endpoint server_endpoint =
        my_engine.lookup(result["addr"].as<std::string>());
    tl::provider_handle ph(server_endpoint, provider_id);
    int ret = sum.on(ph)(42, 63);
    std::cout << "(sum) Server answered " << ret << std::endl;
    ret = prod.on(ph)(42, 63);
    std::cout << "(prod) Server answered " << ret << std::endl;
    std::string name("Matthieu");
    hello.on(ph)(name);
    print.on(ph)(name);
    my_engine.shutdown_remote_engine(ph);
  }

  return 0;
}
