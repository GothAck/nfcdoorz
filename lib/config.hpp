#pragma once

#include <docopt/docopt.h>
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
#include "types.hpp"
#include "nfc.hpp"

namespace nfcdoorz::config {
  using namespace nfcdoorz;

#define OSTREAM(CLASS) std::ostream &operator <<(std::ostream &os, CLASS & m)
#define STR2(s) # s
#define STR(s) STR2(s)
#define __CONVERT_DEF(NAME, TYPE, ...) NAME = node[STR(NAME)].as<TYPE>()
#define CONVERT_NODE(NAME, ...) __CONVERT_DEF(NAME, ## __VA_ARGS__, decltype(NAME))
#define CONVERT_ITER_NFC(DEST, NODE, ITERTYPE, TYPE) \
  decodePath.push_back(NODE); \
  DEST = node[NODE].as<std::ITERTYPE<TYPE>>(); \
  decodePath.pop_back();
#define CONVERT_ITER_VARIANT(DEST, NODE) \
  decodePath.push_back(NODE); \
  if (!node[NODE].IsSequence()) throw ValidationException("Expected a sequence"); \
  DEST.resize(node[NODE].size()); \
  std::transform( \
    node[NODE].begin(), node[NODE].end(), \
    DEST.begin(), \
    [](const YAML::Node &node) -> auto { return GetVariant<decltype(DEST)::value_type>::convertNodeByTypeString(node); }); \
  decodePath.pop_back();


  extern std::vector<std::string> decodePath;
  const std::string printDecodePath();

  class ValidationException : public std::runtime_error {
    constexpr static const char *prefix = "ValidationException: ";
    static std::string build_what(std::string msg, std::string msg2) {
      return msg + " " + msg2;
    }
public:
    ValidationException(std::string msg, std::string msg2 = "") :
      std::runtime_error(build_what(msg, msg2)) {
    }
  };

  struct Node {
    virtual void decode(const YAML::Node&) = 0;
    virtual YAML::Node encode() = 0;
  };

  struct Key : Node {
    constexpr static const char *node_name = "key";
    uint8_t id = 0;
    std::string name;

    virtual uint8_t *begin() = 0;
    virtual uint8_t *end() = 0;
    virtual YAML::Node encode() override;
    virtual YAML::Node encodeType(const std::string_view &type);
    virtual void decode(const YAML::Node &node) override;

  };

  struct KeyDES : Key, nfc::KeyDES {
    uint8_t *begin() override {
      return data.begin();
    };
    uint8_t *end() override {
      return data.end();
    };
    virtual YAML::Node encode() override;
    virtual void decode(const YAML::Node &node) override;
  };

  struct Key3DES : Key, nfc::Key3DES {
    uint8_t *begin() override {
      return data.begin();
    };
    uint8_t *end() override {
      return data.end();
    };
    virtual YAML::Node encode() override;
    virtual void decode(const YAML::Node &node) override;
  };

  struct Key3k3DES : Key, nfc::Key3k3DES {
    uint8_t *begin() override {
      return data.begin();
    };
    uint8_t *end() override {
      return data.end();
    };
    virtual YAML::Node encode() override;
    virtual void decode(const YAML::Node &node) override;
  };

  struct KeyAES : Key, nfc::KeyAES {
    uint8_t *begin() override {
      return data.begin();
    };
    uint8_t *end() override {
      return data.end();
    };
    virtual YAML::Node encode() override;
    virtual void decode(const YAML::Node &node) override;
  };

  typedef std::variant<KeyDES, Key3DES, Key3k3DES, KeyAES> KeyType_t;
  OSTREAM(KeyDES);
  OSTREAM(Key3DES);
  OSTREAM(Key3k3DES);
  OSTREAM(KeyAES);

  struct PICC : Node {
    constexpr static const char *node_name = "picc";
    virtual YAML::Node encode() override;
    virtual void decode(const YAML::Node &node) override;
    KeyType_t key;
  };
  OSTREAM(PICC);

  struct AppSettings : Node {
    constexpr static const char *node_name = "settings";
    virtual YAML::Node encode() override;
    virtual void decode(const YAML::Node &node) override;
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
    virtual YAML::Node encode() override;
    virtual void decode(const YAML::Node &node) override;
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
    enum class FileCommSettings : uint8_t {
      PLAIN = MDCM_PLAIN,
      MACED = MDCM_MACED,
      ENCIPHERED = MDCM_ENCIPHERED,
    };


    uint8_t id;
    std::string name;
    FileCommSettings communication_settings = FileCommSettings::PLAIN;
    AccessRights access_rights;

