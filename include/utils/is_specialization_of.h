#pragma once

#include <type_traits>

// Usage example:
//
// template<typename T>
// inline constexpr bool is_map = utils::is_specialization_of_v<T, std::map>;
//
// template<typename T>
// concept ConceptIsMap = is_map<T>;
//
namespace utils {

template<class, template<class...> class>
inline constexpr bool is_specialization_of = false;

template<template<class...> class U, class... Args>
inline constexpr bool is_specialization_of<U<Args...>, U> = true;

template<typename T, template<typename...> typename U>
inline constexpr bool is_specialization_of_v = is_specialization_of<T, U>;

template <typename Derived, template<typename...> typename Base>
struct is_derived_from_specialization_of
{
  template <typename... Args>
  static constexpr bool check(Base<Args...>*) { return std::is_base_of_v<Base<Args...>, Derived>; }

  static constexpr bool check(...) { return false; }

  static constexpr bool value = check((Derived*)nullptr);
};

template<typename Derived, template<typename...> typename Base>
inline constexpr bool is_derived_from_specialization_of_v = is_derived_from_specialization_of<Derived, Base>::value;

} // namespace utils
