#include <exception>
#include <vector>
#include <variant>
#include <string>
#include <string_view>
#include <optional>
#include <functional>
#include <tuple>

#include <nfc/nfc.h>
#include <freefare.h>

#include "tag.hpp"
#include "types.hpp"

#pragma once

namespace nfcdoorz::nfc {
  class Device;

  class Context {
public:
    Context() : _context(nullptr) {
    };
    ~Context();
    bool init();
    std::vector<Device> getDevices();
    std::optional<Device> getDeviceMatching(std::string suffix);
    Device getDeviceString(std::string device_string);

    operator nfc_context *() {
      return _context;
    };

private:
    nfc_context *_context;
  };

  class Tag;

  class Device {
public:
    Device(Context &context, std::string device_string) :
      _context(context),
      _device(nullptr),
      _device_string(device_string),
      _tags(nullptr) {
    }
    ~Device();
    bool open();

    bool initiatorInit();
    bool initiatorPollTarget(std::function<bool (Tag &tag)> handleTag);
    std::vector<Tag> getTags();

private:
    Context &_context;
    nfc_device *_device;
    std::string _device_string;
    FreefareTag *_tags;
  };


  using TagInterfaceVariant_t = std::variant<DESFireTagInterface>;

  class Tag {
public:
    Tag(Device &device, FreefareTag tag) :
      _device(device), _tag(tag) {
    };

    enum freefare_tag_type getTagType();
    TagInterfaceVariant_t getTagInterfaceByType();
    operator FreefareTag();
private:
    Device &_device;
    FreefareTag _tag;
  };

  constexpr AppID_t ZERO_AID = { 0, 0, 0 };

  struct Key {
    bool diversify = false;
    virtual operator MifareDESFireKey();
    virtual MifareDESFireKey deriveKey(const UID_t &uid, const AppID_t &aid = ZERO_AID);

protected:
    MifareDESFireKey deriveKeyImpl(MifareKeyType key_type, const UID_t &uid, const AppID_t &aid);
  };

  template<uint8_t size>
  std::array<uint8_t, size> hexToArray(std::string str) {
    std::array<uint8_t, size> arr;
    size_t len = str.length();
    if (len != size * 2) {
      throw std::exception();
    }
    for (uint8_t i = 0; i < len; i += 2) {
      arr[i / 2] = (char2int(str[i]) << 4) | char2int(str[i + 1]);
    }
    return arr;
  }

  struct KeyDES : public Key {
    constexpr static const std::string_view type = "des";
    static const MifareKeyType key_type = MIFARE_KEY_DES;
    constexpr static const uint8_t size = 8;
    using KeyArray_t = std::array<uint8_t, size>;
    KeyArray_t data;
    KeyDES() {
    };
    KeyDES(KeyArray_t _data) : data(_data) {
    };
    KeyDES(std::string hexString) : data(hexToArray<size>(hexString)) {
    };
    KeyDES(std::string hexString, bool _diversify) : data(hexToArray<size>(hexString)) {
      diversify = _diversify;
    };

    operator MifareDESFireKey() override;
    MifareDESFireKey deriveKey(const UID_t &uid, const AppID_t &aid = ZERO_AID) override;
  };
  struct Key3DES : public Key {
    constexpr static const std::string_view type = "3des";
    static const MifareKeyType key_type = MIFARE_KEY_2K3DES;
    constexpr static const uint8_t size = 16;
    using KeyArray_t = std::array<uint8_t, size>;
    KeyArray_t data;
    Key3DES() {
    };
    Key3DES(KeyArray_t _data) : data(_data) {
    };
    Key3DES(std::string hexString) : data(hexToArray<size>(hexString)) {
    };
    Key3DES(std::string hexString, bool _diversify) : data(hexToArray<size>(hexString)) {
      diversify = _diversify;
    };
    operator MifareDESFireKey() override;
    MifareDESFireKey deriveKey(const UID_t &uid, const AppID_t &aid = ZERO_AID) override;
  };
  struct Key3k3DES : public Key {
    constexpr static const std::string_view type = "3k3des";
    static const MifareKeyType key_type = MIFARE_KEY_3K3DES;
    constexpr static const uint8_t size = 24;
    using KeyArray_t = std::array<uint8_t, size>;
    KeyArray_t data;
    Key3k3DES() {
    };
    Key3k3DES(KeyArray_t _data) : data(_data) {
    };
    Key3k3DES(std::string hexString) : data(hexToArray<size>(hexString)) {
    };
    Key3k3DES(std::string hexString, bool _diversify) : data(hexToArray<size>(hexString)) {
      diversify = _diversify;
    };
    operator MifareDESFireKey() override;
    MifareDESFireKey deriveKey(const UID_t &uid, const AppID_t &aid = ZERO_AID) override;
  };
  struct KeyAES : public Key {
    constexpr static const std::string_view type = "aes";
    static const MifareKeyType key_type = MIFARE_KEY_AES128;
    constexpr static const uint8_t size = 16;
    using KeyArray_t = std::array<uint8_t, size>;
    KeyArray_t data;
    KeyAES() {
    };
    KeyAES(KeyArray_t _data) : data(_data) {
    };
    KeyAES(std::string hexString) : data(hexToArray<size>(hexString)) {
    };
    KeyAES(std::string hexString, bool _diversify) : data(hexToArray<size>(hexString)) {
      diversify = _diversify;
    };
    operator MifareDESFireKey() override;
    MifareDESFireKey deriveKey(const UID_t &uid, const AppID_t &aid = ZERO_AID) override;
  };

  using KeyVariant_t = std::variant<KeyDES, Key3DES, Key3k3DES, KeyAES>;
  using OptionalKeyVariant_t = std::variant<std::monostate, KeyDES, Key3DES, Key3k3DES, KeyAES>;
}
