#include <string>
#include <vector>
#include <optional>
#include "types.hpp"
#include "config.hpp"
#include "tag.hpp"

#pragma once

namespace nfcdoorz {

  class Adapter {
  public:
    config::Config &config;
    nfc::DESFireTagInterface tagInterface;
    nfc::Tag &_tag;
    Adapter(config::Config &_config, nfc::Tag &_tag):
      config(_config),
      _tag(_tag),
      tagInterface(std::get<nfc::DESFireTagInterface>(_tag.getTagInterfaceByType())),
      _aid({0, 0, 0}),
      _uid({0, 0, 0, 0, 0, 0, 0}),
      _app(nullptr) {}

    bool setOverriddenMasterKey(nfc::KeyVariant_t key);
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

    bool setUID(UID_t uid);
    bool setUID(std::string uid);
    bool getUIDFromCard();
    UID_t getUID();
    std::string getUIDString();
  private:
    nfc::OptionalKeyVariant_t _overriddenMasterKey;
    UID_t _uid;
    AppID_t _aid;
    config::App *_app;
  };

}
