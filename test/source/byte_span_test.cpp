#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "lsm-tree/byte_span.hpp"

using lsm::utils::basic_byte_span;
using lsm::utils::byte_span;
using lsm::utils::byte_view;
using lsm::utils::cbyte_span;
using lsm::utils::cbyte_view;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

struct trivial_struct {
  int x;
  double y;
};

struct non_trivial_struct {
  std::vector<int> data;
};

TEST_CASE("byte_view basic functionality", "[byte_view]") {
  // std::array<std::byte, 3> data{std::byte{1}, std::byte{2}, std::byte{3}};

  SECTION("default constructor") {
    STATIC_REQUIRE(std::is_constructible_v<byte_view>);
    STATIC_REQUIRE(std::is_constructible_v<cbyte_view>);
    STATIC_REQUIRE(std::is_constructible_v<basic_byte_span<std::byte, 0>>);

    STATIC_REQUIRE_FALSE(
        std::is_constructible_v<basic_byte_span<std::byte, 10>>);
  }

  SECTION("empty span") {
    auto const span = byte_view{};
    REQUIRE(span.empty());
  }

  SECTION("construction from different byte types") {
    auto char_data = std::array<char, 3>{'a', 'b', 'c'};
    auto uchar_data = std::array<unsigned char, 3>{0x01, 0x02, 0x03};
    auto byte_data =
        std::array<std::byte, 3>{std::byte{1}, std::byte{2}, std::byte{3}};

    STATIC_REQUIRE(std::is_constructible_v<byte_view, char*, std::size_t>);
    STATIC_REQUIRE(
        std::is_constructible_v<byte_view, unsigned char*, std::size_t>);
    STATIC_REQUIRE(std::is_constructible_v<byte_view, std::byte*, std::size_t>);

    // cannot construct from const pointer
    STATIC_REQUIRE_FALSE(
        std::is_constructible_v<byte_view, const char*, std::size_t>);
    STATIC_REQUIRE_FALSE(
        std::is_constructible_v<byte_view, const unsigned char*, std::size_t>);
    STATIC_REQUIRE_FALSE(
        std::is_constructible_v<byte_view, const std::byte*, std::size_t>);

    // can construct from const pointer
    STATIC_REQUIRE(
        std::is_constructible_v<cbyte_view, const char*, std::size_t>);
    STATIC_REQUIRE(
        std::is_constructible_v<cbyte_view, const unsigned char*, std::size_t>);
    STATIC_REQUIRE(
        std::is_constructible_v<cbyte_view, const std::byte*, std::size_t>);

    STATIC_REQUIRE_FALSE(
        std::is_constructible_v<std::span<char>, const char*, std::size_t>);

    auto const s1 = byte_view{char_data.data(), char_data.size()};
    auto const s2 = byte_view{uchar_data.data(), uchar_data.size()};
    auto const s3 = byte_view{byte_data.data(), byte_data.size()};

    auto const const_char_data = std::array<char, 3>{'x', 'y', 'z'};
    auto const const_uchar_data =
        std::array<unsigned char, 3>{0x04, 0x05, 0x06};
    auto const const_byte_data =
        std::array<std::byte, 3>{std::byte{4}, std::byte{5}, std::byte{6}};

    auto const s7 = cbyte_view{const_char_data.data(), const_char_data.size()};
    auto const s8 =
        cbyte_view{const_uchar_data.data(), const_uchar_data.size()};
    auto const s9 = cbyte_view{const_byte_data.data(), const_byte_data.size()};
  }

  SECTION("construction from non-byte trivially copyable types") {
    std::array<int, 3> int_data{1, 2, 3};
    std::array<double, 2> double_data{1.0, 2.0};
    std::array<trivial_struct, 2> struct_data{{{1, 1.0}, {2, 2.0}}};

    // allow explicit conversion from non-byte types
    STATIC_REQUIRE(std::is_constructible_v<byte_view, int*, std::size_t>);
    STATIC_REQUIRE(std::is_constructible_v<byte_view, double*, std::size_t>);
    STATIC_REQUIRE(
        std::is_constructible_v<byte_view, trivial_struct*, std::size_t>);

    // prevent implicit conversion from non-byte types
    STATIC_REQUIRE_FALSE(
        std::is_convertible_v<std::pair<int*, std::size_t>, byte_view>);
    STATIC_REQUIRE_FALSE(
        std::is_convertible_v<std::pair<double*, std::size_t>, byte_view>);
    STATIC_REQUIRE_FALSE(
        std::is_convertible_v<std::pair<trivial_struct*, std::size_t>,
                              byte_view>);

    // prevent construction from non-trivially copyable types
    STATIC_REQUIRE_FALSE(
        std::is_constructible_v<byte_view, non_trivial_struct*, std::size_t>);
    STATIC_REQUIRE_FALSE(
        std::is_constructible_v<byte_view, std::vector<int>*, std::size_t>);

    auto s1 = byte_view{int_data.data(), int_data.size()};
    auto s2 = byte_view{double_data.data(), double_data.size()};
    auto s3 = byte_view{struct_data.data(), struct_data.size()};

    REQUIRE(s1.size() == sizeof(int) * int_data.size());
    REQUIRE(s2.size() == sizeof(double) * double_data.size());
    REQUIRE(s3.size() == sizeof(trivial_struct) * struct_data.size());

    auto s4 = byte_view{int_data.begin(), int_data.end()};
    auto s5 = byte_view{double_data.begin(), double_data.end()};
    auto s6 = byte_view{struct_data.begin(), struct_data.end()};

    REQUIRE(s4.size() == sizeof(int) * int_data.size());
    REQUIRE(s5.size() == sizeof(double) * double_data.size());
    REQUIRE(s6.size() == sizeof(trivial_struct) * struct_data.size());
  }

  SECTION("construction from void pointers") {
    std::array<std::byte, 8> buffer{};
    void* data = buffer.data();
    const void* const_data = buffer.data();

    STATIC_REQUIRE(std::is_constructible_v<byte_view, void*, std::size_t>);
    STATIC_REQUIRE(std::is_constructible_v<cbyte_view, void*, std::size_t>);
    STATIC_REQUIRE(
        std::is_constructible_v<cbyte_view, const void*, std::size_t>);

    // const safety
    STATIC_REQUIRE_FALSE(
        std::is_constructible_v<byte_view, const void*, std::size_t>);

    // implicit conversion from void* is not allowed
    STATIC_REQUIRE_FALSE(
        std::is_convertible_v<std::pair<void*, std::size_t>, byte_view>);
    STATIC_REQUIRE_FALSE(
        std::is_convertible_v<std::pair<const void*, std::size_t>, cbyte_view>);

    auto s1 = byte_view{data, 8};
    auto s2 = cbyte_view{data, 8};
    auto s3 = cbyte_view{const_data, 8};
    // byte_view{const_data, 8};

    REQUIRE(s1.size() == 8);
    REQUIRE(s2.size() == 8);
    REQUIRE(s3.size() == 8);
    REQUIRE(s1.data() == buffer.data());
  }

  SECTION("void pointer alignment and size handling") {
    alignas(8) std::array<std::byte, 16> aligned_buffer{};
    void* aligned_data = aligned_buffer.data();

    // aligned data
    auto s1 = byte_view{aligned_data, 16};
    REQUIRE(reinterpret_cast<std::uintptr_t>(s1.data()) % 8 == 0);

    // zero size
    auto s2 = byte_view{aligned_data, 0};
    REQUIRE(s2.empty());

    // nullptr and zero size
    auto s3 = byte_view{static_cast<void*>(nullptr), 0};
    REQUIRE(s3.empty());

    // technically valid but dangerous
    auto s4 = byte_view{aligned_data, 1000};
    REQUIRE(s4.size() == 1000);
  }

  SECTION("void pointer type variations") {
    std::array<std::byte, 8> buffer{};
    volatile void* volatile_data = buffer.data();
    const volatile void* const_volatile_data = buffer.data();

    // construct from volatile void*
    auto s1 = byte_view{const_cast<void*>(volatile_data), 8};         // NOLINT
    auto s2 = cbyte_view{const_cast<const void*>(volatile_data), 8};  // NOLINT

    // construct from const volatile void*
    // byte_view{const_volatile_data, 8}; // error
    auto s3 = basic_byte_span<const volatile std::byte>{const_volatile_data, 8};

    REQUIRE(s1.size() == 8);
    REQUIRE(s2.size() == 8);
    REQUIRE(s3.size() == 8);
  }
}

