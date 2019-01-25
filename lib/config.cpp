#include <iostream>
#include <iomanip>
#include <utility>
#include <tuple>

#include <string.h>

#include "config.hpp"
#include "indent.hpp"


namespace nfcdoorz::config {
  using namespace std;
  using namespace nfcdoorz;
  widthstream out(80, cout);

  std::vector<std::string> decodePath;

  const string printDecodePath() {
    stringstream sb("");
    for (size_t i = 0; i < decodePath.size(); i++) {
      if (i) sb << " > ";
      sb << decodePath[i];
    }
    return sb.str();
  }

  Config Config::load(string filename) {
    try {
      YAML::Node config = YAML::LoadFile(filename);
      return config.as<Config>();
    } catch (YAML::RepresentationException e) {
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

  bool FileStdData::create(nfc::DESFireTagInterface &card, FileStdData &file) {
    return card.create_std_data_file(file.id, static_cast<uint8_t>(file.communication_settings), file.access_rights.get_lib_value(), file.size);
  }
  bool FileBackupData::create(nfc::DESFireTagInterface &card, FileBackupData &file) { return true; }
  bool FileLinearRecord::create(nfc::DESFireTagInterface &card, FileLinearRecord &file) { return true; }
  bool FileCyclicRecord::create(nfc::DESFireTagInterface &card, FileCyclicRecord &file) { return true; }
  bool FileValue::create(nfc::DESFireTagInterface &card, FileValue &file) { return true; }

  App::operator MifareDESFireAID() {
    return mifare_desfire_aid_new(aid[0] | (aid[1] << 8) | (aid[2] << 16));
  }

  ostream &operator <<(ostream &os, KeyDES &m) {
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
  ostream &operator <<(ostream &os, Key3DES &m) {
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
  ostream &operator <<(ostream &os, Key3k3DES &m) {
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
  ostream &operator <<(ostream &os, KeyAES &m) {
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
  ostream &operator <<(ostream &os, PICC &m) {
    os
      << "key:" << endl;
    out.indent(2);

    visit([&os](auto &c){ os << c; }, m.key);
    out.indent(-2);
      // << "settings:" << endl << m.settings
    return os;
  }
  ostream &operator <<(ostream &os, AppSettings &m) {
    return os
      << "accesskey: " << m.accesskey << endl
      << "frozen: " << m.frozen << endl
      << "req_auth_fileops: " << m.req_auth_fileops << endl
      << "req_auth_dir: " << m.req_auth_dir << endl
      << "allow_master_key_chg: " << m.allow_master_key_chg << endl;
  }
  ostream &operator <<(ostream &os, AccessRights &m) {
    return os
      << "read: " << m.read << endl
      << "write: " << m.write << endl
      << "read_write: " << m.read_write << endl
      << "change_access_rights: " << m.change_access_rights << endl;
    // return os
    //   << "picc_master_key_changeable: " << m.picc_master_key_changeable << endl
    //   << "free_listing_apps_and_key_settings: " << m.free_listing_apps_and_key_settings << endl
    //   << "free_create_delete_application: " << m.free_create_delete_application << endl
    //   << "picc_master_key_settings_changeable" << m.picc_master_key_settings_changeable << endl;
  }
  ostream &operator <<(ostream &os, File &m) {
    out.indent(2);
    os
      << "id: " << (int) m.id << endl
      << "name: " << m.name << endl;
    out.indent(-2);
    return os;
  }
  ostream &operator <<(ostream &os, FileStdData &m) {
    return os
      << "type: " << m.type << endl
      << "size: " << (int) m.size << endl;
  }
  ostream &operator <<(ostream &os, FileBackupData &m) {
    return os
      << "type: " << m.type << endl
      << "size: " << (int) m.size << endl;
  }
  ostream &operator <<(ostream &os, FileLinearRecord &m) {
    return os
      << "type: " << m.type << endl
      << "record_size: " << m.record_size << endl
      << "max_number_of_records: " << m.max_number_of_records << endl;
  }
  ostream &operator <<(ostream &os, FileCyclicRecord &m) {
    return os
      << "type: " << m.type << endl
      << "record_size: " << m.record_size << endl
      << "max_number_of_records: " << m.max_number_of_records << endl;
  }
  ostream &operator <<(ostream &os, FileValue &m) {
    return os
      << "type: " << m.type << endl
      << "lower_limit: " << m.lower_limit << endl
      << "upper_limit: " << m.upper_limit << endl
      << "value: " << m.value << endl
      << "limited_credit_enable: " << m.limited_credit_enable << endl;
  }
  ostream &operator <<(ostream &os, App &m) {
    os
      << "App" << endl;
      out.indent(2);
    os
      << "files:" << endl;
    out.indent(2);
    for(auto &f: m.files) visit([&os](auto &c){ os << c; }, f);
    out.indent(-2);
    os
      << "keys:" << endl;
    out.indent(2);
    for(auto &f: m.keys) visit([&os](auto &c){ os << c; }, f);
    out.indent(-4);
    return os;
  }
  ostream &operator <<(ostream &os, Config &m) {
    out.indent(0)
      << "Config" << endl;
    out.indent(2)
      << "picc:" << endl;
    out.indent(2)
      << m.picc;
    out.indent(-2)
      << "apps:" << endl;
      out.indent(2);
    for (auto &app: m.apps) out << app;
    return os;
  }
}
