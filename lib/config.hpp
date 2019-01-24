#pragma once

#include <yaml-cpp/yaml.h>
#include <iostream>
#include <memory>
#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <variant>
#include <utility>
#include <stdexcept>
#include <cstdarg>

#include "keys.hpp"
#include "tag.hpp"
#include "types.h"
#include "nfc.hpp"

namespace nfcdoorz::config {
  using namespace nfcdoorz;

#define OSTREAM(CLASS) std::ostream &operator <<(std::ostream &os, CLASS &m)
#define STR2(s) #s
#define STR(s) STR2(s)
#define __CONVERT_DEF(NAME, TYPE, ...) NAME = node[STR(NAME)].as<TYPE>()
#define CONVERT_NODE(NAME, ...) __CONVERT_DEF(NAME, ##__VA_ARGS__, decltype(NAME))
#define CONVERT_ITER_NFC(DEST, NODE, ITERTYPE, TYPE) \
  decodePath.push_back(NODE); \
  DEST = node[NODE].as<std::ITERTYPE<TYPE>>(); \
  decodePath.pop_back();

extern std::vector<std::string> decodePath;
const std::string printDecodePath();

class ValidationException : public std::runtime_error {
  constexpr static const char *prefix = "ValidationException: ";
  static std::string build_what(std::string msg, std::string msg2) {
    return msg + " " + msg2;
  }
public:
  ValidationException(std::string msg, std::string msg2 = ""):
    std::runtime_error(build_what(msg, msg2)) {}
};

struct Node {
  virtual void decode(const YAML::Node &) = 0;
};

struct Key : Node {
  constexpr static const char *node_name = "key";

  struct KeyType : Node {
    virtual uint8_t* begin() = 0;
    virtual uint8_t* end() = 0;
  };
  struct KeyDES : KeyType, nfc::KeyDES {
    constexpr static const char *node_name = "(des)";
    uint8_t* begin() override { return data.begin(); };
    uint8_t* end() override { return data.end(); };
    void decode(const YAML::Node &node) {
      nfcdoorz::config::decodePath.push_back("data");
      auto _data = node["data"].as<std::vector<int>>();
      if (_data.size() != data.size())
        throw ValidationException("data array length incorrect", std::to_string(data.size()));
      std::transform(_data.begin(), _data.end(), data.begin(), [](int c) -> uint8_t { return c; });
      nfcdoorz::config::decodePath.pop_back();
    }
  };
  struct Key3DES : KeyType, nfc::Key3DES {
    constexpr static const char *node_name = "(3des)";
    uint8_t* begin() override { return data.begin(); };
    uint8_t* end() override { return data.end(); };
    void decode(const YAML::Node &node) {
      nfcdoorz::config::decodePath.push_back("data");
      auto _data = node["data"].as<std::vector<int>>();
      if (_data.size() != data.size())
        throw ValidationException("data array length incorrect", std::to_string(data.size()));
      std::transform(_data.begin(), _data.end(), data.begin(), [](int c) -> uint8_t { return c; });
      nfcdoorz::config::decodePath.pop_back();
    }
  };

  struct Key3k3DES : KeyType, nfc::Key3k3DES {
    constexpr static const char *node_name = "(3k3des)";
    uint8_t* begin() override { return data.begin(); };
    uint8_t* end() override { return data.end(); };
    void decode(const YAML::Node &node) {
      nfcdoorz::config::decodePath.push_back("data");
      auto _data = node["data"].as<std::vector<int>>();
      if (_data.size() != data.size())
        throw ValidationException("data array length incorrect", std::to_string(data.size()));
      std::transform(_data.begin(), _data.end(), data.begin(), [](int c) -> uint8_t { return c; });
      nfcdoorz::config::decodePath.pop_back();
    }
  };

  struct KeyAES : KeyType, nfc::KeyAES {
    constexpr static const char *node_name = "(aes)";
    uint8_t* begin() override { return data.begin(); };
    uint8_t* end() override { return data.end(); };
    void decode(const YAML::Node &node) {
      nfcdoorz::config::decodePath.push_back("data");
      auto _data = node["data"].as<std::vector<int>>();
      if (_data.size() != data.size())
        throw ValidationException("data array length incorrect", std::to_string(data.size()));
      std::transform(_data.begin(), _data.end(), data.begin(), [](int c) -> uint8_t { return c; });
      nfcdoorz::config::decodePath.pop_back();
    }
  };

