
#include <abt.h>
#include <fmt/core.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <range/v3/view.hpp>
#include <unused.hpp>
#include <vector>

using thread_arg_t = struct { size_t tid; };

void helloWorld(void* arg) {
  size_t tid = reinterpret_cast<thread_arg_t*>(arg)->tid;
  int rank;
  ABT_xstream_self_rank(&rank);
  fmt::print("Hello world! (thread = {:d}, ES = {:d})\n", tid, rank);
}

auto main(int argc, char** argv) -> int {
  ABT_init(argc, argv);

  constexpr int kNxstreams = 2;
  constexpr int kNthreads = 8;

  std::vector<ABT_xstream> xstreams{kNxstreams};
  std::vector<ABT_pool> pools{xstreams.size()};
  std::vector<ABT_thread> threads{kNthreads};
  std::vector<thread_arg_t> thread_args{threads.size()};

  size_t i;

  // get primary ES
  ABT_xstream_self(&xstreams[0]);

  // create seconday ESs
  for (auto& xstream : xstreams | ranges::views::drop(1)) {
    ABT_xstream_create(ABT_SCHED_NULL, &xstream);
  }

  // get default pools
  i = 0;
  for (auto& xstream : xstreams) {
    ABT_xstream_get_main_pools(xstream, 1, &pools[i]);
    ++i;
  }

  // create ULTs
  i = 0;
  for (auto& thread : threads) {
    size_t pool_id = i % xstreams.size();
    thread_args[i].tid = i;
    ABT_thread_create(pools[pool_id], helloWorld, &thread_args[i],
                      ABT_THREAD_ATTR_NULL, &thread);
    ++i;
  }

  // Join and free ULTs.
  for (auto& thread : threads) {
    ABT_thread_free(&thread);
  }

  // Join and free secondary execution streams.
  for (auto& xstream : xstreams) {
    ABT_xstream_join(xstream);
    ABT_xstream_free(&xstream);
  }

  ABT_finalize();
  return 0;
}
