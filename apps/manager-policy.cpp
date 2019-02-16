#include <docopt/docopt.h>
#include <yaml-cpp/yaml.h>
#include <sol.hpp>

#include "lib/types.hpp"
#include "lib/logging.hpp"
#include "lib/config.hpp"
#include "lib/signals.h"
#include "lib/nfc.hpp"
#include "lib/adapter.hpp"
#include "lib/ipc/policy.hpp"

using namespace std;
using namespace nfcdoorz;

ipc::IpcClientPolicy client;

static const char USAGE[] =
  R"(NFC-Doorz Manager Policy daemon

      Usage:
        manager-policy [-v | -vv | -vvv] [options]
        manager-policy (-h | --help)
        manager-policy --version

      Options:
        --standalone            Disable connection to manager process.
        -h --help               Show this help
        --version               Show version
        -v --verbose            Verbose output
        )" MANAGER_CLIENT_ARGS R"(
  )";

int main(int argc, char *argv[]) {
  map<string, docopt::value> args = docopt::docopt(
    USAGE,
    { argv + 1, argv + argc },
    true,
    "NFC-Doorz Authentication Manager API v0.0.1"
    );
  logging::init(args, plog::info);

  if (!(args["--standalone"].asBool())) {
    client.connect(args["--ipc-connect"].asString());
    client.runThread();
  }

  sol::state lua;
  lua.open_libraries( sol::lib::base );

  lua.script( "print('bark bark bark!')" );

  return 0;
}