TEST_CASE("byte_view construction from raw array", "[byte_view][array]") {
  SECTION("construction from raw array") {
    char char_data[3] = {'a', 'b', 'c'};               // NOLINT
    unsigned char uchar_data[3] = {0x01, 0x02, 0x03};  // NOLINT
    // NOLINTNEXTLINE
    std::byte byte_data[3] = {
        std::byte{1},
        std::byte{2},
        std::byte{3},
    };

    auto s1 = basic_byte_span{char_data};
    auto s2 = basic_byte_span{uchar_data};
    auto s3 = basic_byte_span{byte_data};

    REQUIRE(s1.size() == 3);
    REQUIRE(s2.size() == 3);
    REQUIRE(s3.size() == 3);
    REQUIRE(s1.extent == 3);
    REQUIRE(s2.extent == 3);
    REQUIRE(s3.extent == 3);
    REQUIRE(reinterpret_cast<char*>(s1.data()) == char_data);  // NOLINT
    REQUIRE(reinterpret_cast<unsigned char*>(s2.data())
            == uchar_data);                                         // NOLINT
    REQUIRE(reinterpret_cast<std::byte*>(s3.data()) == byte_data);  // NOLINT
  }

  SECTION("construction from const raw array") {}

  SECTION("construction from raw array with different types") {}
}

