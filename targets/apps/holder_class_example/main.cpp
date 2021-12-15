#include <memory>
#include <vector>

// https://stackoverflow.com/questions/62110426/why-does-code-with-stdvector-not-compile-but-with-stdunique-ptr-it-does-if

template <typename T>
struct Holder {
  T v;

  Holder() = default;

  // NG! Using both will result in a compilation error.
  // Holder(Holder const& other) : v(other.v) {}
  Holder(Holder&& other) : v(std::move(other).v) {}

  // OK but it needs c++20 concepts
  // Holder(Holder const& other) requires std::is_copy_constructible_v<T>
  //     : v(other.v) {}

  // OK
  Holder(Holder const& other) = default;

  // OK
  // Holder(Holder&&) = default;

  Holder(T const& other) : v(other) {}
  Holder(T&& other) : v(std::move(other)) {}
};

auto main(int argc, char* argv[]) -> int {
  (void)argc;
  (void)argv;

  // OK
  std::vector<std::unique_ptr<int>> v1;
  v1.emplace_back(std::make_unique<int>(100));

  // Possible compile error.
  std::vector<Holder<std::unique_ptr<int>>> v2;
  v2.emplace_back(std::make_unique<int>(200));

  return 0;
}
