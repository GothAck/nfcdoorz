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
#include <plog/Appenders/ColorConsoleAppender.h>

#include "lib/types.hpp"
#include "lib/config.hpp"
#include "lib/signals.h"
#include "lib/nfc.hpp"
#include "lib/adapter.hpp"

using namespace std;
using namespace nfcdoorz;

static const char USAGE[] =
  R"(NFC-Doorz card pre-configuration

      Usage:
        preconfig [-v | -vv | -vvv] [options] <reader>
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

static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;

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

  plog::Severity severity = plog::info;
  switch(args["--verbose"].asLong()) {
    case 1:
      severity = plog::info;
      break;
    case 2:
      severity = plog::debug;
      break;
    case 3:
      severity = plog::verbose;
      break;
  }

  plog::init(severity, &consoleAppender);

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

  if (options.dry_run)
    LOG_WARNING << "DRY RUN!";

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
    LOG_WARNING << "Overridden UID: " << uid;
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
    LOG_WARNING << "Overridden PICC Key: " << picc_key;
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
    LOG_WARNING << "Delete apps: " << delete_apps;
    options.delete_apps = delete_apps;
  }

  config::Config config;

  try {
    config = config::Config::load(args["--config"].asString());
  } catch (YAML::Exception e) {
    LOG_ERROR
      << e.what() << endl
      << "Invalid config node found" << endl
      << config::printDecodePath();
    return 9;
  } catch (config::ValidationException e) {
    LOG_ERROR
      << e.what() << endl
      << "Invalid config node found" << endl
      << config::printDecodePath();
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


  LOG_INFO << "Fetched tags";
  for (auto &tag: tags) {
    if (MIFARE_DESFIRE != tag.getTagType()) {
      LOG_WARNING << "tag not MIFARE_DESFIRE";
      continue;
    }
    Adapter adapter(config, tag);
    LOG_INFO << "Found desfire tag";

    if (options.overridden_uid)
      adapter.setUID(*options.overridden_uid);

    if (options.overridden_picc_key)
      adapter.setOverriddenMasterKey(*options.overridden_picc_key);

    LOG_VERBOSE << "Connecting to desfire";
    CHECK_BOOL(adapter.connect(), "Can't connect to Mifare DESFire target.");

    adapter.getUID();

    if (!options.skip_picc_rekey) {
      if (!options.overridden_picc_key) {
        LOG_INFO << "Authenticating with null des key";
        CHECK_BOOL(adapter.authenticatePICC(null_key_des), "Authentication on PICC failed");
        LOG_INFO << "Authenticated master application";
      } else {
        CHECK_BOOL(adapter.authenticatePICC(), "Authentication on PICC with overridden key failed");
      }

      if (!options.dry_run) {
        LOG_INFO << "Setting tag config";
        CHECK_BOOL(adapter.tagInterface.set_configuration(false, true), "Failed to set card config");
      }
    }



    LOG_INFO << "Tag uid " << adapter.getUIDString();

    LOG_INFO << "Authenticating with PICC master key";

    {
      // DESFireAESKey key = config.picc.key;
      if (options.overridden_picc_key) {
        LOG_INFO << "Authenticating with existing key ";
        CHECK_BOOL(adapter.authenticatePICC(), "Failed to autenticate");
        LOG_INFO << "Authenticated";
      }

      if (!options.skip_picc_rekey) {
        if (!options.dry_run) {
          LOG_INFO << "Setting new master key";
          visit([&adapter](auto &key){
            CHECK_BOOL(adapter.changeKey(0, key, null_key_des), "Failed to change master key");
            LOG_INFO << "Set new master key";
          }, config.picc.key);
        }
      }

      LOG_INFO << "Authenticating with master key";
      visit([&adapter](auto &key){
        CHECK_BOOL(adapter.authenticatePICC(key), "Failed to auth with master key");
      }, config.picc.key);
    }

    LOG_INFO << "Card contains applications:";
    auto apps = adapter.tagInterface.get_application_ids();
    for (auto it = apps.begin(); it != apps.end(); it++) {
      LOG_INFO
        << "  - "
        << setfill('0') << setw(2) << hex
        << (int) it->at(0) << ":" << (int) it->at(1) << ":" << (int) it->at(2);
    }

    if (!options.skip_app_rekey) {
      if (!options.dry_run) {
        LOG_INFO << "Setting default app key";
        CHECK_BOOL(adapter.tagInterface.set_default_key(null_key_aes), "Failed to set default application key");
      }
    }
    if (options.dry_run) return 0;

    if (options.delete_app_creation) {
      AppID_t aid = app.aid;
      LOG_INFO << "Deleting app";
      CHECK_BOOL(adapter.deleteApplication(aid), "Failed to delete application");
    }

    if (options.delete_apps) {
      AppID_t aid;
      const char *c_str = options.delete_apps->c_str();
      for (uint8_t i = 0; i < 6; i += 2) {
        aid[i / 2] = (char2int(c_str[i]) << 4) | char2int(c_str[i + 1]);
      }
      LOG_INFO << "Deleting app";
      CHECK_BOOL(adapter.deleteApplication(aid), "Failed to delete application");
    }

    if (!options.skip_app_creation) {
      AppID_t aid = app.aid;
      LOG_INFO << "Creating app";
      CHECK_BOOL(adapter.createApplication(aid, app.settings.get_lib_value(), app.keys.size()), "Failed to create application");
    }

    if (options.skip_app_rekey) {
      LOG_INFO << "Authenticating with app key";
      AppID_t aid = app.aid;
      visit([&adapter, &aid](auto &key){
        CHECK_BOOL(adapter.authenticateAppByID(aid, key), "Failed to authenticate app");
      }, app.keys[0]);
    } else {
      AppID_t aid = app.aid;
      CHECK_BOOL(adapter.authenticateAppByID(aid, null_key_aes), "Failed to authenticate app with default key");
      for (auto it = app.keys.rbegin(); it != app.keys.rend(); it++) {
        visit([&adapter, &aid, it](auto &key){
          LOG_INFO << "Key id: " << (int) key.id;
          LOG_INFO << "Key name: " << key.name;
          LOG_INFO << "Key diversify: " << key.diversify;
          CHECK_BOOL(adapter.changeKey(key.id, key, null_key_aes), "Failed to change key");
        }, *it);
      }

    }

    {
      LOG_INFO << "Reauthenticating with app master key";
      AppID_t aid = app.aid;
      visit([&adapter, &aid](auto &key){
        CHECK_BOOL(adapter.authenticateAppByID(aid, key), "Failed to authenticate app with master key");
      }, app.keys[0]);
      LOG_INFO << "Authentication okay";
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