  void decode(const YAML::Node &node) {
    if (node["id"]) CONVERT_NODE(id, int);
    if (node["name"]) CONVERT_NODE(name);
    auto type = node["type"].as<std::string>();
    key = GetVariant<decltype(key)>::convertNode(node, type);
    CONVERT_NODE(diversify);
  }
  uint8_t id = 0;
  std::string name;
  bool diversify = false;
  typedef std::variant<KeyDES, Key3DES, Key3k3DES, KeyAES> KeyType_t;
  KeyType_t key;

  operator MifareDESFireKey();
};
OSTREAM(Key);
OSTREAM(Key::KeyDES);
OSTREAM(Key::Key3DES);
OSTREAM(Key::Key3k3DES);
OSTREAM(Key::KeyAES);

struct PICC : Node {
  constexpr static const char *node_name = "picc";
  void decode(const YAML::Node &node) {
    CONVERT_NODE(key);
  }
  Key key;
};
OSTREAM(PICC);

struct AppSettings : Node {
  constexpr static const char *node_name = "settings";
  void decode(const YAML::Node &node) {
     CONVERT_NODE(accesskey);
     CONVERT_NODE(frozen);
     CONVERT_NODE(req_auth_fileops);
     CONVERT_NODE(req_auth_dir);
     CONVERT_NODE(allow_master_key_chg);
  }
  uint8_t get_lib_value() {
    uint8_t ret = 0;
    ret |= accesskey << 4;
    ret |= (!!frozen) << 3;
    ret |= (!!req_auth_fileops) << 2;
    ret |= (!!req_auth_dir) << 1;
    ret |= (!!allow_master_key_chg) << 0;
    return ret;
  }
  uint8_t accesskey;
  bool frozen;
  bool req_auth_fileops;
  bool req_auth_dir;
  bool allow_master_key_chg;
};
OSTREAM(AppSettings);

struct AccessRights : Node {
  constexpr static const char *node_name = "access_rights";
  void decode(const YAML::Node &node) {
    CONVERT_NODE(read, int);
    CONVERT_NODE(write, int);
    CONVERT_NODE(read_write, int);
    CONVERT_NODE(change_access_rights, int);
  }
  uint8_t read;
  uint8_t write;
  uint8_t read_write;
  uint8_t change_access_rights;

  uint16_t get_lib_value() {
    return
      (read << 12) |
      (write << 8) |
      (read_write << 4) |
      (change_access_rights);
  }
};
OSTREAM(AccessRights);

struct File : Node {
  constexpr static const char *node_name = "file";
  struct FileType : Node {
    virtual bool create(nfcdoorz::nfc::DESFireTagInterface &card, File &file) = 0;
  };
  struct StdData : FileType {
    constexpr static const char *node_name = "(std_data)";
    constexpr static const std::string_view type = "std_data";
    void decode(const YAML::Node &node) {
      CONVERT_NODE(size, int);
    }
    uint8_t size = 0;
    bool create(nfcdoorz::nfc::DESFireTagInterface &card, File &file) override;
  };
  struct Value : FileType {
    constexpr static const char *node_name = "(value)";
    constexpr static const std::string_view type = "value";
    void decode(const YAML::Node &node) {
      CONVERT_NODE(lower_limit);
      CONVERT_NODE(upper_limit);
      CONVERT_NODE(value);
      CONVERT_NODE(limited_credit_enable);
    }
    uint32_t lower_limit;
    uint32_t upper_limit;
    uint32_t value;
    uint32_t limited_credit_enable;
    bool create(nfcdoorz::nfc::DESFireTagInterface &card, File &file) override;
  };
  struct LinearRecord : FileType {
    constexpr static const char *node_name = "(linear_record)";
    constexpr static const std::string_view type = "linear_record";
    void decode(const YAML::Node &node) {
      CONVERT_NODE(record_size, int);
      CONVERT_NODE(max_number_of_records, int);
    }
    uint32_t record_size;
    uint32_t max_number_of_records;
    bool create(nfcdoorz::nfc::DESFireTagInterface &card, File &file) override;
  };
  struct CyclicRecord : FileType {
    constexpr static const char *node_name = "(cyclic_record)";
    constexpr static const std::string_view type = "cyclic_record";
    void decode(const YAML::Node &node) {
      CONVERT_NODE(record_size, int);
      CONVERT_NODE(max_number_of_records, int);
    }
    uint32_t record_size;
    uint32_t max_number_of_records;
    bool create(nfcdoorz::nfc::DESFireTagInterface &card, File &file) override;
  };
  struct BackupData : FileType {
    constexpr static const char *node_name = "(backup_data)";
    constexpr static const std::string_view type = "backup_data";
    void decode(const YAML::Node &node) {
      CONVERT_NODE(size, int);
    }
    uint32_t size;
    bool create(nfcdoorz::nfc::DESFireTagInterface &card, File &file) override;
  };

