#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>

namespace lsm::utils {

using std::dynamic_extent;

template <typename B, size_t Extent>
class basic_byte_span;

namespace detail {

template <typename T>
concept byte_like = std::same_as<std::remove_cv_t<T>, char>
                 || std::same_as<std::remove_cv_t<T>, unsigned char>
                 || std::same_as<std::remove_cv_t<T>, std::byte>;

template <typename From, typename To>
concept const_convertible = std::is_const_v<std::remove_reference_t<To>>
                         || !std::is_const_v<std::remove_reference_t<From>>;

template <typename Range>
concept byte_range = requires {
  requires std::ranges::contiguous_range<Range>;
  requires std::ranges::sized_range<Range>;
  requires byte_like<std::ranges::range_value_t<Range>>;
};

template <typename Range>
concept non_byte_range = requires {
  requires std::ranges::contiguous_range<Range>;
  requires std::ranges::sized_range<Range>;
  requires !byte_like<std::ranges::range_value_t<Range>>;
  requires std::is_trivially_copyable_v<std::ranges::range_value_t<Range>>;
};

template <typename Range, typename ElementType>
concept const_safe_range =
    std::is_const_v<ElementType>
    || ((!std::is_const_v<
            std::remove_reference_t<std::ranges::range_value_t<Range>>>)
        && (!std::is_const_v<std::remove_reference_t<Range>>)
        && std::ranges::borrowed_range<Range>);

template <typename T>
concept is_basic_byte_span = requires {
  typename T::element_type;
} && std::same_as<T, basic_byte_span<typename T::element_type, T::extent>>;

}  // namespace detail

template <typename B, size_t Extent = dynamic_extent>
class basic_byte_span {
  static_assert(std::same_as<std::remove_cv_t<B>, std::byte>,
                "basic_byte_span can only be instantiated with std::byte");

 public:
  using span_type = std::span<B, Extent>;
  using element_type = typename span_type::element_type;
  using value_type = typename span_type::value_type;
  using size_type = typename span_type::size_type;
  using difference_type = typename span_type::difference_type;
  using pointer = typename span_type::pointer;
  using const_pointer = typename span_type::const_pointer;
  using reference = typename span_type::reference;
  using const_reference = typename span_type::const_reference;
  using iterator = typename span_type::iterator;
  using reverse_iterator = typename span_type::reverse_iterator;
  // using const_iterator = typename span_type::const_iterator;
  // using const_reverse_iterator = typename span_type::const_reverse_iterator;

  static constexpr size_t extent = Extent;

  constexpr basic_byte_span() noexcept
    requires(Extent == dynamic_extent || Extent == 0)
      : span_{} {}

  // From iterator and sentinel
  template <std::contiguous_iterator It, std::sized_sentinel_for<It> End>
    requires std::is_trivially_copyable_v<std::iter_value_t<It>>
          && detail::const_convertible<std::iter_reference_t<It>, element_type&>
  constexpr explicit(Extent != dynamic_extent
                     || !detail::byte_like<std::iter_value_t<It>>)
      basic_byte_span(It first, End last) noexcept(noexcept(last - first))
      : span_{reinterpret_cast<pointer>(std::to_address(first)),
              static_cast<size_type>(last - first)
                  * sizeof(std::iter_value_t<It>)} {}

  // From iterator and count
  template <std::contiguous_iterator It>
    requires std::is_trivially_copyable_v<std::iter_value_t<It>>
          && detail::const_convertible<std::iter_reference_t<It>, element_type&>
  constexpr explicit(Extent != dynamic_extent
                     || !detail::byte_like<std::iter_value_t<It>>)
      basic_byte_span(It first, size_type count) noexcept
      : span_{reinterpret_cast<pointer>(std::to_address(first)),
              count * sizeof(std::iter_value_t<It>)} {}

  // For void* - always explicit
  // void* and size
  template <typename VoidType>
    requires std::is_void_v<VoidType>
          && detail::const_convertible<VoidType, element_type>
  constexpr explicit basic_byte_span(VoidType* data, size_type size) noexcept
      : span_{reinterpret_cast<pointer>(data), size} {}

  // For c-style arrays
  template <typename T, size_t ArrayExtent>
    requires std::is_trivially_copyable_v<T>
          && (Extent == dynamic_extent || Extent == ArrayExtent * sizeof(T))
          && detail::const_convertible<T, element_type>
  constexpr explicit(!detail::byte_like<T>)
      // NOLINTNEXTLINE
      basic_byte_span(T (&arr)[ArrayExtent]) noexcept
      : span_{reinterpret_cast<pointer>(arr), ArrayExtent * sizeof(T)} {}

  // std::array
  template <typename T, size_t ArrayExtent>
    requires std::is_trivially_copyable_v<T>
          && (Extent == dynamic_extent || Extent == ArrayExtent * sizeof(T))
          && detail::const_convertible<T, element_type>
  constexpr explicit(!detail::byte_like<T>)
      // NOLINTNEXTLINE
      basic_byte_span(std::array<T, ArrayExtent>& arr) noexcept
      : span_{reinterpret_cast<pointer>(arr.data()), ArrayExtent * sizeof(T)} {}

