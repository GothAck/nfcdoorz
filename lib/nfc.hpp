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
#include "types.h"

#pragma once

namespace nfcdoorz::nfc {
  class Device;

  class Context {
  public:
    Context(): _context(nullptr) {};
    ~Context();
    bool init();
    std::vector<Device> getDevices();
    std::optional<Device> getDeviceMatching(std::string suffix);

    operator nfc_context*() { return _context; };

  private:
    nfc_context *_context;
  };

  class Tag;

  class Device {
  public:
    Device(Context &context, std::string device_string):
      _context(context),
      _device(nullptr),
      _device_string(device_string),
      _tags(nullptr) {}
    ~Device();
    bool open();

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
    Tag(Device &device, FreefareTag tag):
      _device(device), _tag(tag) {};

    enum freefare_tag_type getTagType();
    TagInterfaceVariant_t getTagInterfaceByType();
    operator FreefareTag();
  private:
    Device &_device;
    FreefareTag _tag;
  };

  struct Key {
    virtual operator MifareDESFireKey();
  };

  struct KeyDES : public Key {
    constexpr static const std::string_view type = "des";
    constexpr static const uint8_t size = 8;
    std::array<uint8_t, size> data;
    KeyDES() {};
    KeyDES(std::array<uint8_t, size> _data): data(_data) {};
    operator MifareDESFireKey() override;
  };
  struct Key3DES : public Key {
    constexpr static const std::string_view type = "3des";
    constexpr static const uint8_t size = 16;
    std::array<uint8_t, size> data;
    Key3DES() {};
    Key3DES(std::array<uint8_t, size> _data): data(_data) {};
    operator MifareDESFireKey() override;
  };
  struct Key3k3DES : public Key {
    constexpr static const std::string_view type = "3k3des";
    constexpr static const uint8_t size = 24;
    std::array<uint8_t, size> data;
    Key3k3DES() {};
    Key3k3DES(std::array<uint8_t, size> _data): data(_data) {};
    operator MifareDESFireKey() override;
  };
  struct KeyAES : public Key {
    constexpr static const std::string_view type = "aes";
    constexpr static const uint8_t size = 16;
    std::array<uint8_t, size> data;
    KeyAES() {};
    KeyAES(std::array<uint8_t, size> _data): data(_data) {};
    operator MifareDESFireKey() override;
  };
}