  enum class FileCommSettings : uint8_t {
    PLAIN = MDCM_PLAIN,
    MACED = MDCM_MACED,
    ENCIPHERED = MDCM_ENCIPHERED,
  };

  void decode(const YAML::Node &node) {
    CONVERT_NODE(id, int);
    CONVERT_NODE(name);
    auto type = node["type"].as<std::string>();
    config = GetVariant<decltype(config)>::convertNode(node, type);
    communication_settings = typeStrToFileCommSettings(node["communication_settings"].as<std::string>());
    if (node["access_rights"]) {
      CONVERT_NODE(access_rights);
    }
  }
  uint8_t id;
  std::string name;
  FileCommSettings communication_settings = FileCommSettings::PLAIN;
  AccessRights access_rights;
  typedef std::variant<StdData, Value, LinearRecord, CyclicRecord, BackupData> FileConfig_t;
  FileConfig_t config;

  static FileCommSettings typeStrToFileCommSettings(std::string type) {
    transform (type.begin(), type.end(), type.begin(), ::tolower);
    if (type.compare("plain") == 0)
      return FileCommSettings::PLAIN;
    if (type.compare("maced") == 0)
      return FileCommSettings::MACED;
    if (type.compare("enciphered") == 0)
      return FileCommSettings::ENCIPHERED;
    return FileCommSettings::PLAIN;
  };
};
OSTREAM(File);
OSTREAM(File::StdData);
OSTREAM(File::BackupData);
OSTREAM(File::LinearRecord);
OSTREAM(File::CyclicRecord);
OSTREAM(File::Value);

struct App : Node {
  constexpr static const char *node_name = "app";
  void decode(const YAML::Node &node) {
    decodePath.push_back("aid");
    auto int_aid = node["aid"].as<std::array<int, 3>>();
    std::transform(int_aid.begin(), int_aid.end(), aid.begin(), [](int c) -> uint8_t { return c; });
    decodePath.pop_back();
    CONVERT_NODE(name);
    CONVERT_NODE(settings);
    CONVERT_ITER_NFC(keys, "keys", vector, Key);
    if (keys.size()) {
      auto key0 = keys[0].key.index();
      if (!all_of(keys.begin(), keys.end(), [key0](Key key){ return key.key.index(); })) {
        throw ValidationException("All keys in an application must have same type");
      }
    }
    CONVERT_ITER_NFC(files, "files", vector, File);
  }
  operator MifareDESFireAID();
  AppID_t aid = {0, 0, 0};
  std::string name;
  AppSettings settings;
  std::vector<Key> keys;
  std::vector<File> files;
};
OSTREAM(App);

struct Config : Node {
  void decode(const YAML::Node &node) {
     CONVERT_NODE(picc);
     CONVERT_ITER_NFC(apps, "apps", vector, App);
  }
  constexpr static const char *node_name = "config";
  PICC picc;
  std::vector<App> apps;

  static Config load(std::string filename);
  static Config parse(std::string content);
};
OSTREAM(Config);
}

namespace YAML {
  #define Y_CONVERT(CLASS) template <> \
    struct convert<nfcdoorz::config::CLASS> { \
      static bool decode(const Node& node, nfcdoorz::config::CLASS &rhs) { \
        nfcdoorz::config::decodePath.push_back(nfcdoorz::config::CLASS::node_name); \
        rhs.decode(node); \
        nfcdoorz::config::decodePath.pop_back(); \
        return true; \
      } \
    }
  Y_CONVERT(AccessRights);
  Y_CONVERT(File::StdData);
  Y_CONVERT(File::BackupData);
  Y_CONVERT(File::Value);
  Y_CONVERT(File::LinearRecord);
  Y_CONVERT(File::CyclicRecord);
  Y_CONVERT(File);
  Y_CONVERT(Key);
  Y_CONVERT(Key::KeyDES);
  Y_CONVERT(Key::Key3DES);
  Y_CONVERT(Key::Key3k3DES);
  Y_CONVERT(Key::KeyAES);
  Y_CONVERT(PICC);
  Y_CONVERT(AppSettings);
  Y_CONVERT(App);
  Y_CONVERT(Config);
}
