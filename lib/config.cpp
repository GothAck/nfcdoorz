#include <iostream>
#include <iomanip>
#include <utility>
#include <tuple>
#include <string>

#include <string.h>

#include "config.hpp"
#include "logging.hpp"
#include "indent.hpp"


namespace nfcdoorz::config {
  using namespace std;
  using namespace nfcdoorz;
  widthstream out(80, cout);

  vector<string> decodePath;

  const string printDecodePath() {
    stringstream sb("");
    for (size_t i = 0; i < decodePath.size(); i++) {
      if (i) {
        sb << " > ";
      }
      sb << decodePath[i];
    }
    return sb.str();
  }

  Config Config::load(map<string, docopt::value> args) {
    return load(args["--config"].asString());
  }

  Config Config::load(string filename) {
    try {
      YAML::Node config = YAML::LoadFile(filename);
      return config.as<Config>();
    } catch (YAML::Exception e) {
      string path = printDecodePath();
      throw ValidationException(e.what(), path);
    } catch (ValidationException e) {
      string path = printDecodePath();
      throw ValidationException(e.what(), path);
    }
  }

  Config Config::parse(string content) {
    try {
      YAML::Node config = YAML::Load(content);
      return config.as<Config>();
    } catch (YAML::RepresentationException e) {
      string path = printDecodePath();
      throw ValidationException(e.what(), path);
    }
  }

  string Config::stringify() {
    YAML::Emitter out;
    out << encode();
    return out.c_str();
  }

  YAML::Node Key::encode() {
    YAML::Node node;
    node["id"] = (int) id;
    node["name"] = name;
    // node["data"] = vector<YAML::Node>();
    return node;
  }
  YAML::Node Key::encodeType(const std::string_view &type) {
    YAML::Node node = Key::encode();
    string strType;
    strType.resize(type.length());
    memcpy(strType.data(), type.data(), type.length());
    node["type"] = strType;
    return node;
  }
  void Key::decode(const YAML::Node &node) {
    if (node["id"]) CONVERT_NODE(id, int);
    if (node["name"]) CONVERT_NODE(name);
  }

  YAML::Node KeyDES::encode() {
    YAML::Node node = Key::encodeType(type);
    node["diversify"] = diversify;
    return node;
  }
  void KeyDES::decode(const YAML::Node &node) {
    CONVERT_NODE(diversify);
    nfcdoorz::config::decodePath.push_back(type.data());
    Key::decode(node);
    if (node["data"]) {
      nfcdoorz::config::decodePath.push_back("data");
      auto _data = node["data"].as<vector<int>>();
      if (_data.size() != data.size())
        throw ValidationException("data array length incorrect", to_string(data.size()));
      transform(_data.begin(), _data.end(), data.begin(), [](int c) -> uint8_t {
        return c;
      });
      nfcdoorz::config::decodePath.pop_back();
    }
    nfcdoorz::config::decodePath.pop_back();
  }

  YAML::Node Key3DES::encode() {
    YAML::Node node = Key::encodeType(type);
    node["diversify"] = diversify;
    return node;
  }
  void Key3DES::decode(const YAML::Node &node) {
    CONVERT_NODE(diversify);
    nfcdoorz::config::decodePath.push_back(type.data());
    Key::decode(node);
    if (node["data"]) {
      nfcdoorz::config::decodePath.push_back("data");
      auto _data = node["data"].as<vector<int>>();
      if (_data.size() != data.size())
        throw ValidationException("data array length incorrect", to_string(data.size()));
      transform(_data.begin(), _data.end(), data.begin(), [](int c) -> uint8_t {
        return c;
      });
      nfcdoorz::config::decodePath.pop_back();
    }
    nfcdoorz::config::decodePath.pop_back();
  }

  YAML::Node Key3k3DES::encode() {
    YAML::Node node = Key::encodeType(type);
    node["diversify"] = diversify;
    return node;
  }
  void Key3k3DES::decode(const YAML::Node &node) {
    CONVERT_NODE(diversify);
    nfcdoorz::config::decodePath.push_back(type.data());
    Key::decode(node);
    if (node["data"]) {
      nfcdoorz::config::decodePath.push_back("data");
      auto _data = node["data"].as<vector<int>>();
      if (_data.size() != data.size())
        throw ValidationException("data array length incorrect", to_string(data.size()));
      transform(_data.begin(), _data.end(), data.begin(), [](int c) -> uint8_t {
        return c;
      });
      nfcdoorz::config::decodePath.pop_back();
    }
    nfcdoorz::config::decodePath.pop_back();
  }

