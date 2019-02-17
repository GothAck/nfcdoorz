#include <array>
#include <variant>
#include <type_traits>
#include <tuple>
#include <string>
#include <stdexcept>
#include <functional>

#include <yaml-cpp/yaml.h>
#include <freefare.h>

#include "logging.hpp"

#pragma once

void freekey(MifareDESFireKey *key);
void freederiver(MifareKeyDeriver *deriver);
void freeaid(MifareDESFireAID *aid);
void freetag(MifareDESFireAID *aid);

#define CLEAN_KEY __attribute__((cleanup(freekey)))
#define CLEAN_DERIVER __attribute__((cleanup(freederiver)))
#define CLEAN_AID __attribute__((cleanup(freeaid)))

typedef std::array<uint8_t, 3> AppID_t;
typedef std::array<uint8_t, 7> UID_t;

class ConversionException : public std::runtime_error {
  constexpr static const char *prefix = "ConversionException: ";
  static std::string build_what(std::string msg, std::string msg2) {
    return msg + " " + msg2;
  }
public:
  ConversionException(std::string msg, std::string msg2 = "") :
    std::runtime_error(build_what(msg, msg2)) {
  }
};

template<typename TVariant, size_t I = std::variant_size_v<TVariant>-1>
struct GetVariant {
  static constexpr TVariant convertNode(
    const YAML::Node &node,
    decltype(std::variant_alternative_t<I, TVariant>::type) type
    ) {
    if (type == std::variant_alternative_t<I, TVariant>::type) {
      return node.as<std::variant_alternative_t<I, TVariant>>();
    }
    if constexpr (I == 0) {
      throw ConversionException("Converting node");
    } else {
      return GetVariant<TVariant, I - 1>::convertNode(node, type);
    }
  }
  static constexpr TVariant convertNodeByTypeString(const YAML::Node &node) {
    const std::string type = node["type"].as<std::string>();
    return convertNode(node, type);
    if (type == std::variant_alternative_t<I, TVariant>::type) {
      return node.as<std::variant_alternative_t<I, TVariant>>();
    }
    if constexpr (I == 0) {
      throw ConversionException("Converting node");
    } else {
      return GetVariant<TVariant, I - 1>::convertNodeByTypeString(node);
    }
  }
  static constexpr auto make(
    decltype(std::variant_alternative_t<I, TVariant>::type) type
    ) {
    if (type == std::variant_alternative_t<I, TVariant>::type) {
      return std::variant_alternative_t<I, TVariant>();
    }
    if constexpr (I == 0) {
      throw ConversionException("Converting node");
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
      throw ConversionException("Converting node");
    } else {
      return GetVariant<TVariant, I - 1>::make(type);
    }
  }

};

template<class ... Ts> struct overloaded : Ts ... { using Ts::operator () ...; };
template<class ... Ts> overloaded(Ts ...)->overloaded<Ts...>;
