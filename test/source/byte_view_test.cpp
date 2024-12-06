#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <span>
#include <stdexcept>
#include <type_traits>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "lsm-tree/byte_view.hpp"

using lsm::utils::basic_byte_view;
using lsm::utils::byte_view;
using lsm::utils::cbyte_view;

TEST_CASE("byte_view basic functionality", "[byte_view]") {
  std::array<std::byte, 3> data{std::byte{1}, std::byte{2}, std::byte{3}};

  SECTION("construct and basic accessors") {
    auto const view = byte_view(data.data(), data.size());

    REQUIRE(view.data() == data.data());
    REQUIRE(view.size() == 3);
    REQUIRE_FALSE(view.empty());
  }

  SECTION("empty view") {
    auto const view = byte_view{};

    REQUIRE(view.data() == nullptr);
    REQUIRE(view.size() == 0);  // NOLINT
    REQUIRE(view.empty());
  }

  SECTION("element access with operator[]") {
    auto const view = byte_view(data.data(), data.size());

    REQUIRE(std::to_integer<int>(view[0]) == 1);
    REQUIRE(std::to_integer<int>(view[1]) == 2);
    REQUIRE(std::to_integer<int>(view[2]) == 3);

    // writable
    view[1] = std::byte{42};
    REQUIRE(std::to_integer<int>(view[1]) == 42);
    REQUIRE(std::to_integer<int>(data[1]) == 42);
  }

  SECTION("element access with at()") {
    auto const view = byte_view(data.data(), data.size());

    REQUIRE(std::to_integer<int>(view.at(0)) == 1);
    REQUIRE(std::to_integer<int>(view.at(1)) == 2);
    REQUIRE(std::to_integer<int>(view.at(2)) == 3);

    REQUIRE_THROWS_AS(view.at(3), std::out_of_range);

    // writable
    view.at(1) = std::byte{42};
    REQUIRE(std::to_integer<int>(view.at(1)) == 42);
    REQUIRE(std::to_integer<int>(data[1]) == 42);
  }

  SECTION("front/back access") {
    auto const view = byte_view(data.data(), data.size());

    REQUIRE(std::to_integer<int>(view.front()) == 1);
    REQUIRE(std::to_integer<int>(view.back()) == 3);

    // writable
    view.front() = std::byte{42};
    view.back() = std::byte{43};
    REQUIRE(std::to_integer<int>(data[0]) == 42);
    REQUIRE(std::to_integer<int>(data[2]) == 43);
  }

  SECTION("iterator operations") {
    auto const view = byte_view(data.data(), data.size());

    // iterator
    REQUIRE(std::distance(view.begin(), view.end()) == 3);
    REQUIRE(std::to_integer<int>(*view.begin()) == 1);
    REQUIRE(std::to_integer<int>(*(view.end() - 1)) == 3);

    // writable
    *view.begin() = std::byte{42};
    REQUIRE(std::to_integer<int>(data[0]) == 42);

    // const_iterator
    REQUIRE(std::distance(view.cbegin(), view.cend()) == 3);
    REQUIRE(std::to_integer<int>(*view.cbegin()) == 42);

    // reverse_iterator
    auto rbegin = view.rbegin();
    auto rend = view.rend();
    REQUIRE(std::distance(rbegin, rend) == 3);
    REQUIRE(std::to_integer<int>(*rbegin) == 3);
    REQUIRE(std::to_integer<int>(*(rend - 1)) == 42);

    // const_reverse_iterator
    auto crbegin = view.crbegin();
    auto crend = view.crend();
    REQUIRE(std::distance(crbegin, crend) == 3);
    REQUIRE(std::to_integer<int>(*crbegin) == 3);

    // compatible with std::algorithms
    std::array<std::byte, 3> expected{std::byte{42}, std::byte{2},
                                      std::byte{3}};
    REQUIRE(std::equal(view.begin(), view.end(), expected.begin()));

    // reverse compatible with std::algorithms
    std::array<std::byte, 3> reversed{std::byte{3}, std::byte{2},
                                      std::byte{42}};
    REQUIRE(std::equal(view.rbegin(), view.rend(), reversed.begin()));
  }

  SECTION("swap operation") {
    std::array<std::byte, 3> data1{std::byte{1}, std::byte{2}, std::byte{3}};
    std::array<std::byte, 2> data2{std::byte{4}, std::byte{5}};

    auto view1 = byte_view(data1.data(), data1.size());
    auto view2 = byte_view(data2.data(), data2.size());

    // swap by member function
    view1.swap(view2);

    // view1 reference data2
    REQUIRE(view1.data() == data2.data());
    REQUIRE(view1.size() == 2);
    REQUIRE(std::to_integer<int>(view1[0]) == 4);
    REQUIRE(std::to_integer<int>(view1[1]) == 5);

    // view2 reference data1
    REQUIRE(view2.data() == data1.data());
    REQUIRE(view2.size() == 3);
    REQUIRE(std::to_integer<int>(view2[0]) == 1);
    REQUIRE(std::to_integer<int>(view2[1]) == 2);
    REQUIRE(std::to_integer<int>(view2[2]) == 3);

    // swap back by non-member function
    swap(view1, view2);

    REQUIRE(view1.data() == data1.data());
    REQUIRE(view1.size() == 3);
    REQUIRE(std::to_integer<int>(view1[0]) == 1);

    REQUIRE(view2.data() == data2.data());
    REQUIRE(view2.size() == 2);
    REQUIRE(std::to_integer<int>(view2[0]) == 4);
  }
}