  YAML::Node KeyAES::encode() {
    YAML::Node node = Key::encodeType(type);
    node["diversify"] = diversify;
    return node;
  }
  void KeyAES::decode(const YAML::Node &node) {
    nfcdoorz::config::decodePath.push_back(type.data());
    CONVERT_NODE(diversify);
    Key::decode(node);
    if (node["data"]) {
      nfcdoorz::config::decodePath.push_back("data");
      auto _data = node["data"].as<vector<int>>();
      if (_data.size() != data.size())
        throw ValidationException("data array length incorrect", to_string(data.size()));
      transform(_data.begin(), _data.end(), data.begin(), [](int c) -> uint8_t {
        return c;
      });
      nfcdoorz::config::decodePath.pop_back();
    }
    nfcdoorz::config::decodePath.pop_back();
  }

  YAML::Node PICC::encode() {
    YAML::Node node;
    node["key"] = visit([](auto &key) {
      return key.encode();
    }, key);
    return node;
  }
  void PICC::decode(const YAML::Node &node) {
    key = GetVariant<decltype(key)>::convertNodeByTypeString(node["key"]);
  }

  YAML::Node AppSettings::encode() {
    YAML::Node node;
    node["accesskey"] = accesskey;
    node["frozen"] = frozen;
    node["req_auth_fileops"] = req_auth_fileops;
    node["req_auth_dir"] = req_auth_dir;
    node["allow_master_key_chg"] = allow_master_key_chg;
    return node;
  }
  void AppSettings::decode(const YAML::Node &node) {
    CONVERT_NODE(accesskey);
    CONVERT_NODE(frozen);
    CONVERT_NODE(req_auth_fileops);
    CONVERT_NODE(req_auth_dir);
    CONVERT_NODE(allow_master_key_chg);
  }

  YAML::Node AccessRights::encode() {
    YAML::Node node;
    node["read"] = (int) read;
    node["write"] = (int) write;
    node["read_write"] = (int) read_write;
    node["change_access_rights"] = (int) change_access_rights;
    return node;
  }
  void AccessRights::decode(const YAML::Node &node) {
    CONVERT_NODE(read, int);
    CONVERT_NODE(write, int);
    CONVERT_NODE(read_write, int);
    CONVERT_NODE(change_access_rights, int);
  }

  YAML::Node File::encode() {
    YAML::Node node;
    node["id"] = (int) id;
    node["name"] = name;
    node["communication_settings"] = typeFileCommSettingsToStr(communication_settings);
    node["access_rights"] = access_rights;
    return node;
  }
  YAML::Node File::encodeType(string_view type) {
    auto node = File::encode();
    string strType;
    strType.resize(type.length());
    memcpy(strType.data(), type.data(), type.length());
    node["type"] = strType;
    return node;
  }
  void File::decode(const YAML::Node &node) {
    CONVERT_NODE(id, int);
    CONVERT_NODE(name);

    communication_settings = typeStrToFileCommSettings(node["communication_settings"].as<string>());
    if (node["access_rights"]) CONVERT_NODE(access_rights);
  }

  YAML::Node FileStdData::encode() {
    YAML::Node node = File::encodeType(type);
    node["size"] = (int) size;
    return node;
  }
  void FileStdData::decode(const YAML::Node &node) {
    File::decode(node);
    CONVERT_NODE(size, int);
  }

  YAML::Node FileValue::encode() {
    YAML::Node node = File::encodeType(type);
    node["lower_limit"] = (int) lower_limit;
    node["upper_limit"] = (int) upper_limit;
    node["value"] = (int) value;
    node["limited_credit_enable"] = (int) limited_credit_enable;
    return node;
  }
  void FileValue::decode(const YAML::Node &node) {
    File::decode(node);
    CONVERT_NODE(lower_limit);
    CONVERT_NODE(upper_limit);
    CONVERT_NODE(value);
    CONVERT_NODE(limited_credit_enable, uint16_t);
  }

  YAML::Node FileLinearRecord::encode() {
    YAML::Node node = File::encodeType(type);
    node["record_size"] = (int) record_size;
    node["max_number_of_records"] = (int) max_number_of_records;
    return node;
  }
  void FileLinearRecord::decode(const YAML::Node &node) {
    File::decode(node);
    CONVERT_NODE(record_size, int);
    CONVERT_NODE(max_number_of_records, int);
  }

  YAML::Node FileCyclicRecord::encode() {
    YAML::Node node = File::encodeType(type);
    node["record_size"] = (int) record_size;
    node["max_number_of_records"] = (int) max_number_of_records;
    return node;
  }
  void FileCyclicRecord::decode(const YAML::Node &node) {
    File::decode(node);
    CONVERT_NODE(record_size, int);
    CONVERT_NODE(max_number_of_records, int);
  }

