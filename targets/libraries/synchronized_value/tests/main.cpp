#include <chrono>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <fmt/core.h>
#include <limits>

#define VA_NUM_ARGS_IMPL(_1, _2, _3, _4, _5, N, ...) N
#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(__VA_ARGS__, 5, 4, 3, 2, 1)
#define UNUSED_IMPL_(nargs) UNUSED##nargs
#define UNUSED_IMPL(nargs) UNUSED_IMPL_(nargs)
#define UNUSED(...) UNUSED_IMPL(VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

#pragma clang diagnostic ignored "-Wkeyword-macro"
#define private public
#include <synchronized_value/synchronized_value.hpp>

template <typename T>
using synch = range3::SynchronizedValue<T>;

TEST_CASE("synchronized_value") {
  SUBCASE("constructor") {
    synch<int> c;
    int x = 100;
    int x1 = x;
    synch<int> c1{x};
    synch<int> c2{200};
    synch<int> c3{std::move(x1)};
    synch<int> c4 = c1;
    synch<int> c5{c1};
    synch<int> c6{synch<int>{x}};
    synch<int> c7;
    c7 = c1;
    synch<int> c8;
    c8 = synch<int>{x};
    synch<int> c9;
    c9 = 200;
    synch<int> c10;
    c10 = int{200};

    CHECK(c1.value_ == x);
    CHECK(c2.value_ == 200);
    CHECK(c3.value_ == x);
    CHECK(c4.value_ == x);
    CHECK(c5.value_ == x);
    CHECK(c6.value_ == x);
    CHECK(c7.value_ == x);
    CHECK(c8.value_ == x);
    CHECK(c9.value_ == 200);
    CHECK(c10.value_ == 200);

    synch<std::pair<int, double>> p;
    auto xx = std::make_pair(100, 2.);
    auto xx1 = xx;
    decltype(p) p1{xx};
    decltype(p) p2{std::move(xx)};

    CHECK(p1.value_ == xx1);
    CHECK(p2.value_ == xx1);
  }

  SUBCASE("conversion") {
    synch<int> c{100};
    auto x1 = c.get();
    auto x2 = static_cast<int>(c);
    CHECK(x1 == 100);
    CHECK(x2 == 100);
  }

  SUBCASE("io") {
    synch<int> c{1000};
    std::stringstream ss;
    ss << c;
    CHECK(ss.str() == "1000");
    ss.str("200");
    ss >> c;
    CHECK(c.get() == 200);
  }

  SUBCASE("synchronize") {
    synch<int> c{250};
    {
      decltype(c)::LockGuardPtr ptr{c};

      std::thread{[&]() { CHECK(c.mutex_.try_lock() == false); }}.join();
    }

    {
      // RVO: copy elision, requires C++17
      auto ptr = c.synchronize();
      CHECK(*ptr == 250);
      *ptr = 100;
      CHECK(*ptr == 100);

      std::thread{[&]() { CHECK(c.mutex_.try_lock() == false); }}.join();
    }
  }

  SUBCASE("operator->") {
    synch<std::string> c{"hello"};
    c->append(" world");
    CHECK(*c.synchronize() == "hello world");
  }

  SUBCASE("synchronize multi SynchronizedValue") {
    synch<std::string> sync_src{"hello"};
    synch<std::string> sync_dst{"world"};
    auto proxyes = range3::synchronize(sync_src, sync_dst);
    auto& src = std::get<0>(proxyes);
    auto& dst = std::get<1>(proxyes);

    std::thread{[&]() {
      CHECK(sync_src.mutex_.try_lock() == false);
      CHECK(sync_dst.mutex_.try_lock() == false);
    }}.join();

    CHECK(src.owns_lock());
    CHECK(dst.owns_lock());

    CHECK(*src == "hello");
    CHECK(*dst == "world");
  }

  SUBCASE("condition_variable") {
    synch<int> counter = 0;
    std::condition_variable cond;

    std::thread producer{[&]() {
      size_t i;
      for(i = 0; i < 1000; ++i) {
        {
          auto uniq = counter.uniqueSynchronize();
          *uniq += 1;
        }
        cond.notify_all();
      }
    }};

    {
      auto uniq = counter.uniqueSynchronize();
      cond.wait(uniq, [&]() {
        // fmt::print("{} ", *uniq);
        return *uniq >= 1000;
      });
    }

    producer.join();
  }
}
