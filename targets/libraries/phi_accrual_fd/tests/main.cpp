#include <chrono>
#include <cmath>
#include <thread>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <fmt/core.h>
#include <limits>

#pragma clang diagnostic ignored "-Wkeyword-macro"
#define private public
#include <phi_accrual_fd/failure_detector.hpp>
#include <phi_accrual_fd/window.hpp>

template <class T>
auto almostEqual(T x, T y) ->
    typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type {
  return std::fabs(x - y) <= std::numeric_limits<T>::epsilon() ||
         std::fabs(x - y) < std::numeric_limits<T>::min();
}

TEST_CASE("Window") {
  using namespace std::chrono_literals;
  SUBCASE("average") {
    phi_accrual_fd::Window window;
    window.add(1s);
    window.add(3s);
    window.add(2s);

    CHECK(almostEqual(window.mean().count(), 2000.));

    fmt::print("window.average() = {}, window.variance() = {}\n",
               window.mean().count(), window.variance().count());
    // fmt::print("{}, {}\n", window.sum_.count(),
    // window.sum_of_squares_.count());
  }

  SUBCASE("overextend") {
    phi_accrual_fd::Window window{3};
    window.add(1s);
    window.add(1s);
    window.add(1s);
    window.add(3s);
    window.add(2s);

    CHECK(almostEqual(window.mean().count(), 2000.));

    fmt::print("window.average() = {}, window.variance() = {}\n",
               window.mean().count(), window.variance().count());
  }
  SUBCASE("normal dist") {
    phi_accrual_fd::Window window;
    window.add(1s);
    window.add(3s);
    window.add(2s);
    auto normal_dist = window.normalDistribution();

    CHECK(almostEqual(normal_dist.mean(), 2000.));
    CHECK(almostEqual(normal_dist.stddev(),
                      std::sqrt(window.variance().count())));
  }
}

TEST_CASE("Detector") {
  using namespace std::chrono_literals;

  SUBCASE("constructor") {
    phi_accrual_fd::FailureDetector detector;
    auto t = decltype(detector)::clock_t::now();
    detector.reset(t);
    t += 1000ms;
    detector.heartbeat(t);
    t += 1100ms;
    detector.heartbeat(t);
    t += 1300ms;
    detector.heartbeat(t);
    t += 1000ms;
    detector.heartbeat(t);

    fmt::print("mean = {}, stddev={}\n", detector.window_.mean().count(),
               detector.window_.normalDistribution().stddev());

    // good phi
    fmt::print("good phi = {}\n", detector.phi(t + 1000ms));

    // bad phi
    fmt::print("bad phi = {}\n", detector.phi(t + 3s));
  }
}
