#include <iomanip>

#include "config.hpp"
#include "adapter.hpp"
#include "types.hpp"

namespace nfcdoorz {
  using namespace std;

  bool Adapter::connect() {
    return tag.connect();
  }
  bool Adapter::disconnect() {
    return tag.disconnect();
  }

  bool Adapter::authenticatePICC(nfc::Key &key) {
    AppID_t aid = {0, 0, 0};
    return authenticateAppByID(aid, key);
  }

  bool Adapter::authenticatePICC() {
    AppID_t aid = {0, 0, 0};
    if (_overriddenMasterKey) {
      return authenticateAppByID(
        aid,
        *_overriddenMasterKey
      );
    } else {
      auto key = get<config::KeyAES>(config.picc.key);
      return authenticateAppByID(
        aid,
        key
      );
    }
  }

  bool Adapter::authenticateAppByID(AppID_t &aid, nfc::Key &key) {
    CLEAN MifareDESFireAID api_aid = mifare_desfire_aid_new(aid[0] | (aid[1] << 8) | (aid[2] << 16));
    if (!tag.select_application(api_aid)) return false;
    _aid = aid;
    CLEAN_KEY MifareDESFireKey api_key = key;
    return tag.authenticate(0, api_key);
  }

  bool Adapter::authenticateApp(string app_name, string key_name) {
    for (auto &app: config.apps) {
      if (app.name == app_name) {
        for (auto &key: app.keys) {
          if (visit([](auto &key) -> string { return key.name; }, key) == key_name) {
            auto key_type = get<config::KeyAES>(key);
            if (authenticateAppByID(app.aid, key_type)) {
              _app = &app;
              return true;
            }
            _app = nullptr;
            return false;
          }
          return false;
        }
      }
    }
    return false;
  }

  bool Adapter::changeKey(uint8_t key_id, nfc::Key &new_key, nfc::Key &old_key) {
    CLEAN_KEY MifareDESFireKey api_new_key = new_key;
    CLEAN_KEY MifareDESFireKey api_old_key = old_key;
    return tag.change_key(key_id, api_new_key, api_old_key);
  }

  bool Adapter::deleteApplication(AppID_t aid) {
    CLEAN MifareDESFireAID api_aid = mifare_desfire_aid_new(aid[0] | (aid[1] << 8) | (aid[2] << 16));
    return tag.delete_application(api_aid);
  }

  bool Adapter::createApplication(AppID_t aid, uint8_t settings, uint8_t numKeys) {
    CLEAN MifareDESFireAID api_aid = mifare_desfire_aid_new(aid[0] | (aid[1] << 8) | (aid[2] << 16));
    return tag.create_application_aes(api_aid, settings, numKeys);
  }

  bool Adapter::setUID(vector<uint8_t> uid) {
    _uid = uid;
    return true;
  }

  bool Adapter::setUID(string uid_str) {
    vector<uint8_t> uid;
    int len = uid_str.length();
    if (len % 2) return false;
    for (uint8_t i = 0; i < len; i += 2) {
      uid.push_back((char2int(uid_str[i]) << 4) | char2int(uid_str[i+1]));
    }
    return setUID(uid);
  }

  bool Adapter::getUIDFromCard() {
    CLEAN char *uid = tag.get_uid();
    return setUID(uid);
  }

  vector<uint8_t> Adapter::getUID() {
    if (!_uid.size()) getUIDFromCard();
    return _uid;
  }

  string Adapter::getUIDString() {
    auto uid = getUID();
    stringstream sb("");
    bool rest = false;
    for (auto c: uid) {
      if (rest) sb << ":";
      sb
        << setfill('0')
        << setw(2)
        << hex
        << (int) c;
      rest = true;
    }
    return sb.str();
  }

  bool Adapter::setOverriddenMasterKey(nfc::Key key) {
    _overriddenMasterKey = key;
    return true;
  }
  bool Adapter::setOverriddenMasterKey(string key) {
    // TODO Make less brittle
    auto master_key = nfc::KeyAES();
    int len = key.length();
    if (len % 2) return false;
    for (uint8_t i = 0; i < len; i += 2) {
      master_key.data[i / 2] = (char2int(key[i]) << 4) | char2int(key[i+1]);
    }
    return setOverriddenMasterKey(master_key);
  }

}
