#include <algorithm>
#include <iostream>
#include <iomanip>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <docopt/docopt.h>
#include <yaml-cpp/yaml.h>
#include <nfc/nfc.h>
#include <freefare.h>

#include "lib/config.hpp"
#include "lib/signals.h"
#include "lib/nfc.hpp"
#include "lib/adapter.hpp"

using namespace std;
using namespace nfcdoorz;

static const char USAGE[] =
  R"(NFC-Doorz card pre-configuration

      Usage:
        preconfig [options] <reader>
        preconfig (-h | --help)
        preconfig --version

      Options:
        -h --help               Show this help
        --version               Show version
        --config=CONFIG         Use config file [default: ./config.yaml]
        -v --verbose            Verbose output
        -n --dry-run            Disable writing to tag
        --uid=UID               Override tag uid
        --current-picc-key=KEY  Current PICC key
        --skip-picc-rekey       Skip rekey of PICC (already keyed)
        --delete-app=AID        Delete AID app from card
        --skip-app-creation     Skip app creation (already exists)
        --skip-app-rekey        Skip app rekey from default null key
        --delete-app-creation   Delete app before creation (already exists)
        --print-config          Print loaded config for debugging purposes
        <reader>                Configure card presented to this reader
  )";

uint8_t salt[SALT_SIZE] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#define CHECK_BOOL(call, err_str) if (!call) { errx(9, err_str); }

nfc::KeyDES null_key_des;
nfc::KeyAES null_key_aes;

