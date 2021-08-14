#pragma once

#include <chrono>
#include <cmath>
#include <list>
#include <random>

namespace phi_accrual_fd {
class Window {
 public:
  using clock_t = std::chrono::high_resolution_clock;
  using duration_t = std::chrono::duration<double, std::milli>;

  Window() = default;
  explicit Window(size_t size) : window_size_(size) {}

  void reset() {
    durations_.clear();
    sum_ = duration_t{0};
    sum_of_squares_ = duration_t{0};
  }

  void add(duration_t d) {
    durations_.push_back(d);
    sum_ += durations_.back();
    sum_of_squares_ += this->pow2(durations_.back());
    if (durations_.size() > window_size_) {
      sum_ -= durations_.front();
      sum_of_squares_ -= this->pow2(durations_.front());
      durations_.pop_front();
    }
  }

  auto mean() const -> duration_t { return sum_ / durations_.size(); }

  auto variance() const -> duration_t {
    return sum_of_squares_ / durations_.size() - this->pow2(this->mean());
  }

  auto normalDistribution() const -> std::normal_distribution<> {
    auto mean = this->mean();
    return std::normal_distribution<>{
        mean.count(),
        std::sqrt(
            (sum_of_squares_ / durations_.size() - this->pow2(mean)).count())};
  }

 private:
  auto pow2(const duration_t& d) const -> duration_t { return d * d.count(); }

  size_t window_size_ = 1000;
  std::list<duration_t> durations_;
  duration_t sum_;
  duration_t sum_of_squares_;
};
}  // namespace phi_accrual_fd
