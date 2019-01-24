#include <array>
#include <variant>
#include <tuple>
#include <functional>

#include <yaml-cpp/yaml.h>

#pragma once

typedef std::array<uint8_t, 3> AppID_t;

template<typename TVariant, size_t I = std::variant_size_v<TVariant> - 1>
struct GetVariant {
  static constexpr TVariant convertNode(
    const YAML::Node &node, decltype(std::variant_alternative_t<I, TVariant>::type) type
  ) {
    if (type == std::variant_alternative_t<I, TVariant>::type) {
      return node.as<std::variant_alternative_t<I, TVariant>>();
    }
    if constexpr (I == 0) {
      throw std::exception();
    } else {
      return GetVariant<TVariant, I - 1>::convertNode(node, type);
    }
  }
  static constexpr auto make(
    decltype(std::variant_alternative_t<I, TVariant>::type) type
  ) {
    if (type == std::variant_alternative_t<I, TVariant>::type) {
      return std::variant_alternative_t<I, TVariant>();
    }
    if constexpr (I == 0) {
      throw std::exception();
    } else {
      return GetVariant<TVariant, I - 1>::make(type);
    }
  }

  static constexpr auto makeArgs(
    decltype(std::variant_alternative_t<I, TVariant>::type) type,
    auto args
  ) {
    if (type == std::variant_alternative_t<I, TVariant>::type) {
      return std::make_from_tuple<std::variant_alternative_t<I, TVariant>>(
        args
      );
    }
    if constexpr (I == 0) {
      throw std::exception();
    } else {
      return GetVariant<TVariant, I - 1>::make(type);
    }
  }
};
