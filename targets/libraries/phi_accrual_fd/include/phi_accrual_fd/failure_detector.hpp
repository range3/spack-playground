#pragma once

#include <chrono>
#include <cmath>
#include "window.hpp"

namespace phi_accrual_fd {
class FailureDetector {
 public:
  using clock_t = Window::clock_t;
  using time_point_t = clock_t::time_point;
  using duration_t = Window::duration_t;

  void reset(time_point_t t = clock_t::now()) {
    window_.reset();
    latest_heartbeat_ = t;
  }

  void heartbeat(time_point_t t = clock_t::now()) {
    window_.add(t - latest_heartbeat_);
    latest_heartbeat_ = t;
  }

  // From Akka's phi function.
  // https://github.com/akka/akka/blob/031886a7b32530228f34176cd41bba9d344f43bd/akka-remote/src/main/scala/akka/remote/PhiAccrualFailureDetector.scala#L198
  auto phi(time_point_t t = clock_t::now()) -> double {
    duration_t time_since_last_heartbeat = t - latest_heartbeat_;
    auto normal_distribution = window_.normalDistribution();
    auto sigma = normal_distribution.stddev();
    // any small sigma rounds up to 1ms
    if (sigma < 1.) {
      sigma = 1.;
    }
    // convert the given probability variable to a variable that follows a
    // standard normal distribution
    double y =
        (time_since_last_heartbeat.count() - normal_distribution.mean()) /
        sigma;
    // a part of the cumulative distribution function for standard normal
    // distribution using Logistic approximation
    double e = std::exp(-y * (1.5976 + 0.070566 * y * y));
    if (time_since_last_heartbeat.count() < normal_distribution.mean()) {
      // to avoid the loss of floating point precision
      // when y is a large enough negative value, then e becomes Infinite.
      return -std::log10(1. - 1. / (1. + e));
    } else {  // NOLINT
      // to avoid the loss of floating point precision
      // when e is very small (< 1e-15) then (1. + e) becomes 1.
      return -std::log10(e / (1. + e));
    }
  }

 private:
  time_point_t latest_heartbeat_ = clock_t::now();
  Window window_;
};
}  // namespace phi_accrual_fd
