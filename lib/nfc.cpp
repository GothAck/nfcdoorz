#include <iostream>

#include "nfc.hpp"

namespace nfcdoorz::nfc {
  using namespace std;

  Context::~Context() {
    if (_context) nfc_exit(_context);
  }

  bool Context::init() {
    if (_context != nullptr) return true;
    nfc_init(&_context);
    return _context != nullptr;
  }

  vector<Device> Context::getDevices() {
    vector<Device> ret;
    if (_context == nullptr) return ret;
    nfc_connstring devices[32];
    int device_count;

    device_count = nfc_list_devices(_context, devices, 32);
    for (uint8_t i = 0; i < device_count; i++) {
      ret.emplace_back(*this, devices[i]);
    }
    return ret;
  }

  optional<Device> Context::getDeviceMatching(string suffix) {
    optional<Device> ret;
    if (_context == nullptr) return ret;
    nfc_connstring devices[32];
    int device_count;

    device_count = nfc_list_devices(_context, devices, 32);
    for (uint8_t i = 0; i < device_count; i++) {
      string device_string = devices[i];
      if (
        device_string.size() >= suffix.size() &&
        device_string.compare(
          device_string.size() - suffix.size(),
          suffix.size(),
          suffix
        ) == 0
      ) {
        ret.emplace(*this, device_string);
        break;
      }
    }
    return ret;
  }

  Device::~Device() {
    if (_tags) {
      freefare_free_tags(_tags);
    }
    if (_device) {
      nfc_close(_device);
    }
  }

  bool Device::open() {
    if (_device) return true;
    _device = nfc_open(_context, _device_string.c_str());
    return _device != nullptr;
  }

  vector<Tag> Device::getTags() {
    vector<Tag> ret;
    if (!_device) return ret;
    if (_tags) free(_tags);
    _tags = freefare_get_tags(_device);
    if (!_tags) return ret;
    for(size_t i = 0; _tags[i] && i < 32; i++) {
      ret.emplace_back(*this, _tags[i]);
    }
    return ret;
  }

  enum freefare_tag_type Tag::getTagType() {
    return freefare_get_tag_type(_tag);
  }

  TagInterfaceVariant_t Tag::getTagInterfaceByType() {
    return
      GetVariant<
        TagInterfaceVariant_t
      >::makeArgs(getTagType(), make_tuple(*this));
  }

  Tag::operator FreefareTag() {
    return _tag;
  }

  Key::operator MifareDESFireKey() {
    return nullptr;
  }

  MifareDESFireKey Key::deriveKeyImpl(
    MifareKeyType key_type,
    const UID_t &uid,
    const AppID_t &aid
  ) {
    MifareDESFireKey derived = nullptr;
    CLEAN_KEY MifareDESFireKey master_key = *this;
    IF_LOG(plog::debug) {
      stringbuf sbuf;
      ostream os (&sbuf);
      os << "master_key: ";
      for (uint8_t i = 0; i < 24; i++) {
        os << hex << (int) *(((uint8_t *)master_key) + i);
      }
      LOG_DEBUG << sbuf.str();
    }
    CLEAN_DERIVER MifareKeyDeriver deriver =
      mifare_key_deriver_new_an10922(master_key, key_type);
    if (!deriver)
      return nullptr;
    if (mifare_key_deriver_begin(deriver) < 0)
      return nullptr;

    IF_LOG(plog::debug) {
      stringbuf sbuf;
      ostream os (&sbuf);
      os << "uid: ";
      for (uint8_t i = 0; i < uid.size(); i++) {
        os << hex << (int) *(((uint8_t *)uid.data()) + i);
      }
      LOG_DEBUG << sbuf.str();
    }

    if (mifare_key_deriver_update_data(deriver, uid.data(), uid.size()) < 0)
      return nullptr;
    if (aid[0] || aid[1] || aid[2]) {
      LOG_DEBUG << "AppID";
      if (mifare_key_deriver_update_data(deriver, aid.data(), aid.size()) < 0)
        return nullptr;
    }

    MifareDESFireKey derived_key = mifare_key_deriver_end(deriver);

    IF_LOG(plog::debug) {
      stringbuf sbuf;
      ostream os (&sbuf);
      os << "derived_key: ";
      for (uint8_t i = 0; i < 24; i++) {
        os << hex << (int) *(((uint8_t *)derived_key) + i);
      }
      LOG_DEBUG << sbuf.str();
    }

    return derived_key;
  }

  MifareDESFireKey Key::deriveKey(const UID_t &uid, const AppID_t &aid) {
    LOG_VERBOSE << "Key::deriveKey";
    return nullptr;
  }

  KeyDES::operator MifareDESFireKey() {
    return mifare_desfire_des_key_new(data.data());
  }

  MifareDESFireKey KeyDES::deriveKey(const UID_t &uid, const AppID_t &aid) {
    LOG_VERBOSE << "KeyDES::deriveKey";
    if (!diversify) return nullptr;
    return Key::deriveKeyImpl(key_type, uid, aid);
  }

  Key3DES::operator MifareDESFireKey() {
    return mifare_desfire_3des_key_new(data.data());
  }

  MifareDESFireKey Key3DES::deriveKey(const UID_t &uid, const AppID_t &aid) {
    LOG_VERBOSE << "Key3DES::deriveKey";
    if (!diversify) return nullptr;
    return Key::deriveKeyImpl(key_type, uid, aid);
  }

  Key3k3DES::operator MifareDESFireKey() {
    return mifare_desfire_3k3des_key_new(data.data());
  }

  MifareDESFireKey Key3k3DES::deriveKey(const UID_t &uid, const AppID_t &aid) {
    LOG_VERBOSE << "Key3k3DES::deriveKey";
    if (!diversify) return nullptr;
    return Key::deriveKeyImpl(key_type, uid, aid);
  }

  KeyAES::operator MifareDESFireKey() {
    return mifare_desfire_aes_key_new(data.data());
  }

  MifareDESFireKey KeyAES::deriveKey(const UID_t &uid, const AppID_t &aid) {
    LOG_VERBOSE << "KeyAES::deriveKey";
    if (!diversify) return nullptr;
    return Key::deriveKeyImpl(key_type, uid, aid);
  }

}
