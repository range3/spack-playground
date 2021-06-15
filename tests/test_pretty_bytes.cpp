#include <doctest/doctest.h>
#include <iostream>
#include <pretty_bytes/pretty_bytes.hpp>
#include <stdexcept>

TEST_CASE("testing pretty_bytes") {  // NOLINT
  SUBCASE(
      "shoud throw std::invalid_argument if the arg doesn't match a pretty "
      "byte") {
    CHECK_THROWS_AS(pretty_bytes::prettyTo<uint64_t>("hoge"),
                    std::invalid_argument);
    CHECK_THROWS_AS(pretty_bytes::prettyTo<uint64_t>("12A"),
                    std::invalid_argument);
    CHECK_THROWS_AS(pretty_bytes::prettyTo<uint64_t>("20k"),
                    std::invalid_argument);
  }

  SUBCASE("K") {
    CHECK(pretty_bytes::prettyTo<uint64_t>("100K") == 100 * 1024);
  }
  SUBCASE("M") {
    CHECK(pretty_bytes::prettyTo<uint64_t>("5M") == 5 * 1024 * 1024);
  }
  SUBCASE("G") {
    CHECK(pretty_bytes::prettyTo<uint64_t>("2G") == 2UL * 1024 * 1024 * 1024);
  }
  SUBCASE("P") {
    CHECK(pretty_bytes::prettyTo<uint64_t>("1P") ==
          1UL * 1024 * 1024 * 1024 * 1024 * 1024);
  }
  SUBCASE("No suffix") {
    CHECK(pretty_bytes::prettyTo<uint64_t>("512") == 512);
  }
}
