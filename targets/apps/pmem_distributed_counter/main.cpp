#include <fmt/core.h>
#include <fmt/ostream.h>
#include <cxxopts.hpp>
#include <elapsedtime.hpp>
#include <gen_random_string.hpp>
#include <libpmemobj++/detail/enumerable_thread_specific.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <nlohmann/json.hpp>
#include <pretty_bytes/pretty_bytes.hpp>
#include <psync/barrier.hpp>
#include <string>
#include <thread>
#include <vector>
// #include <libpmemobj++/container/concurrent_hash_map.hpp>
// #include <libpmemobj++/experimental/concurrent_map.hpp>

using json = nlohmann::json;

struct TlsEntry {
  pmem::obj::p<size_t> counter;
  std::array<char, 64 - sizeof(decltype(counter))> padding;
};
static_assert(sizeof(TlsEntry) == 64,
              "The size of TlsEntry should be 64 bytes.");
using tls_t = pmem::detail::enumerable_thread_specific<TlsEntry>;
// using hash_map_t = pmem::obj::concurrent_hash_map<int, int>;
// using map_t = pmem::obj::experimental::concurrent_map<int, double>;

struct Root {
  tls_t counters;
};
using pool_t = pmem::obj::pool<Root>;

auto main(int argc, char* argv[]) -> int try {
  cxxopts::Options options("pmem_distributed_counter",
                           "an example of enumerable_thread_specific");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("p,path", "/path/to/file or device", cxxopts::value<std::string>()->default_value("./pmem_dist_counter.pool"))
    ("S,pool_size", "pool size (bytes)", cxxopts::value<std::string>()->default_value("1G"))
    ("n,nthreads", "number of threads", cxxopts::value<size_t>())
  ;
  // clang-format on

  auto parsed = options.parse(argc, argv);
  if (parsed.count("help") != 0U) {
    fmt::print("{}\n", options.help());
    fmt::print("You can use byte suffixes: K|M|G|T|P\n");
    return 0;
  }

  // convert pretty bytes to uint64_t
  auto pool_size =
      pretty_bytes::prettyTo<uint64_t>(parsed["pool_size"].as<std::string>());

  size_t nthreads;
  if (parsed["nthreads"].count() == 0U) {
    nthreads = std::thread::hardware_concurrency();
  } else {
    nthreads = parsed["nthreads"].as<size_t>();
  }

  // create or open a pmemobj pool
  auto const pool_file_path = parsed["path"].as<std::string>();
  static constexpr auto kPoolLayout = "mpsc_queue_bench";
  pool_t pop;

  if (pool_t::check(pool_file_path, kPoolLayout) == 1) {
    pop = pool_t::open(pool_file_path, kPoolLayout);
  } else {
    pop = pool_t::create(pool_file_path, kPoolLayout, pool_size);
  }
  auto r = pop.root();

  fmt::print("print counters stored in the prev run\n");
  for (auto& tls_entry : r->counters) {
    fmt::print("{}\n", tls_entry.counter);
  }

  // clear tls (not thread safe)
  r->counters.clear();

  // create threads
  std::vector<std::thread> workers;

  for (size_t i = 0; i < nthreads; ++i) {
    workers.emplace_back([&, i] {
      fmt::print("thread {}: id: {}\n", i, std::this_thread::get_id());
      auto& tls_entry = r->counters.local();
      for (size_t j = 0; j < 1000; ++j) {
        pmem::obj::transaction::run(pop, [&] { tls_entry.counter++; });
      }
    });
  }

  // join threads
  for (auto& worker : workers) {
    worker.join();
  }

  // print thread local counters
  size_t sum = 0;
  fmt::print("print counters\n");
  for (auto& tls_entry : r->counters) {
    fmt::print("{}\n", tls_entry.counter);
    sum += tls_entry.counter;
  }
  fmt::print("sum: {}\n", sum);

  // pool close
  try {
    pop.close();
  } catch (const std::logic_error& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
} catch (const std::exception& e) {
  fmt::print(stderr, "Exception {}\n", e.what());
  return -1;
}