  YAML::Node FileBackupData::encode() {
    YAML::Node node = File::encodeType(type);
    node["size"] = (int) size;
    return node;
  }
  void FileBackupData::decode(const YAML::Node &node) {
    File::decode(node);
    CONVERT_NODE(size, int);
  }

  YAML::Node App::encode() {
    YAML::Node node;
    array<int, 3> int_aid;
    transform(aid.begin(), aid.end(), int_aid.begin(), [](auto c) -> int {
      return c;
    });
    node["aid"] = int_aid;
    vector<YAML::Node> keyNodes;
    keyNodes.resize(keys.size());
    transform(
      keys.begin(),
      keys.end(),
      keyNodes.begin(),
      [](auto &f) {
      return visit([](auto &f) {
        return f.encode();
      }, f);
    });
    node["name"] = name;
    node["settings"] = settings;
    node["keys"] = keyNodes;
    vector<YAML::Node> fileNodes;
    fileNodes.resize(files.size());
    transform(
      files.begin(),
      files.end(),
      fileNodes.begin(),
      [](auto &f) {
      return visit([](auto &f) {
        return f.encode();
      }, f);
    });
    node["files"] = fileNodes;
    return node;
  }
  void App::decode(const YAML::Node &node) {
    decodePath.push_back("aid");
    auto int_aid = node["aid"].as<array<int, 3>>();
    transform(int_aid.begin(), int_aid.end(), aid.begin(), [](int c) -> uint8_t {
      return c;
    });
    decodePath.pop_back();
    CONVERT_NODE(name);
    CONVERT_NODE(settings);
    CONVERT_ITER_VARIANT(keys, "keys")
    if (keys.size()) {
      auto key0 = keys[0].index();
      if (!all_of(keys.begin(), keys.end(), [key0](KeyType_t &key) {
        return key.index() == key0;
      })) {
        throw ValidationException("All keys in an application must have same type");
      }
    }
    CONVERT_ITER_VARIANT(files, "files");
  }

  YAML::Node Config::encode() {
    YAML::Node node;
    node["picc"] = picc;
    node["apps"] = apps;
    return node;
  }
  void Config::decode(const YAML::Node &node) {
    CONVERT_NODE(picc);
    CONVERT_ITER_NFC(apps, "apps", vector, App);
  }


  bool FileStdData::create(nfc::DESFireTagInterface &card) {
    LOG_VERBOSE
      << "FileStdData::create:" << endl
      << "  id: " << (int) id
      << "  name: " << name;
    return card.create_std_data_file(id, static_cast<uint8_t>(communication_settings), access_rights.get_lib_value(), size);
  }
  bool FileBackupData::create(nfc::DESFireTagInterface &card) {
    LOG_VERBOSE
      << "FileStdData::create:" << endl
      << "  id: " << (int) id
      << "  name: " << name;
    return card.create_std_data_file(id, static_cast<uint8_t>(communication_settings), access_rights.get_lib_value(), size);
  }
  bool FileLinearRecord::create(nfc::DESFireTagInterface &card) {
    LOG_VERBOSE
      << "FileStdData::create:" << endl
      << "  id: " << (int) id
      << "  name: " << name;
    return card.create_linear_record_file(id, static_cast<uint8_t>(communication_settings), access_rights.get_lib_value(), record_size, max_number_of_records);
  }
  bool FileCyclicRecord::create(nfc::DESFireTagInterface &card) {
    LOG_VERBOSE
      << "FileStdData::create:" << endl
      << "  id: " << (int) id
      << "  name: " << name;
    return card.create_cyclic_record_file(id, static_cast<uint8_t>(communication_settings), access_rights.get_lib_value(), record_size, max_number_of_records);
  }
  bool FileValue::create(nfc::DESFireTagInterface &card) {
    LOG_VERBOSE
      << "FileStdData::create:" << endl
      << "  id: " << (int) id
      << "  name: " << name;
    return card.create_value_file(id, static_cast<uint8_t>(communication_settings), access_rights.get_lib_value(), lower_limit, upper_limit, value, limited_credit_enable);
  }

  App::operator MifareDESFireAID() {
    return mifare_desfire_aid_new(aid[0] | (aid[1] << 8) | (aid[2] << 16));
  }

