// (C) Copyright 2010 Just Software Solutions Ltd http://www.justsoftwaresolutions.co.uk
// (C) Copyright 2012 Vicente J. Botet Escriba
// (C) Copyright 2021 range3 ( https://github.com/range3 )
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef RANGE3_SYNCHRONIZED_VALUE_HPP
#define RANGE3_SYNCHRONIZED_VALUE_HPP

#include <mutex>
#include <tuple>
#include <type_traits>
#include <utility>

namespace range3 {

template <typename T, typename Lockable = std::mutex>
class SynchronizedValue {
 public:
  using value_type = T;
  using mutex_type = Lockable;

 private:
  value_type value_;
  mutable mutex_type mutex_;

 public:
  class ConstLockGuardPtr : public std::lock_guard<mutex_type> {
   public:
    using synchronized_value_type = SynchronizedValue<value_type, mutex_type>;

    ConstLockGuardPtr(synchronized_value_type const& sync_value)
        : std::lock_guard<mutex_type>(sync_value.mutex_),
          value_(sync_value.value_) {}
    ConstLockGuardPtr(synchronized_value_type const& sync_value,
                      std::adopt_lock_t tag)
        : std::lock_guard<mutex_type>(sync_value.mutex_, tag),
          value_(sync_value.value_) {}

    auto operator->() const -> const value_type* { return &value_; }
    auto operator*() const -> const value_type& { return value_; }

   protected:
    value_type const& value_;
  };
  class LockGuardPtr : public ConstLockGuardPtr {
   public:
    using typename ConstLockGuardPtr::synchronized_value_type;

    LockGuardPtr(synchronized_value_type& sync_value)
        : ConstLockGuardPtr(sync_value) {}
    LockGuardPtr(synchronized_value_type& sync_value, std::adopt_lock_t tag)
        : ConstLockGuardPtr(sync_value, tag) {}

    auto operator->() -> value_type* {
      return const_cast<value_type*>(&this->value_);
    }
    auto operator*() -> value_type& {
      return const_cast<value_type&>(this->value_);
    }
  };

  class ConstUniqueLockPtr : public std::unique_lock<mutex_type> {
    using base_type = std::unique_lock<mutex_type>;

   public:
    using synchronized_value_type = SynchronizedValue<value_type, mutex_type>;

    ConstUniqueLockPtr(synchronized_value_type const& sync_value)
        : base_type(sync_value.mutex_), value_(sync_value.value_){};
    ConstUniqueLockPtr(synchronized_value_type const& sync_value,
                       std::adopt_lock_t tag)
        : base_type(sync_value.mutex_, tag), value_(sync_value.value_){};

    auto operator->() const -> const value_type* { return &value_; }
    auto operator*() const -> const value_type& { return value_; }

   protected:
    value_type const& value_;
  };
  class UniqueLockPtr : public ConstUniqueLockPtr {
   public:
    using typename ConstUniqueLockPtr::synchronized_value_type;

    UniqueLockPtr(synchronized_value_type& sync_value)
        : ConstUniqueLockPtr(sync_value) {}
    UniqueLockPtr(synchronized_value_type& sync_value, std::adopt_lock_t tag)
        : ConstUniqueLockPtr(sync_value, tag) {}

    auto operator->() -> value_type* {
      return const_cast<value_type*>(&this->value_);
    }
    auto operator*() -> value_type& {
      return const_cast<value_type&>(this->value_);
    }
  };

  SynchronizedValue() = default;
  SynchronizedValue(value_type const& other) : value_(other) {}
  SynchronizedValue(value_type&& other) : value_(std::move(other)) {}
  SynchronizedValue(SynchronizedValue const& other)
      : SynchronizedValue(other, std::lock_guard<mutex_type>(other.mutex_)) {}
  SynchronizedValue(SynchronizedValue&& other)
      : SynchronizedValue(std::move(other),
                          std::lock_guard<mutex_type>(other.mutex_)) {}

  auto operator=(SynchronizedValue const& rhs) -> SynchronizedValue& {
    if (&rhs != this) {
      std::scoped_lock lock{mutex_, rhs.mutex_};
      value_ = rhs.value_;
    }
    return *this;
  }
  auto operator=(SynchronizedValue&& rhs) -> SynchronizedValue& {
    if (&rhs != this) {
      std::scoped_lock lock{mutex_, rhs.mutex_};
      value_ = std::move(rhs.value_);
    }
    return *this;
  }
  auto operator=(value_type const& other) -> SynchronizedValue& {
    std::lock_guard<mutex_type> lock(mutex_);
    value_ = other;
    return *this;
  }
  auto operator=(value_type&& other) -> SynchronizedValue& {
    std::lock_guard<mutex_type> lock(mutex_);
    value_ = std::move(other);
    return *this;
  }
  auto get() const -> value_type {
    std::lock_guard<mutex_type> lock(mutex_);
    return value_;
  }
  explicit operator T() const { return get(); }

