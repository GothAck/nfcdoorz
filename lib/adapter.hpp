#include <string>
#include <vector>
#include <optional>
#include "types.hpp"
#include "config.hpp"
#include "tag.hpp"

#pragma once

namespace nfcdoorz {

  // typedef std::vector<uint8_t> KeyVector_t;

  class Adapter {
  public:
    config::Config &config;
    nfc::DESFireTagInterface tag;
    Adapter(config::Config &_config, nfc::Tag &_tag):
      config(_config),
      tag(std::get<nfc::DESFireTagInterface>(_tag.getTagInterfaceByType())),
      _overriddenMasterKey(std::nullopt),
      _aid({0, 0, 0}),
      _app(nullptr) {}

    bool setOverriddenMasterKey(nfc::Key key);
    bool setOverriddenMasterKey(std::string key);

    bool connect();
    bool disconnect();

    bool authenticatePICC(nfc::Key &key);
    bool authenticatePICC();

    bool authenticateAppByID(AppID_t &aid, nfc::Key &key);
    bool authenticateApp(std::string app_name, std::string key_name);

    bool changeKey(uint8_t key_id, nfc::Key &new_key, nfc::Key &old_key);

    bool deleteApplication(AppID_t aid);
    bool createApplication(AppID_t aid, uint8_t settings, uint8_t numKeys);

    bool setUID(std::vector<uint8_t> uid);
    bool setUID(std::string uid);
    bool getUIDFromCard();
    std::vector<uint8_t> getUID();
    std::string getUIDString();
  private:
    std::optional<nfc::Key> _overriddenMasterKey;
    std::vector<uint8_t> _uid;
    AppID_t _aid;
    config::App *_app;
  };

}
