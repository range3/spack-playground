#pragma once

#include <condition_variable>
#include <mutex>

namespace sync {
class barrier {
  size_t nthreads_;
  std::mutex mtx_;
  std::condition_variable cond_;

 public:
  barrier(size_t nthreads) : nthreads_(nthreads) {}

  void enter() {
    {
      std::unique_lock<std::mutex> lk(mtx_);
      nthreads_ -= 1;
      cond_.notify_all();
      cond_.wait(lk, [&] { return nthreads_ == 0; });
    }
  }
};
}  // namespace sync
