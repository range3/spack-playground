#include <fmt/core.h>
#include <libpmemblk.h>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <cxxopts.hpp>
#include <elapsedtime.hpp>
#include <gen_random_string.hpp>
#include <mutex>
#include <pretty_bytes/pretty_bytes.hpp>
#include <random>
#include <sync/barrier.hpp>
#include <thread>
#include <vector>

/* size of the pmemblk pool -- 1 GB */
#define POOL_SIZE ((size_t)(1 << 30))

auto main(int argc, char* argv[]) -> int {
  cxxopts::Options options("pmemblkbench",
                           "Benchmarking persistent memory resident array of "
                           "blocks using libpmemblk");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("p,pool", "/path/to/blk.poll", cxxopts::value<std::string>()->default_value("./pmemblkbench.pool"))
    ("S,pool_size", "pool size (bytes)", cxxopts::value<std::string>()->default_value("2G"))
    ("b,block", "block size (bytes)", cxxopts::value<std::string>()->default_value("512"))
    ("s,stripe", "stripe size (bytes)", cxxopts::value<std::string>()->default_value("512"))
    ("n,nthreads", "number of threads", cxxopts::value<std::string>()->default_value("1"))
    ("t,total", "total array of blocks size (bytes)", cxxopts::value<std::string>()->default_value("1G"))
    ("read", "read")
    ("write", "write")
    ("r,random", "randomize block access in a strip unit.")
  ;
  // clang-format on

  auto result = options.parse(argc, argv);
  if (result.count("help") != 0U) {
    fmt::print("{}\n", options.help());
    fmt::print("You can use byte suffixes: K|M|G|T|P\n");
    return 0;
  }

  // convert pretty bytes to uint64_t
  auto pool_size =
      pretty_bytes::prettyTo<uint64_t>(result["pool_size"].as<std::string>());
  auto block_size =
      pretty_bytes::prettyTo<uint64_t>(result["block"].as<std::string>());
  auto stripe_size =
      pretty_bytes::prettyTo<uint64_t>(result["stripe"].as<std::string>());
  auto nthreads =
      pretty_bytes::prettyTo<uint64_t>(result["nthreads"].as<std::string>());
  auto total_size =
      pretty_bytes::prettyTo<uint64_t>(result["total"].as<std::string>());
  bool op_read = result.count("read") != 0U;
  bool op_write = result.count("write") != 0U;
  bool op_random = result.count("random") != 0U;

  // check arguments
  if (!op_read && !op_write) {
    fmt::print(stderr, "Error: --read or --write required \n");
    return 1;
  }

  if (((total_size % stripe_size) != 0U) || ((total_size % block_size) != 0U) ||
      ((stripe_size % block_size) != 0U) || ((stripe_size % nthreads) != 0U) ||
      stripe_size / nthreads < block_size || op_read == op_write) {
    fmt::print(stderr, "Error: Invalid arguments\n", options.help());
    return 1;
  }
  if (pool_size < PMEMBLK_MIN_POOL) {
    fmt::print(stderr, "Error: The pool size should be greator or equal {}\n",
               PMEMBLK_MIN_POOL);
    return 1;
  }

  // print benchmark info
  fmt::print("block size: {}\n", block_size);
  fmt::print("stripe size: {}\n", stripe_size);
  fmt::print("nthreads: {}\n", nthreads);
  fmt::print("total_size: {}\n", total_size);
  fmt::print("access pattern: {}\n", op_random ? "random" : "sequential");

  PMEMblkpool* pbp;
  size_t nelements;

  /* create the pmemblk pool or open it if it already exists */
  pbp = pmemblk_create(result["pool"].as<std::string>().c_str(), block_size,
                       pool_size, 0666);

  if (pbp == nullptr) {
    std::perror(result["pool"].as<std::string>().c_str());
  }

  if (pbp == nullptr) {
    pbp = pmemblk_open(result["pool"].as<std::string>().c_str(), block_size);
  }

  if (pbp == nullptr) {
    std::perror(result["pool"].as<std::string>().c_str());
    exit(1);
  }

  // how many elements fit into the file?
  nelements = pmemblk_nblock(pbp);

  if (nelements < total_size / block_size) {
    fmt::print(stderr, "Error: The pool size is too small.\n");
    pmemblk_close(pbp);
    exit(1);
  }

  auto random_string_data = grs::generateRandomAlphanumericString(block_size);

  auto stripe_count = total_size / stripe_size;
  auto strip_unit_size = stripe_size / nthreads;
  auto blocks_in_strip_unit = strip_unit_size / block_size;
  auto blocks_in_stripe = stripe_size / block_size;

  std::vector<size_t> access_offset(blocks_in_strip_unit);
  // 0 1 2 3 4 ...
  std::iota(access_offset.begin(), access_offset.end(), 0);
  // randomize offset
  if (op_random) {
    std::shuffle(access_offset.begin(), access_offset.end(),
                 std::mt19937{std::random_device{}()});
  }

  std::vector<std::thread> workers;
  sync::barrier wait_for_ready(nthreads + 1);
  sync::barrier wait_for_timer(nthreads + 1);
  std::atomic<bool> fail(false);

  for (size_t i = 0; i < nthreads; ++i) {
    workers.emplace_back([&, i] {
      std::string buf = random_string_data;
      wait_for_ready.enter();
      wait_for_timer.enter();

      for (size_t stripe = 0; stripe < stripe_count; ++stripe) {
        size_t strip_ofs = stripe * blocks_in_stripe + i * blocks_in_strip_unit;
        for (size_t j = 0; j < blocks_in_strip_unit; ++j) {
          size_t block_ofs = access_offset[j] + strip_ofs;
          if (op_write) {
            if (pmemblk_write(pbp, random_string_data.data(),
                              static_cast<long long>(block_ofs)) < 0) {
              perror("pmemblk_write");
              fail.store(true, std::memory_order_release);
              break;
            }
          } else {
            if (pmemblk_read(pbp, &buf[0], static_cast<long long>(block_ofs)) < 0) {
              perror("pmemblk_read");
              fail.store(true, std::memory_order_release);
              break;
            }
          }
        }
        if (fail.load(std::memory_order_acquire)) {
          break;
        }
      }
    });
  }

  wait_for_ready.enter();
  et::ElapsedTime elapsed_time;
  wait_for_timer.enter();

  for (auto& worker : workers) {
    worker.join();
  }

  elapsed_time.freeze();

  fmt::print("Elapsed Time: {} msec\n", elapsed_time.msec());
  fmt::print("Throuput: {} bytes/sec\n",
             static_cast<double>(total_size) / elapsed_time.sec());

  pmemblk_close(pbp);
}