  ostream&operator <<(ostream &os, KeyDES &m) {
    os
      << "id: " << (int) m.id << endl;
    os
      << "type: " << m.type << endl
      << "size: " << (int) m.size << endl
      << "data: ";
    for (auto c: m.data)
      os
        << setfill('0')
        << setw(2)
        << hex
        << (int) c;

    os << dec << endl;
    return os;
  }
  ostream&operator <<(ostream &os, Key3DES &m) {
    os
      << "type: " << m.type << endl
      << "size: " << (int) m.size << endl
      << "data: ";
    for (auto c: m.data)
      os
        << setfill('0')
        << setw(2)
        << hex
        << (int) c;

    os << dec << endl;
    return os;
  }
  ostream&operator <<(ostream &os, Key3k3DES &m) {
    os
      << "type: " << m.type << endl
      << "size: " << (int) m.size << endl
      << "data: ";
    for (auto c: m.data)
      os
        << setfill('0')
        << setw(2)
        << hex
        << (int) c;

    os << dec << endl;
    return os;
  }
  ostream&operator <<(ostream &os, KeyAES &m) {
    os
      << "type: " << m.type << endl
      << "size: " << (int) m.size << endl
      << "data: ";
    for (auto c: m.data)
      os
        << setfill('0')
        << setw(2)
        << hex
        << (int) c;

    os << dec << endl;
    return os;
  }
  ostream&operator <<(ostream &os, PICC &m) {
    os
      << "key:" << endl;
    out.indent(2);

    visit([&os](auto &c) {
      os << c;
    }, m.key);
    out.indent(-2);
    return os;
  }
  ostream&operator <<(ostream &os, AppSettings &m) {
    return os
      << "accesskey: " << m.accesskey << endl
      << "frozen: " << m.frozen << endl
      << "req_auth_fileops: " << m.req_auth_fileops << endl
      << "req_auth_dir: " << m.req_auth_dir << endl
      << "allow_master_key_chg: " << m.allow_master_key_chg << endl;
  }
  ostream&operator <<(ostream &os, AccessRights &m) {
    return os
      << "read: " << m.read << endl
      << "write: " << m.write << endl
      << "read_write: " << m.read_write << endl
      << "change_access_rights: " << m.change_access_rights << endl;
    // return os
    // << "picc_master_key_changeable: " << m.picc_master_key_changeable << endl
    // << "free_listing_apps_and_key_settings: " << m.free_listing_apps_and_key_settings << endl
    // << "free_create_delete_application: " << m.free_create_delete_application << endl
    // << "picc_master_key_settings_changeable" << m.picc_master_key_settings_changeable << endl;
  }
  ostream&operator <<(ostream &os, File &m) {
    out.indent(2);
    os
      << "id: " << (int) m.id << endl
      << "name: " << m.name << endl;
    out.indent(-2);
    return os;
  }
  ostream&operator <<(ostream &os, FileStdData &m) {
    return os
      << "type: " << m.type << endl
      << "size: " << (int) m.size << endl;
  }
  ostream&operator <<(ostream &os, FileBackupData &m) {
    return os
      << "type: " << m.type << endl
      << "size: " << (int) m.size << endl;
  }
  ostream&operator <<(ostream &os, FileLinearRecord &m) {
    return os
      << "type: " << m.type << endl
      << "record_size: " << m.record_size << endl
      << "max_number_of_records: " << m.max_number_of_records << endl;
  }
  ostream&operator <<(ostream &os, FileCyclicRecord &m) {
    return os
      << "type: " << m.type << endl
      << "record_size: " << m.record_size << endl
      << "max_number_of_records: " << m.max_number_of_records << endl;
  }
  ostream&operator <<(ostream &os, FileValue &m) {
    return os
      << "type: " << m.type << endl
      << "lower_limit: " << m.lower_limit << endl
      << "upper_limit: " << m.upper_limit << endl
      << "value: " << m.value << endl
      << "limited_credit_enable: " << m.limited_credit_enable << endl;
  }
  ostream&operator <<(ostream &os, App &m) {
    os
      << "App" << endl;
    out.indent(2);
    os
      << "files:" << endl;
    out.indent(2);
    for (auto &f: m.files) visit([&os](auto &c) {
        os << c;
      }, f);
    out.indent(-2);
    os
      << "keys:" << endl;
    out.indent(2);
    for (auto &f: m.keys) visit([&os](auto &c) {
        os << c;
      }, f);
    out.indent(-4);
    return os;
  }
  ostream&operator <<(ostream &os, Config &m) {
    out.indent(0)
      << "Config" << endl;
    out.indent(2)
      << "picc:" << endl;
    out.indent(2)
      << m.picc;
    out.indent(-2)
      << "apps:" << endl;
    out.indent(2);
    for (auto &app: m.apps) {
      out << app;
    }
    return os;
  }
}
