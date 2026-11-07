#pragma once
#include <array>
#include <meta>
#include <string_view>

template <class Enum> constexpr auto generate_enum_to_string_view_array() {
  constexpr std::span<const std::meta::info> enumerators =
      std::define_static_array(std::meta::enumerators_of(^^Enum));

  std::array<std::pair<Enum, std::string_view>, enumerators.size()> ret;

  int i = 0;
  for (const auto e : enumerators) {
    ret[i] = std::pair<Enum, std::string_view>{
        std::meta::extract<Enum>(e),
        std::define_static_string(std::meta::identifier_of(e))};

    i += 1;
  }

  return ret;
}
