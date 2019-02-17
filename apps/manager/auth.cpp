#include <algorithm>
#include <iostream>
#include <iomanip>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include <docopt/docopt.h>
#include <yaml-cpp/yaml.h>
#include <nfc/nfc.h>
#include <freefare.h>

#include "lib/types.hpp"
#include "lib/logging.hpp"
#include "lib/config.hpp"
#include "lib/signals.h"
#include "lib/nfc.hpp"
#include "lib/adapter.hpp"
#include "lib/ipc/auth.hpp"

using namespace std;
using namespace nfcdoorz;

static const char USAGE[] =
  R"(NFC-Doorz Manager Auth daemon

      Usage:
        manager-auth [-v | -vv | -vvv] [options] <reader>
        manager-auth (-h | --help)
        manager-auth --version

      Options:
        -h --help               Show this help
        --version               Show version
        -v --verbose            Verbose output
        --uid=UID               Override tag uid
        <reader>                Reader tty or fd
        )" MANAGER_CLIENT_ARGS R"(
  )";

#define CHECK_BOOL(call, err_str) if (!call) { errx(9, err_str); }

ipc::IpcClientAuth client;

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


  logging::init(args);

  client.connect(args["--ipc-connect"].asString());
  client.runThread();

  auto config = client.getConfigCall().gen().get();

  cout << config->config << endl;

  nfc::Context context;
  if (!context.init()) {
    errx(EXIT_FAILURE, "Unable to init libnfc (malloc)");
  }

  // auto matched_device = context.getDeviceMatching(args["<reader>"].asString());

  string reader = args["<reader>"].asString();

  string port = reader.substr(reader.find(":") + 1);

  cout << "Reader: " << port << endl;

  int fd = open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);

  if (fd == -1) {
    cout << "Could not open" << endl;
    return 99;
  }

  struct termios termios_backup;
  struct termios termios_new;

  if (tcgetattr(fd, &termios_backup) == -1) {
    cout << "tcgetattr fail" << endl;
    return 99;
  }

  termios_new = termios_backup;

  termios_new.c_cflag = CS8 | CLOCAL | CREAD;
  termios_new.c_iflag = IGNPAR;
  termios_new.c_oflag = 0;
  termios_new.c_lflag = 0;

  termios_new.c_cc[VMIN] = 0; // block until n bytes are received
  termios_new.c_cc[VTIME] = 0; // block until a timer expires (n * 100 mSec.)

  if (tcsetattr(fd, TCSANOW, &termios_new) == -1) {
    cout << "tcsetattr" << endl;
    return 99;
  }

  close(fd);
  // return 1;

  auto device = context.getDeviceString(reader);

  if (!device.open()) {
    errx(9, "nfc_open() failed.");
  }

  if (!device.initiatorInit()) {
    LOG_ERROR << "Could not init initiator";
    return 7;
  }

  while (true) {
    if (!device.initiatorPollTarget([](nfc::Tag &tag) {
      if (tag.getTagType() == MIFARE_DESFIRE) {
        LOG_WARNING << "DESFIRE";
      }
      return true;
    })) {
      LOG_WARNING << "Exiting on error";
      break;
    };
  }
  return 0;
}
