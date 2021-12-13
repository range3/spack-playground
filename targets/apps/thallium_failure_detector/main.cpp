#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <chrono>
#include <cxxopts.hpp>
#include <hello_thallium.hpp>
#include <iostream>
#include <mutex>
#include <ostream>
#include <phi_accrual_fd/failure_detector.hpp>
#include <thallium.hpp>
#include <thallium/provider.hpp>
#include <thallium/request.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thread>

namespace tl = thallium;
namespace ht = hello_thallium;

auto main(int argc, char** argv) -> int {
  cxxopts::Options options("thallium_failure_detector",
                           "Detect failures by Phi accrual failure detector");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("a,addr", "Address", cxxopts::value<std::string>()->default_value("tcp"))
    ("S,server", "Server", cxxopts::value<bool>()->default_value("false"))
    ("times", "Times to send heartbeats", cxxopts::value<size_t>()->default_value("10"))
  ;
  // clang-format on

  auto parsed = options.parse(argc, argv);
  if (parsed.count("help") != 0U) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  // margo log
  // margo_set_global_log_level(MARGO_LOG_TRACE);

  if (parsed["server"].as<bool>()) {
    phi_accrual_fd::FailureDetector failure_detector;
    bool received_first_heartbeat = false;
    std::mutex mtx;

    std::thread print_phi_thread([&] {
      while (true) {
        using namespace std::chrono_literals;
        {
          std::scoped_lock<std::mutex> lock{mtx};
          fmt::print("phi = {}\n", failure_detector.phi());
        }
        std::this_thread::sleep_for(1s);
      }
    });
    print_phi_thread.detach();

    // server
    {
      tl::engine my_engine(parsed["addr"].as<std::string>(),
                           THALLIUM_SERVER_MODE);
      std::cout << "Server running at address " << my_engine.self()
                << std::endl;

      my_engine
          .define("hb",
                  [&](const tl::request&) {
                    std::scoped_lock<std::mutex> lock{mtx};
                    if (!received_first_heartbeat) {
                      received_first_heartbeat = true;
                      failure_detector.reset();
                    } else {
                      failure_detector.heartbeat();
                    }
                    // fmt::print("receive heartbeat\n");
                  })
          .disable_response();
    }

  } else {
    auto protocol = ht::protocol(parsed["addr"].as<std::string>());
    // client
    tl::engine my_engine(protocol, MARGO_CLIENT_MODE);
    tl::remote_procedure remote_hb = my_engine.define("hb").disable_response();
    tl::endpoint server_endpoint =
        my_engine.lookup(parsed["addr"].as<std::string>());

    size_t times = parsed["times"].as<size_t>();
    size_t i;
    for (i = 0; i < times; ++i) {
      using namespace std::chrono_literals;
      remote_hb.on(server_endpoint)();
      std::this_thread::sleep_for(100ms);
    }
  }

  return 0;
}