TEST_CASE("byte_view conversion constraints", "[byte_view]") {
  // byte type
  STATIC_REQUIRE(std::is_constructible_v<byte_view, char*, std::size_t>);
  STATIC_REQUIRE(
      std::is_constructible_v<byte_view, unsigned char*, std::size_t>);
  STATIC_REQUIRE(std::is_constructible_v<byte_view, std::byte*, std::size_t>);

  // non-byte type
  STATIC_REQUIRE(std::is_constructible_v<byte_view, int*, std::size_t>);
  STATIC_REQUIRE(std::is_constructible_v<byte_view, float*, std::size_t>);
  STATIC_REQUIRE(std::is_constructible_v<byte_view, double*, std::size_t>);
}

// Byte type constructors
TEMPLATE_TEST_CASE("construct from different byte types",
                   "[byte_view][template]",
                   char,
                   unsigned char,
                   std::byte) {
  auto data = std::array<TestType, 3>{TestType{1}, TestType{2}, TestType{3}};

  SECTION("construct with pointer and size") {
    auto view = byte_view(data.data(), data.size());
    REQUIRE(view.size() == 3);
    REQUIRE_FALSE(view.empty());
  }

  SECTION("construct with pointer range") {
    auto view = byte_view(data.data(), data.data() + data.size());
    REQUIRE(view.size() == 3);
    REQUIRE_FALSE(view.empty());
  }

  SECTION("construct empty view") {
    auto view = byte_view(data.data(), size_t{0});
    REQUIRE(view.size() == 0);  // NOLINT
    REQUIRE(view.empty());
  }
}

// TEST_CASE("test", "[test]") {
//   SECTION("test") {
//     auto data =
//         std::array<std::byte, 3>{std::byte{1}, std::byte{2}, std::byte{3}};
//     auto view = byte_view(data.begin(), data.end());
//     // REQUIRE(view.empty());
//     REQUIRE(view.size() == 3);
//   }

//   SECTION("span") {
//     auto data =
//         std::array<std::byte, 3>{std::byte{1}, std::byte{2}, std::byte{3}};
//     auto view = std::span(data.data(), 0);
//     REQUIRE(view.size() == 3);
//     REQUIRE_FALSE(view.empty());
//   }
// }

// Non-byte type constructors
TEMPLATE_TEST_CASE("construct from non-byte types",
                   "[byte_view][template]",
                   int,
                   float,
                   double) {
  auto data = std::array<TestType, 3>{TestType{1}, TestType{2}, TestType{3}};

  SECTION("construct with pointer and size") {
    auto view = byte_view(data.data(), data.size());
    REQUIRE(view.size() == 3 * sizeof(TestType));
    REQUIRE_FALSE(view.empty());
  }

  SECTION("construct with pointer range") {
    auto view = byte_view(data.data(), data.data() + data.size());
    REQUIRE(view.size() == 3 * sizeof(TestType));
    REQUIRE_FALSE(view.empty());
  }

  SECTION("construct empty view") {
    auto view = byte_view(data.data(), 0);
    REQUIRE(view.size() == 0);  // NOLINT
    REQUIRE(view.empty());
  }
}