  void swap(SynchronizedValue& rhs) {
    if (this == &rhs) {
      return;
    }
    std::scoped_lock lock{mutex_, rhs.mutex_};
    std::swap(value_, rhs.value_);
  }
  void swap(value_type& rhs) {
    std::lock_guard<mutex_type> lock(mutex_);
    std::swap(value_, rhs);
  }
  template <typename OStream>
  void save(OStream& os) const {
    std::lock_guard<mutex_type> lock(mutex_);
    os << value_;
  }
  template <typename IStream>
  void load(IStream& is) {
    std::lock_guard<mutex_type> lock(mutex_);
    is >> value_;
  }

  // LockGuardPtr is non-copyable and non-movable, but
  // C++17 guarantees prvalue copy elision. so it's legal.
  auto synchronize() -> LockGuardPtr { return {*this}; }
  auto synchronize() const -> ConstLockGuardPtr { return {*this}; }
  auto uniqueSynchronize() -> UniqueLockPtr { return {*this}; }
  auto uniqueSynchronize() const -> ConstUniqueLockPtr { return {*this}; }

  auto operator->() -> LockGuardPtr { return synchronize(); }
  auto operator->() const -> ConstLockGuardPtr { return synchronize(); }

 private:
  SynchronizedValue(SynchronizedValue const& other,
                    std::lock_guard<mutex_type> const&)
      : value_(other.value_) {}
  SynchronizedValue(SynchronizedValue&& other,
                    std::lock_guard<mutex_type> const&)
      : value_(std::move(other.value_)) {}
};

// template <typename SV1, typename SV2>
// auto synchronize(SV1& sv1, SV2& sv2) -> std::tuple<
//     typename std::conditional<std::is_const<SV1>::value,
//                               typename SV1::ConstUniqueLockPtr,
//                               typename SV1::UniqueLockPtr>::type,
//     typename std::conditional<std::is_const<SV2>::value,
//                               typename SV2::ConstUniqueLockPtr,
//                               typename SV2::UniqueLockPtr>::type> {
//   std::lock(sv1.mutex_, sv2.mutex_);

//   return std::make_tuple(
//       typename std::conditional<
//           std::is_const<SV1>::value, typename SV1::ConstUniqueLockPtr,
//           typename SV1::UniqueLockPtr>::type(sv1, std::adopt_lock),
//       typename std::conditional<
//           std::is_const<SV2>::value, typename SV2::ConstUniqueLockPtr,
//           typename SV2::UniqueLockPtr>::type(sv2, std::adopt_lock));
// }

template <typename... SV>
auto synchronize(SV&... sv) -> std::tuple<
    typename std::conditional<std::is_const<SV>::value,
                              typename SV::ConstUniqueLockPtr,
                              typename SV::UniqueLockPtr>::type...> {
  std::lock(sv.mutex_...);

  return std::make_tuple(
      typename std::conditional<
          std::is_const<SV>::value, typename SV::ConstUniqueLockPtr,
          typename SV::UniqueLockPtr>::type(sv, std::adopt_lock)...);
}

template <typename T, typename L>
inline void swap(SynchronizedValue<T, L>& lhs, SynchronizedValue<T, L>& rhs) {
  lhs.swap(rhs);
}
template <typename T, typename L>
inline void swap(SynchronizedValue<T, L>& lhs, T& rhs) {
  lhs.swap(rhs);
}
template <typename T, typename L>
inline void swap(T& lhs, SynchronizedValue<T, L>& rhs) {
  rhs.swap(lhs);
}

template <typename OStream, typename T, typename L>
inline auto operator<<(OStream& os, SynchronizedValue<T, L> const& rhs)
    -> OStream& {
  rhs.save(os);
  return os;
}
template <typename IStream, typename T, typename L>
inline auto operator>>(IStream& is, SynchronizedValue<T, L>& rhs) -> IStream& {
  rhs.load(is);
  return is;
}

}  // namespace range3

#endif