    static FileCommSettings typeStrToFileCommSettings(std::string type) {
      transform(type.begin(), type.end(), type.begin(), ::tolower);
      if (type.compare("plain") == 0)
        return FileCommSettings::PLAIN;
      if (type.compare("maced") == 0)
        return FileCommSettings::MACED;
      if (type.compare("enciphered") == 0)
        return FileCommSettings::ENCIPHERED;
      return FileCommSettings::PLAIN;
    };
    static std::string typeFileCommSettingsToStr(const FileCommSettings &type) {
      switch (type) {
      case FileCommSettings::PLAIN:
        return "plain";
      case FileCommSettings::MACED:
        return "maced";
      case FileCommSettings::ENCIPHERED:
        return "enciphered";
      }
      return "unknown";
    };

    virtual YAML::Node encode() override;
    virtual YAML::Node encodeType(std::string_view type);
    virtual void decode(const YAML::Node &node) override;
  };

  struct FileStdData : File {
    constexpr static const std::string_view type = "std_data";
    uint8_t size = 0;

    virtual YAML::Node encode() override;
    virtual void decode(const YAML::Node &node) override;

    bool create(nfc::DESFireTagInterface &card);
  };

  struct FileValue : File {
    constexpr static const std::string_view type = "value";
    uint32_t lower_limit;
    uint32_t upper_limit;
    uint32_t value;
    uint8_t limited_credit_enable;

    virtual YAML::Node encode() override;
    virtual void decode(const YAML::Node &node) override;

    bool create(nfc::DESFireTagInterface &card);
  };

  struct FileLinearRecord : File {
    constexpr static const std::string_view type = "linear_record";
    uint32_t record_size;
    uint32_t max_number_of_records;

    virtual YAML::Node encode() override;
    virtual void decode(const YAML::Node &node) override;

    bool create(nfc::DESFireTagInterface &card);
  };

  struct FileCyclicRecord : File {
    constexpr static const std::string_view type = "cyclic_record";
    uint32_t record_size;
    uint32_t max_number_of_records;

    virtual YAML::Node encode() override;
    virtual void decode(const YAML::Node &node) override;
    bool create(nfc::DESFireTagInterface &card);
  };

  struct FileBackupData : File {
    constexpr static const std::string_view type = "backup_data";
    uint32_t size;

    virtual YAML::Node encode() override;
    virtual void decode(const YAML::Node &node) override;
    bool create(nfc::DESFireTagInterface &card);
  };

  typedef std::variant<
    FileStdData, FileValue, FileLinearRecord, FileCyclicRecord, FileBackupData
    > FileType_t;

  OSTREAM(FileStdData);
  OSTREAM(FileBackupData);
  OSTREAM(FileLinearRecord);
  OSTREAM(FileCyclicRecord);
  OSTREAM(FileValue);

  struct App : Node {
    constexpr static const char *node_name = "app";
    virtual YAML::Node encode() override;
    virtual void decode(const YAML::Node &node) override;
    operator MifareDESFireAID();
    AppID_t aid = { 0, 0, 0 };
    std::string name;
    AppSettings settings;
    std::vector<KeyType_t> keys;
    std::vector<FileType_t> files;
  };
  OSTREAM(App);

  struct Config : Node {
    virtual YAML::Node encode() override;
    virtual void decode(const YAML::Node &node) override;
    constexpr static const char *node_name = "config";
    PICC picc;
    std::vector<App> apps;

    static Config load(std::map<std::string, docopt::value> args);
    static Config load(std::string filename);
    static Config parse(std::string content);
    std::string stringify();
  };
  OSTREAM(Config);
}

namespace YAML {
  #define Y_CONVERT(CLASS) template<> \
  struct convert<nfcdoorz::config::CLASS> { \
    static bool decode(const Node &node, nfcdoorz::config::CLASS &rhs) { \
      nfcdoorz::config::decodePath.push_back(nfcdoorz::config::CLASS::node_name); \
      rhs.decode(node); \
      nfcdoorz::config::decodePath.pop_back(); \
      return true; \
    } \
    static YAML::Node encode(nfcdoorz::config::CLASS rhs) { \
      nfcdoorz::config::decodePath.push_back(nfcdoorz::config::CLASS::node_name); \
      auto node = rhs.encode(); \
      nfcdoorz::config::decodePath.pop_back(); \
      return node; \
    } \
  }
  Y_CONVERT(AccessRights);
  Y_CONVERT(FileStdData);
  Y_CONVERT(FileBackupData);
  Y_CONVERT(FileValue);
  Y_CONVERT(FileLinearRecord);
  Y_CONVERT(FileCyclicRecord);
  Y_CONVERT(File);
  Y_CONVERT(KeyDES);
  Y_CONVERT(Key3DES);
  Y_CONVERT(Key3k3DES);
  Y_CONVERT(KeyAES);
  Y_CONVERT(PICC);
  Y_CONVERT(AppSettings);
  Y_CONVERT(App);
  Y_CONVERT(Config);
}
