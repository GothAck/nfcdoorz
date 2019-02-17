#include <thread>
#include <future>
#include <any>
#include <iostream>
#include <optional>

#include <kj/debug.h>
#include <capnp/ez-rpc.h>
#include <capnp/message.h>
#include <capnp/compat/json.h>

#include <uvw.hpp>

#include <docopt/docopt.h>

#include <seasocks/Logger.h>
#include <seasocks/Server.h>
#include "lib/seasocks-plog.hpp"

#include "lib/config.hpp"
#include "lib/types.hpp"
#include "lib/logging.hpp"
#include "lib/ipc/api.hpp"

#include "lib/apps/api/handlers.hpp"

using namespace seasocks;
using namespace std;
using namespace nfcdoorz;
using namespace nfcdoorz::apps::api;

auto logger = make_shared<PlogLogger>();
Server server(logger);

ipc::IpcClientAPI client;

static const char USAGE[] =
  R"(NFC-Doorz Authentication Manager API

      Usage:
        doorz-auth-manager [-v | -vv | -vvv] [options]
        doorz-auth-manager (-h | --help)
        doorz-auth-manager --version

      Options:
        -h --help               Show this help
        --version               Show version
        -v --verbose            Verbose output
        --api-port=PORT         API port [default: 9672]
        --api-host=HOST         API host [default: ::1]
        )" MANAGER_CLIENT_ARGS R"(
  )";

int main(int argc, const char *argv[]) {
  map<string, docopt::value> args = docopt::docopt(
    USAGE,
    { argv + 1, argv + argc },
    true,
    "NFC-Doorz Authentication Manager API v0.0.1"
    );
  logging::init(args, plog::info);

  config::Config config;

  client.connect(args["--ipc-connect"].asString());
  client.runThread();

  auto configString = client.getConfigCall().gen().get()->config;

  config.parse(configString);

  server.addPageHandler(std::make_shared<MyPageHandler>(client));
  server.addWebSocketHandler("/tty", make_shared<TtySockHandler>(client, server));

  server.serve("web", args["--api-port"].asLong());

  return 0;
}
