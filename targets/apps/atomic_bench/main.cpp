#include <fmt/core.h>
#include <fmt/ostream.h>
#include <sys/types.h>
#include <atomic>
#include <cstring>
#include <cxxopts.hpp>
#include <elapsedtime.hpp>
#include <functional>
#include <gen_random_string.hpp>
#include <iomanip>
#include <mutex>
#include <new>
#include <nlohmann/json.hpp>
#include <pretty_bytes/pretty_bytes.hpp>
#include <psync/barrier.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>
#include <exception>

using json = nlohmann::json;

auto main(int argc, char* argv[]) -> int try {
  cxxopts::Options options("atomic_bench", "benchmark std::atomic<T>");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("n,nthreads", "number of threads", cxxopts::value<std::string>()->default_value("1"))
    ("c,count", "total count", cxxopts::value<std::string>()->default_value("1K"))
    ("prettify", "prettify JSON outputs")
  ;
  // clang-format on

  auto parsed = options.parse(argc, argv);
  if (parsed.count("help") != 0U) {
    fmt::print("{}\n", options.help());
    fmt::print("You can use byte suffixes: K|M|G|T|P\n");
    return 0;
  }

  // convert pretty bytes to uint64_t
  auto nthreads =
      pretty_bytes::prettyTo<uint64_t>(parsed["nthreads"].as<std::string>());
  auto total_count =
      pretty_bytes::prettyTo<uint64_t>(parsed["count"].as<std::string>());

  // check arguments
  if (total_count % nthreads != 0U) {
    fmt::print(stderr, "Error: Invalid arguments\n", options.help());
    exit(1);
  }

  // clang-format off
  json benchmark_result = {
    {"params", {
      {"nthreads", nthreads},
      {"totalCount", total_count},
    }},
    {"results", {
      {"success", false},
      {"time", 0.0},
      {"throughput", 0.0},
      {"iops", 0.0},
    }}
  };
  // clang-format on

  // create or open a pmembb pool
  auto count_per_thread = total_count / nthreads;
  std::atomic<uint64_t> counter = 0;

  std::vector<std::thread> workers;
  psync::Barrier wait_for_ready(nthreads + 1);
  psync::Barrier wait_for_timer(nthreads + 1);
  psync::Barrier wait_for_finish(nthreads + 1);

  for (size_t i = 0; i < nthreads; ++i) {
    workers.emplace_back([&] {
      wait_for_ready.enter();
      wait_for_timer.enter();
      for (size_t j = 0; j < count_per_thread; ++j) {
        counter++;
      }
      wait_for_finish.enter();
    });
  }

  wait_for_ready.enter();
  et::ElapsedTime elapsed_time;
  wait_for_timer.enter();
  wait_for_finish.enter();
  elapsed_time.freeze();

  for (auto& worker : workers) {
    worker.join();
  }

  if(counter != total_count) {
    throw std::logic_error(fmt::format("expect: {}. actual {}", total_count, counter));
  }

  benchmark_result["results"]["success"] = true;
  benchmark_result["results"]["time"] = elapsed_time.sec();
  benchmark_result["results"]["throughput"] =
      static_cast<double>(total_count * sizeof(uint64_t)) / elapsed_time.sec();
  benchmark_result["results"]["iops"] =
      static_cast<double>(total_count) / elapsed_time.sec();
  if (parsed.count("prettify") != 0U) {
    std::cout << std::setw(2);
  }
  std::cout << benchmark_result << std::endl;

  return 0;
} catch (const std::exception& e) {
  fmt::print(stderr, "Exception {}\n", e.what());
  return -1;
}
