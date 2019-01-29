#include <iostream>
#include <cstring>

#include <freefare.h>
#include "tag.hpp"
#include "nfc.hpp"

namespace nfcdoorz::nfc {
using namespace std;

struct mifare_desfire_aid {
    uint8_t data[3];
};

DESFireTagInterface::DESFireTagInterface(Tag &tag) {
  _tag = tag;
  _connected = false;
}

void DESFireTagInterface::setStoredUID(UID_t uid) {
  _uid = uid;
}
UID_t DESFireTagInterface::getStoredUID() {
  return _uid;
}

bool DESFireTagInterface::connect() {
  if (_connected)
    return true;
  if (mifare_desfire_connect(_tag) < 0) {
    return false;
  }
  _connected = true;
  return true;
}

bool DESFireTagInterface::authenticate(uint8_t key_id, MifareDESFireKey key) {
  if (!_connected)
    return false;
  if (mifare_desfire_authenticate(_tag, key_id, key) < 0)
    return false;
  return true;
}

bool DESFireTagInterface::format() {
  if (!_connected)
    return false;
  if (mifare_desfire_format_picc(_tag) < 0)
    return false;
  return true;
}

vector<AppID_t> DESFireTagInterface::get_application_ids() {
  vector<AppID_t> ret;
  if (!_connected)
    return ret;
  MifareDESFireAID *app_ids;
  size_t count;
  if (mifare_desfire_get_application_ids(_tag, &app_ids, &count) < 0)
    return ret;

  for (size_t i = 0; i < count; i++) {
    AppID_t aid;
    memcpy(aid.data(), app_ids[i], 3);
    ret.push_back(aid);
  }

  free(app_ids);

  return ret;
}

bool DESFireTagInterface::set_configuration(bool disable_format, bool enable_random_uid) {
  if (!_connected)
    return false;
  if (mifare_desfire_set_configuration(_tag, disable_format, enable_random_uid) < 0)
    return false;
  return true;
}

char *DESFireTagInterface::get_uid() {
  if (!_connected)
    return nullptr;
  return freefare_get_tag_uid(_tag);
}

bool DESFireTagInterface::change_key(uint8_t key_id, MifareDESFireKey new_key, MifareDESFireKey old_key) {
  if (!_connected)
    return false;
  if (mifare_desfire_change_key(_tag, key_id, new_key, old_key) < 0)
    return false;
  return true;
}

bool DESFireTagInterface::set_default_key(MifareDESFireKey key) {
  if (!_connected)
    return false;
  if (mifare_desfire_set_default_key(_tag, key) < 0)
    return false;
  return true;
}

bool DESFireTagInterface::delete_application(MifareDESFireAID aid) {
  if (!_connected)
    return false;
  if (mifare_desfire_delete_application(_tag, aid) < 0)
    return false;
  return true;
}

bool DESFireTagInterface::create_application_aes(MifareDESFireAID aid, uint8_t settings, uint8_t numKeys) {
  if (!_connected)
    return false;
  if (mifare_desfire_create_application_aes(_tag, aid, settings, numKeys) < 0)
    return false;
  return true;
}

bool DESFireTagInterface::select_application(MifareDESFireAID aid) {
  if (!_connected)
    return false;
  if (mifare_desfire_select_application(_tag, aid) < 0)
    return false;
  return true;
}

bool DESFireTagInterface::create_std_data_file(uint8_t file_no, uint8_t communication_settings, uint16_t access_rights, uint32_t file_size) {
  if (!_connected)
    return false;
  if (mifare_desfire_create_std_data_file(_tag, file_no, communication_settings, access_rights, file_size) < 0)
    return false;
  return true;
}

bool DESFireTagInterface::create_backup_data_file(uint8_t file_no, uint8_t communication_settings, uint16_t access_rights, uint32_t file_size) {
  if (!_connected)
    return false;
  if (mifare_desfire_create_backup_data_file(_tag, file_no, communication_settings, access_rights, file_size) < 0)
    return false;
  return true;
}
bool DESFireTagInterface::create_value_file(uint8_t file_no, uint8_t communication_settings, uint16_t access_rights, int32_t lower_limit, int32_t upper_limit, int32_t value, uint8_t limited_credit_enable) {
  if (!_connected)
    return false;
  if (mifare_desfire_create_value_file(_tag, file_no, communication_settings, access_rights, lower_limit, upper_limit, value, limited_credit_enable) < 0)
    return false;
  return true;
}
bool DESFireTagInterface::create_linear_record_file(uint8_t file_no, uint8_t communication_settings, uint16_t access_rights, uint32_t record_size, uint32_t max_number_of_records) {
  if (!_connected)
    return false;
  if (mifare_desfire_create_linear_record_file(_tag, file_no, communication_settings, access_rights, record_size, max_number_of_records) < 0)
    return false;
  return true;
}
bool DESFireTagInterface::create_cyclic_record_file(uint8_t file_no, uint8_t communication_settings, uint16_t access_rights, uint32_t record_size, uint32_t max_number_of_records) {
  if (!_connected)
    return false;
  if (mifare_desfire_create_cyclic_record_file(_tag, file_no, communication_settings, access_rights, record_size, max_number_of_records) < 0)
    return false;
  return true;
}

bool DESFireTagInterface::disconnect() {
  if (!_connected)
    return false;
  _connected = false;
  if (mifare_desfire_disconnect(_tag) < 0)
    return false;
  return true;
}
}
