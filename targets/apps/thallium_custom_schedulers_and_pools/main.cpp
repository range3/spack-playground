#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <cxxopts.hpp>
#include <deque>
#include <hello_thallium.hpp>
#include <iostream>
#include <mutex>  // to use std::lock_guard
#include <ostream>
#include <thallium.hpp>
#include <thallium/provider.hpp>
#include <thallium/request.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thread>
#include <utility>
#include "hello_thallium/utils.hpp"

// https://mochi.readthedocs.io/en/latest/thallium/13_abt_custom.html
// compile error

namespace tl = thallium;
namespace ht = hello_thallium;

constexpr const size_t NUM_XSTREAMS = 1;
constexpr const size_t NUM_THREADS = 16;

class MyUnit;
class MyPool;
class MySchedular;

class MyUnit {
  tl::thread thread_;
  tl::task task_;
  tl::unit_type type_;
  bool inPool_;

  friend class MyPool;

 public:
  MyUnit(tl::thread t)
      : thread_(std::move(t)), type_(tl::unit_type::thread), inPool_(false) {}

  MyUnit(tl::task t)
      : task_(std::move(t)), type_(tl::unit_type::task), inPool_(false) {}

  auto getType() const -> tl::unit_type { return type_; }
  auto getThread() const -> const tl::thread& { return thread_; }
  auto getTask() const -> const tl::task& { return task_; }
  auto isInPool() const -> bool { return inPool_; }

  ~MyUnit() = default;
};

class MyPool {
  mutable tl::mutex mutex_;
  std::deque<MyUnit*> units_;

 public:
  static const tl::pool::access ACCESS_TYPE = tl::pool::access::mpmc;

  MyPool() = default;

  auto getSize() const -> size_t {
    std::lock_guard<tl::mutex> lock(mutex_);
    return units_.size();
  }

  void push(MyUnit* u) {
    std::lock_guard<tl::mutex> lock(mutex_);
    u->inPool_ = true;
    units_.push_back(u);
  }

  auto pop() -> MyUnit* {
    std::lock_guard<tl::mutex> lock(mutex_);
    if (units_.empty()) {
      return nullptr;
    }
    MyUnit* u = units_.front();
    units_.pop_front();
    u->inPool_ = false;
    return u;
  }

  void remove(MyUnit* u) {
    std::lock_guard<tl::mutex> lock(mutex_);
    auto it = std::find(units_.begin(), units_.end(), u);
    if (it != units_.end()) {
      (*it)->inPool_ = false;
      units_.erase(it);
    }
  }

  ~MyPool() = default;
};

class MyScheduler : private tl::scheduler {
 public:
  template <typename... Args>
  MyScheduler(Args&&... args) : tl::scheduler(std::forward<Args>(args)...) {}

  void run() {
    size_t n = num_pools();
    MyUnit* unit;
    int target;
    unsigned seed = time(NULL);

    while (true) {
      /* Execute one work unit from the scheduler's pool 0 */
      unit = get_pool(0).pop<MyUnit>();
      if (unit != nullptr) {
        get_pool(0).run_unit(unit);
      } else if (n > 1) {
        /* Steal a work unit from other pools */
        target = (n == 2) ? 1 : (rand_r(&seed) % (n - 1) + 1);
        unit = get_pool(target).pop<MyUnit>();
        if (unit != nullptr) {
          get_pool(target).run_unit(unit);
        }
      }

      if (has_to_stop()) {
        break;
      }

      tl::xstream::check_events(*this);
    }
  }

  auto getMigrPool() const -> tl::pool { return get_pool(0); }

  ~MyScheduler() = default;
};

void hello() {
  tl::xstream es = tl::xstream::self();
  std::cout << "Hello World from ES " << es.get_rank() << ", ULT "
            << tl::thread::self_id() << std::endl;
}

auto main(int argc, char** argv) -> int {
  cxxopts::Options options("main", "example");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("a,addr", "Address", cxxopts::value<std::string>()->default_value("tcp"))
    ("S,server", "Server", cxxopts::value<bool>()->default_value("false"))
  ;
  // clang-format on

  auto result = options.parse(argc, argv);
  if (result.count("help") != 0U) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  tl::abt scope;

  // create pools
  std::vector<tl::managed<tl::pool>> pools;
  for (size_t i = 0; i < NUM_XSTREAMS; i++) {
    pools.push_back(tl::pool::create<MyPool, MyUnit>());
  }

  // create schedulers
  std::vector<tl::managed<tl::scheduler>> scheds;
  for (size_t i = 0; i < NUM_XSTREAMS; i++) {
    std::vector<tl::pool> poolsForSchedI;
    for (size_t j = 0; j < pools.size(); j++) {
      poolsForSchedI.push_back(*pools[j + i % pools.size()]);
    }
    scheds.push_back(tl::scheduler::create<MyScheduler>(poolsForSchedI.begin(),
                                                        poolsForSchedI.end()));
  }

  std::vector<tl::managed<tl::xstream>> ess;

  for (size_t i = 0; i < NUM_XSTREAMS; i++) {
    tl::managed<tl::xstream> es = tl::xstream::create(*scheds[i]);
    ess.push_back(std::move(es));
  }

  std::vector<tl::managed<tl::thread>> ths;
  for (size_t i = 0; i < NUM_THREADS; i++) {
    tl::managed<tl::thread> th =
        ess[i % ess.size()]->make_thread([]() { hello(); });
    ths.push_back(std::move(th));
  }

  for (auto& mth : ths) {
    mth->join();
  }

  for (size_t i = 0; i < NUM_XSTREAMS; i++) {
    ess[i]->join();
  }

  return 0;
}
