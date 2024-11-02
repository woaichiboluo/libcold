#ifndef COLD_UTIL_INTHELPER
#define COLD_UTIL_INTHELPER

#include <algorithm>

#include "Endian.h"

namespace Cold {

template <typename T, typename It>
It WriteInt(T value, It b) requires(
    std::is_integral_v<T> &&
    sizeof(typename std::iterator_traits<It>::value_type) == 1) {
  constexpr size_t size = sizeof(T);
  using Type = typename std::iterator_traits<It>::value_type;
  T network = value;
  if constexpr (std::is_same_v<int16_t, T> || std::is_same_v<uint16_t, T>) {
    network = static_cast<T>(
        Endian::Host16ToNetwork16(static_cast<uint16_t>(network)));
  } else if constexpr (std::is_same_v<int32_t, T> ||
                       std::is_same_v<uint32_t, T>) {
    network = static_cast<T>(
        Endian::Host32ToNetwork32(static_cast<uint32_t>(network)));
  } else if constexpr (std::is_same_v<int64_t, T> ||
                       std::is_same_v<uint64_t, T>) {
    network = static_cast<T>(
        Endian::Host64ToNetwork64(static_cast<uint64_t>(network)));
  }
  const Type* begin = reinterpret_cast<const Type*>(&network);
  const Type* end = begin + size;
  std::copy(begin, end, b);
  auto e = b;
  std::advance(e, size);
  return e;
}

template <typename T, typename It>
size_t WriteInt(T value, std::back_insert_iterator<It> b) requires(
    std::is_integral_v<T>) {
  constexpr size_t size = sizeof(T);
  using Type = uint8_t;
  T network = value;
  if constexpr (std::is_same_v<int16_t, T> || std::is_same_v<uint16_t, T>) {
    network = static_cast<T>(
        Endian::Host16ToNetwork16(static_cast<uint16_t>(network)));
  } else if constexpr (std::is_same_v<int32_t, T> ||
                       std::is_same_v<uint32_t, T>) {
    network = static_cast<T>(
        Endian::Host32ToNetwork32(static_cast<uint32_t>(network)));
  } else if constexpr (std::is_same_v<int64_t, T> ||
                       std::is_same_v<uint64_t, T>) {
    network = static_cast<T>(
        Endian::Host64ToNetwork64(static_cast<uint64_t>(network)));
  }
  const Type* begin = reinterpret_cast<const Type*>(&network);
  const Type* end = begin + size;
  std::copy(begin, end, b);
  return size;
}

template <typename T, typename It>
It ReadInt(T& value, It b) requires(
    std::is_integral_v<T> &&
    sizeof(typename std::iterator_traits<It>::value_type) == 1) {
  constexpr size_t size = sizeof(T);
  using Type = typename std::iterator_traits<It>::value_type;
  auto e = b;
  std::advance(e, size);
  std::copy(b, e, reinterpret_cast<Type*>(&value));
  if constexpr (std::is_same_v<int16_t, T> || std::is_same_v<uint16_t, T>) {
    value =
        static_cast<T>(Endian::Network16ToHost16(static_cast<uint16_t>(value)));
  } else if constexpr (std::is_same_v<int32_t, T> ||
                       std::is_same_v<uint32_t, T>) {
    value =
        static_cast<T>(Endian::Network32ToHost32(static_cast<uint32_t>(value)));
  } else if constexpr (std::is_same_v<int64_t, T> ||
                       std::is_same_v<uint64_t, T>) {
    value =
        static_cast<T>(Endian::Network64ToHost64(static_cast<uint64_t>(value)));
  }
  return e;
}

template <typename T, typename It>
T ReadInt(It b) {
  T value;
  ReadInt(value, b);
  return value;
}

}  // namespace Cold

#endif /* COLD_UTIL_INTHELPER */