TEST_CASE("byte_view construction from ranges", "[byte_view][range]") {
  SECTION("construction from byte containers") {
    // std::vector<char>
    auto char_vec = std::vector<char>{'a', 'b', 'c'};
    auto char_span = byte_view{char_vec};
    REQUIRE(char_span.size() == 3);
    REQUIRE(reinterpret_cast<const char*>(char_span.data()) == char_vec.data());

    // std::string
    auto str = std::string{"hello"};
    auto str_span = byte_view{str};
    REQUIRE(str_span.size() == 5);
    REQUIRE(reinterpret_cast<const char*>(str_span.data()) == str.data());

    // const containers to cbyte_view
    auto const_uchar_vec = std::vector<unsigned char>{0x01, 0x02, 0x03};
    auto const_span = cbyte_view{const_uchar_vec};
    REQUIRE(const_span.size() == 3);
    REQUIRE(reinterpret_cast<const unsigned char*>(const_span.data())
            == const_uchar_vec.data());
  }

  SECTION("construction from non-byte containers") {
    // std::vector<int>
    auto int_vec = std::vector<int>{1, 2, 3, 4};
    auto int_span = byte_view{int_vec};
    REQUIRE(int_span.size() == sizeof(int) * 4);
    REQUIRE(reinterpret_cast<const int*>(int_span.data()) == int_vec.data());

    // const containers
    auto const const_double_vec = std::vector<double>{1.0, 2.0};
    auto double_span = cbyte_view{const_double_vec};
    REQUIRE(double_span.size() == sizeof(double) * 2);
    REQUIRE(reinterpret_cast<const double*>(double_span.data())
            == const_double_vec.data());
  }

  SECTION("construction from std::span") {
    auto vec = std::vector<int>{1, 2, 3, 4};
    auto int_span = std::span{vec};
    auto span_from_span = byte_view{int_span};
    REQUIRE(span_from_span.size() == sizeof(int) * 4);
    REQUIRE(reinterpret_cast<const int*>(span_from_span.data()) == vec.data());
  }

  SECTION("construction from std::as_bytes/as_writable_bytes") {
    auto vec = std::vector<int>{1, 2, 3, 4};
    auto bytes_span = std::as_bytes(std::span{vec});
    auto span_from_bytes = cbyte_view{bytes_span};
    REQUIRE(span_from_bytes.size() == sizeof(int) * 4);
    REQUIRE(span_from_bytes.data() == bytes_span.data());

    auto writable_span = std::as_writable_bytes(std::span{vec});
    auto span_from_writable = byte_view{writable_span};
    REQUIRE(span_from_writable.size() == sizeof(int) * 4);
    REQUIRE(span_from_writable.data() == writable_span.data());
  }

  SECTION("borrowed range safety") {
    // Temporary non-const range should not be allowed
    STATIC_REQUIRE_FALSE(
        std::is_constructible_v<byte_view, std::vector<char>&&>);

    // Temporary const range to cbyte_view should be allowed
    STATIC_REQUIRE(
        std::is_constructible_v<cbyte_view, const std::vector<char>&&>);
  }

  SECTION("constness safety") {
    auto const const_vec = std::vector<char>{'a', 'b', 'c'};

    // const container to non-const span should fail
    STATIC_REQUIRE_FALSE(
        std::is_constructible_v<byte_view, const std::vector<char>&>);

    // const container to const span should work
    STATIC_REQUIRE(
        std::is_constructible_v<cbyte_view, const std::vector<char>&>);
  }
}

#pragma GCC diagnostic pop