int main(int argc, char *argv[]) {
  int error = EXIT_SUCCESS;

  map<string, docopt::value> args = docopt::docopt(
    USAGE,
    { argv + 1, argv + argc },
    true,
    "NFC-Doorz preconfig v0.0.1"
  );

  struct {
    optional<string> overridden_uid;
    optional<string> delete_apps;
    optional<string> overridden_picc_key;
    bool dry_run;
    bool print_config;
    bool skip_picc_rekey;
    bool skip_app_rekey;
    bool delete_app_creation;
    bool skip_app_creation;
  } options = {
    args["--uid"]
      ? make_optional(args["--uid"].asString())
      : nullopt,
    args["--delete-app"]
      ? make_optional(args["--delete-app"].asString())
      : nullopt,
    args["--current-picc-key"]
     ? make_optional(args["--current-picc-key"].asString())
     : nullopt,
    args["--dry-run"].asBool(),
    args["--print-config"].asBool(),
    args["--skip-picc-rekey"].asBool(),
    args["--skip-app-rekey"].asBool(),
    args["--delete-app-creation"].asBool(),
    args["--skip-app-creation"].asBool(),
  };

  for (auto a: args) {
    cout << "key: " << a.first << " value: " << a.second << endl;
  }

  if (options.dry_run)
    cout << "DRY RUN!" << endl;

  if (options.overridden_uid) {
    string uid = *options.overridden_uid;
    uid.erase(remove(
      uid.begin(),
      uid.end(),
      ':'
    ), uid.end());
    if (uid.length() % 2)
      errx(9, "overridden uid should be an even number of hex characters e.g. 01020304");
    if (!all_of(uid.begin(), uid.end(), [](unsigned char c){ return isxdigit(c); }))
      errx(9, "overridden uid should be hex characters e.g. a1B2c3D4");
    options.overridden_uid = uid;
  }

  if (options.overridden_picc_key) {
    string picc_key = *options.overridden_picc_key;
    picc_key.erase(remove(
      picc_key.begin(),
      picc_key.end(),
      ':'
    ), picc_key.end());
    if (32 != picc_key.length())
      errx(9, "overridden picc key should be 32 hex characters e.g. 01020304");
    if (!all_of(picc_key.begin(), picc_key.end(), [](unsigned char c){ return isxdigit(c); }))
      errx(9, "overridden picc key should be 32 hex characters e.g. a1B2c3D4");
    options.overridden_picc_key = picc_key;
  }

  if (options.delete_apps) {
    string delete_apps = *options.delete_apps;
    delete_apps.erase(remove(
      delete_apps.begin(),
      delete_apps.end(),
      ':'
    ), delete_apps.end());
    if (6 != delete_apps.length())
      errx(9, "delete app should be 6 hex characters e.g. 010203");
    if (!all_of(delete_apps.begin(), delete_apps.end(), [](unsigned char c){ return isxdigit(c); }))
      errx(9, "delete app should be 6 hex characters e.g. a1B2c3");
    options.delete_apps = delete_apps;
  }

  config::Config config;

  try {
    config = config::Config::load(args["--config"].asString());
  } catch (YAML::Exception e) {
    cout
      << e.what() << endl
      << "Invalid config node found" << endl
      << config::printDecodePath() << endl;
    return 9;
  } catch (config::ValidationException e) {
    cout
      << e.what() << endl
      << "Invalid config node found" << endl
      << config::printDecodePath() << endl;
    return 9;
  }

  if (args["--print-config"].asBool()) {
    cout << config << endl;
    return 0;
  }

  if (!config.apps.size())
    errx(9, "No apps in config");

  auto &app = config.apps[0];

  nfc::Context context;
  if (!context.init())
    errx(EXIT_FAILURE, "Unable to init libnfc (malloc)");

  auto matched_device = context.getDeviceMatching(args["<reader>"].asString());

  if (!matched_device) {
    errx(9, "Could not find device.");
    return 9;
  }
  auto &device = *matched_device;

  if (!device.open()) {
    errx(9, "nfc_open() failed.");
  }

  auto tags = device.getTags();

  if (!tags.size()) {
    errx(EXIT_FAILURE, "Error listing tags.");
  }


  cout << "Fetched tags" << endl;
  // DESFireCard card;
  cout << "iterating" << endl;
  for (auto &tag: tags) {
    if (MIFARE_DESFIRE != tag.getTagType()) {
      cout << "tag not MIFARE_DESFIRE" << endl;
      continue;
    }
    Adapter adapter(config, tag);
    cout << "Found desfire tag" << endl;

    if (options.overridden_uid)
      adapter.setUID(*options.overridden_uid);

    if (options.overridden_picc_key)
      adapter.setOverriddenMasterKey(*options.overridden_picc_key);

    cout << "Connecting to desfire" << endl;
    CHECK_BOOL(adapter.connect(), "Can't connect to Mifare DESFire target.");

    adapter.getUID();


    if (!options.skip_picc_rekey) {
      if (!options.overridden_picc_key) {
        cout << "Authenticating with null des key" << endl;
        CHECK_BOOL(adapter.authenticatePICC(null_key_des), "Authentication on PICC failed");
        cout << "Authenticated master application" << endl;
      } else {
        CHECK_BOOL(adapter.authenticatePICC(), "Authentication on PICC with overridden key failed");
      }

      if (!options.dry_run) {
        cout << "Setting tag config" << endl;
        CHECK_BOOL(adapter.tagInterface.set_configuration(false, true), "Failed to set card config");
      }
    }


    cout << "Tag uid " << adapter.getUIDString() << endl;
    // 62:07:0c:26:23:2b:a6:3e:91:8d:90:35:a3:d7:d0:32:

    cout << "Authenticating with PICC master key";

    {
      // DESFireAESKey key = config.picc.key;
      if (options.overridden_picc_key) {
        cout << "Authenticating with existing key " << endl;
        CHECK_BOOL(adapter.authenticatePICC(), "Failed to autenticate");
        cout << "Authenticated" << endl;
      }

      if (!options.skip_picc_rekey) {
        if (!options.dry_run) {
          cout << "Setting new master key" << endl;
          visit([&adapter](auto &key){
            CHECK_BOOL(adapter.changeKey(0, key, null_key_des), "Failed to change master key");
            cout << "Set new master key" << endl;
          }, config.picc.key);
        }
      }

      cout << "Authenticating with master key" << endl;
      visit([&adapter](auto &key){
        CHECK_BOOL(adapter.authenticatePICC(key), "Failed to auth with master key");
      }, config.picc.key);
    }

    auto apps = adapter.tagInterface.get_application_ids();
    for (auto it = apps.begin(); it != apps.end(); it++) {
      // FIXME: actually print app ids
      cout << "app" << endl;
    }

    if (!options.skip_app_rekey) {
      if (!options.dry_run) {
        cout << "Setting default app key" << endl;
        CHECK_BOOL(adapter.tagInterface.set_default_key(null_key_aes), "Failed to set default application key");
      }
    }
    if (options.dry_run) return 0;

    if (options.delete_app_creation) {
      AppID_t aid = app.aid;
      cout << "Deleting app" << endl;
      CHECK_BOOL(adapter.deleteApplication(aid), "Failed to delete application");
    }

    if (options.delete_apps) {
      AppID_t aid;
      const char *c_str = options.delete_apps->c_str();
      for (uint8_t i = 0; i < 6; i += 2) {
        aid[i / 2] = (char2int(c_str[i]) << 4) | char2int(c_str[i + 1]);
      }
      cout << "Deleting app" << endl;
      CHECK_BOOL(adapter.deleteApplication(aid), "Failed to delete application");
    }

    if (!options.skip_app_creation) {
      AppID_t aid = app.aid;
      cout << "Creating app" << endl;
      CHECK_BOOL(adapter.createApplication(aid, app.settings.get_lib_value(), app.keys.size()), "Failed to create application");
    }

    if (options.skip_app_rekey) {
      cout << "Authenticating with app key" << endl;
      AppID_t aid = app.aid;
      visit([&adapter, &aid](auto &key){
        CHECK_BOOL(adapter.authenticateAppByID(aid, key), "Failed to authenticate app");
      }, app.keys[0]);
    } else {
      AppID_t aid = app.aid;
      CHECK_BOOL(adapter.authenticateAppByID(aid, null_key_aes), "Failed to authenticate app with default key");
      for (auto it = app.keys.rbegin(); it != app.keys.rend(); it++) {
        visit([&adapter, &aid, it](auto &key){
          cout << "Key id: " << (int) key.id << endl;
          cout << "Key name: " << key.name << endl;
          cout << "Key diversify: " << key.diversify << endl;
          CHECK_BOOL(adapter.changeKey(key.id, key, null_key_aes), "Failed to change key");
        }, *it);
      }

    }

    {

      cout << "Reauthenticating with app master key" << endl;
      AppID_t aid = app.aid;
      visit([&adapter, &aid](auto &key){
        CHECK_BOOL(adapter.authenticateAppByID(aid, key), "Failed to authenticate app with master key");
      }, app.keys[0]);
      // if (master_key.diversify) {
      //   cout << "Diversifying " << endl;
      //   CHECK_BOOL(key.diversify(app.aid, tag_uid, salt), "Diversify key failed");
      // }
      cout << "Authentication okay" << endl;
    }

    // Create files

    {
      for (auto &config_file: app.files) {
        visit([](auto &file){
          cout << file << endl;
        }, config_file);
        // visit([&card, &config_file](auto &config){config.create(card, config_file);}, config_file.config);
      }
    }

    adapter.disconnect();
  }
  exit(error);
}
