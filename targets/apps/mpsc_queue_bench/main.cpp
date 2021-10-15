#include <fmt/core.h>
#include <cxxopts.hpp>
#include <elapsedtime.hpp>
#include <gen_random_string.hpp>
#include <libpmemobj++/experimental/mpsc_queue.hpp>
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

using json = nlohmann::json;

struct Root {
  pmem::obj::persistent_ptr<pmem::obj::experimental::mpsc_queue::pmem_log_type>
      log;
};
using pool_t = pmem::obj::pool<Root>;

void singleThreaded(pool_t pop) {
  std::vector<std::string> values_to_produce = {"xxx", "aaaaaaa", "bbbbb",
                                                "cccc", "ddddddddddd"};
  pmem::obj::persistent_ptr<Root> proot = pop.root();

  /* Create mpsc_queue, which uses pmem_log_type object to store
   * data. */
  auto queue = pmem::obj::experimental::mpsc_queue(*proot->log, 1);

  /* Consume data, which was stored in the queue in the previous run of
   * the application. */
  queue.try_consume_batch(
      [&](pmem::obj::experimental::mpsc_queue::batch_type rd_acc) {
        for (pmem::obj::string_view str : rd_acc) {
          std::cout << std::string(str.data(), str.size()) << std::endl;
        }
      });
  /* Produce and consume data. */
  pmem::obj::experimental::mpsc_queue::worker worker = queue.register_worker();
  for (std::string& value : values_to_produce) {
    /* Produce data. */
    worker.try_produce(value);
    /* Consume produced data. */
    queue.try_consume_batch(
        [&](pmem::obj::experimental::mpsc_queue::batch_type rd_acc) {
          for (pmem::obj::string_view str : rd_acc) {
            std::cout << std::string(str.data(), str.size()) << std::endl;
          }
        });
  }
  /* Produce data to be consumed in next run of the application. */
  worker.try_produce("Left for next run");
}

auto main(int argc, char* argv[]) -> int try {
  cxxopts::Options options("mpsc_queue_bench",
                           "Benchmarking pmem::obj::experimental::mpsc_queue");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("p,path", "/path/to/file or device", cxxopts::value<std::string>()->default_value("./mpsc_queue_bench.pool"))
    ("S,pool_size", "pool size (bytes)", cxxopts::value<std::string>()->default_value("1G"))
    ("q,queue_size", "queue size (bytes)", cxxopts::value<std::string>()->default_value("1K"))
    ("n,nthreads", "number of threads", cxxopts::value<std::string>()->default_value("1"))
    ("read", "read")
    ("write", "write")
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
  auto queue_size =
      pretty_bytes::prettyTo<uint64_t>(parsed["queue_size"].as<std::string>());

  // create or open a pmemobj pool
  auto const pool_file_path = parsed["path"].as<std::string>();
  static constexpr auto kPoolLayout = "mpsc_queue_bench";
  pool_t pop;

  if (pool_t::check(pool_file_path, kPoolLayout) == 1) {
    pop = pool_t::open(pool_file_path, kPoolLayout);
  } else {
    pop = pool_t::create(pool_file_path, kPoolLayout, pool_size);
  }

  // init mpsc_queue
  if (pop.root()->log == nullptr) {
    pmem::obj::transaction::run(pop, [&] {
      pop.root()->log = pmem::obj::make_persistent<
          pmem::obj::experimental::mpsc_queue::pmem_log_type>(queue_size);
    });
  }

  singleThreaded(pop);

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