  // const std::array
  template <typename T, size_t ArrayExtent>
    requires std::is_trivially_copyable_v<T>
          && (Extent == dynamic_extent || Extent == ArrayExtent * sizeof(T))
          && detail::const_convertible<const T, element_type>
  constexpr explicit(!detail::byte_like<T>)
      // NOLINTNEXTLINE
      basic_byte_span(const std::array<T, ArrayExtent>& arr) noexcept
      : span_{reinterpret_cast<pointer>(arr.data()), ArrayExtent * sizeof(T)} {}

  constexpr auto data() const noexcept { return span_.data(); }
  constexpr auto size() const noexcept { return span_.size(); }
  constexpr auto size_bytes() const noexcept { return span_.size_bytes(); }
  constexpr auto empty() const noexcept { return span_.empty(); }

 private:
  // NOLINTNEXTLINE(cppcoreguidelines-use-default-member-init,modernize-use-default-member-init)
  span_type span_;
};

// deduction guides
template <std::contiguous_iterator It>
  requires std::is_const_v<std::remove_reference_t<std::iter_reference_t<It>>>
basic_byte_span(It, size_t) -> basic_byte_span<const std::byte>;

template <std::contiguous_iterator It>
  requires(!std::is_const_v<std::remove_reference_t<std::iter_reference_t<It>>>)
basic_byte_span(It, size_t) -> basic_byte_span<std::byte>;

template <std::contiguous_iterator It, std::sized_sentinel_for<It> End>
  requires std::is_const_v<std::remove_reference_t<std::iter_reference_t<It>>>
basic_byte_span(It, End) -> basic_byte_span<const std::byte>;

template <std::contiguous_iterator It, std::sized_sentinel_for<It> End>
  requires(!std::is_const_v<std::remove_reference_t<std::iter_reference_t<It>>>)
basic_byte_span(It, End) -> basic_byte_span<std::byte>;

template <typename VoidType>
  requires std::is_void_v<VoidType> && std::is_const_v<VoidType>
basic_byte_span(VoidType*, size_t) -> basic_byte_span<const std::byte>;

template <typename VoidType>
  requires std::is_void_v<VoidType> && (!std::is_const_v<VoidType>)
basic_byte_span(VoidType*, size_t) -> basic_byte_span<std::byte>;

template <typename T, size_t ArrayExtent>
  requires std::is_const_v<std::remove_reference_t<T>>
basic_byte_span(T (&)[ArrayExtent])  // NOLINT
    ->basic_byte_span<const std::byte, ArrayExtent * sizeof(T)>;

template <typename T, size_t ArrayExtent>
  requires(!std::is_const_v<std::remove_reference_t<T>>)
basic_byte_span(T (&)[ArrayExtent])  // NOLINT
    ->basic_byte_span<std::byte, ArrayExtent * sizeof(T)>;

template <typename T, size_t ArrayExtent>
  requires(!std::is_const_v<T>)
basic_byte_span(std::array<T, ArrayExtent>&)
    -> basic_byte_span<std::byte, ArrayExtent * sizeof(T)>;

template <typename T, size_t ArrayExtent>
  requires(std::is_const_v<T>)
basic_byte_span(std::array<T, ArrayExtent>&)
    -> basic_byte_span<const std::byte, ArrayExtent * sizeof(T)>;

template <typename T, size_t ArrayExtent>
basic_byte_span(const std::array<T, ArrayExtent>&)
    -> basic_byte_span<const std::byte, ArrayExtent * sizeof(T)>;

// type aliases
template <std::size_t Extent = dynamic_extent>
using byte_span = basic_byte_span<std::byte, Extent>;
template <std::size_t Extent = dynamic_extent>
using cbyte_span = basic_byte_span<const std::byte, Extent>;
using byte_view = byte_span<>;
using cbyte_view = cbyte_span<>;

}  // namespace lsm::utils

/*
    // From ranges (byte types) - implicit conversion allowed
    template <detail::ByteRange Range>
      requires detail::ConstSafeRange<Range, element_type>
            && (!std::is_array_v<std::remove_cvref_t<Range>>)
    constexpr explicit(Extent != std::dynamic_extent)
        // NOLINTNEXTLINE
        basic_byte_span(Range&& range) noexcept
        : basic_byte_span(std::ranges::data(range), std::ranges::size(range))
   { if constexpr (Extent != std::dynamic_extent) {
        assert(std::ranges::size(range) == extent);
      }
    }

    // From ranges (non-byte types) - explicit conversion required
    template <detail::NonByteRange Range>
      requires detail::ConstSafeRange<Range, element_type>
            && (!std::is_array_v<std::remove_cvref_t<Range>>)
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    constexpr explicit basic_byte_span(Range&& range) noexcept
        : basic_byte_span(std::ranges::data(range), std::ranges::size(range))
   { if constexpr (Extent != std::dynamic_extent) {
        assert(std::ranges::size(range)
                   * sizeof(std::ranges::range_value_t<Range>)
               == extent);
      }
    }
    */