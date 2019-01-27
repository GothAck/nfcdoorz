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
    Tag &tag,
    MifareKeyType key_type,
    MifareDESFireAID aid
  ) {
    MifareDESFireKey derived = nullptr;
    CLEAN_KEY MifareDESFireKey master_key = *this;
    CLEAN_DERIVER MifareKeyDeriver deriver =
      mifare_key_deriver_new_an10922(master_key, key_type);
    if (!deriver) return nullptr;
    if (mifare_key_deriver_begin(deriver) < 0) return nullptr;
    FreefareTag tag2 = tag;
    if (mifare_key_deriver_update_uid(deriver, tag2) < 0) return nullptr;
    if (aid)
      if (mifare_key_deriver_update_aid(deriver, aid) < 0) return nullptr;

    return mifare_key_deriver_end(deriver);
  }

  MifareDESFireKey Key::deriveKey(Tag &tag, MifareDESFireAID aid) {
    return nullptr;
  }

  KeyDES::operator MifareDESFireKey() {
    return mifare_desfire_des_key_new(data.data());
  }

  MifareDESFireKey KeyDES::deriveKey(Tag &tag, MifareDESFireAID aid) {
    return Key::deriveKeyImpl(tag, key_type, aid);
  }

  Key3DES::operator MifareDESFireKey() {
    return mifare_desfire_3des_key_new(data.data());
  }

  MifareDESFireKey Key3DES::deriveKey(Tag &tag, MifareDESFireAID aid) {
    return Key::deriveKeyImpl(tag, key_type, aid);
  }

  Key3k3DES::operator MifareDESFireKey() {
    return mifare_desfire_3k3des_key_new(data.data());
  }

  MifareDESFireKey Key3k3DES::deriveKey(Tag &tag, MifareDESFireAID aid) {
    return Key::deriveKeyImpl(tag, key_type, aid);
  }

  KeyAES::operator MifareDESFireKey() {
    return mifare_desfire_aes_key_new(data.data());
  }

  MifareDESFireKey KeyAES::deriveKey(Tag &tag, MifareDESFireAID aid) {
    return Key::deriveKeyImpl(tag, key_type, aid);
  }

}
