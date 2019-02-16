#include <iostream>
#include <filesystem>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <signal.h>

#include <kj/debug.h>
#include <kj/async.h>
#include <capnp/ez-rpc.h>
#include <capnp/message.h>

#include <uvw.hpp>

#include <docopt/docopt.h>

#include <seasocks/Logger.h>
#include <seasocks/PrintfLogger.h>
#include <seasocks/Server.h>

#include "lib/types.hpp"
#include "lib/logging.hpp"
#include "lib/config.hpp"
#include "lib/procmanager.hpp"
#include "lib/ipc/ipc.hpp"
#include "lib/ipc/server.hpp"

using namespace seasocks;
using namespace std;
using namespace nfcdoorz;
using namespace nfcdoorz::manager;

static const char USAGE[] =
  R"(NFC-Doorz Authentication Manager

      Usage:
        doorz-auth-manager [-v | -vv | -vvv] [options]
        doorz-auth-manager (-h | --help)
        doorz-auth-manager --version

      Options:
        -h --help               Show this help
        --version               Show version
        --config=CONFIG         Use config file [default: ./config.yaml]
        -v --verbose            Verbose output
        --api-port=PORT         API port [default: 9672]
        --api-host=HOST         API host [default: ::1]
  )";

proc::ProcManager procManager;

int main(int argc, const char *argv[]) {
  map<string, docopt::value> args = docopt::docopt(
    USAGE,
    { argv + 1, argv + argc },
    true,
    "NFC-Doorz Authentication Manager v0.0.1"
    );

  procManager.setRegisterPid([](auto &proc) {
    LOG_DEBUG << "registerPid " << proc.getName() << " " << proc.getPid();
    ipc::server->registerPid(proc.getName(), proc.getPid());
  });

  procManager.setUnregisterPid([](auto &proc) {
    LOG_DEBUG << "unregisterPid " << proc.getName() << " " << proc.getPid();
    ipc::server->unregisterPid(proc.getPid());
  });

  logging::init(args);

  config::Config config;

  try {
    config = config::Config::load(args);
  } catch (config::ValidationException e) {
    LOG_ERROR
      << e.what() << endl
      << "Invalid config node found" << endl;
    return 9;
  }

  if (!procManager.init(filesystem::path(argv[0]).parent_path() / "manager", ipc::server->pipe)) {
    LOG_ERROR << "Failed to init process manager";
    return 9;
  }

  {
    int server_sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    struct sockaddr_un server_sock_addr;
    socklen_t server_sock_addr_len;

    memset(&server_sock_addr, 0, sizeof(struct sockaddr_un));
    server_sock_addr.sun_family = AF_UNIX;

    if (bind(
      server_sock,
      (struct sockaddr *) &server_sock_addr,
      offsetof(struct sockaddr_un, sun_path)
      ) == -1) {
      LOG_ERROR << "Failed to bind " << errno;
      return false;
    }

    ipc::server->open(server_sock);
  }
  ipc::server->listen();
  // ipc::server->pipe->bind("");
  // ipc::server->pipe->listen();

  {
    procManager.create(
      ipc::api::Server::GetFullyQualifiedName(),
      "api",
      proc::ProcManager::filterArgs(args, "--api-"),
      true
      )->run();
  }

  if (0) {
    procManager.create(
      ipc::auth::Server::GetFullyQualifiedName(),
      "auth",
      proc::ProcManager::filterArgs(args, "--api-"),
      true
      )->run();
  }

  ipc::server->registerServiceHandler<ipc::auth::Server>();
  ipc::server->registerServiceHandler<ipc::policy::Server>();

  ipc::server->registerServiceHandler<ipc::api::Server>()
  ->registerHandler(
    ipc::api::APICalls::StatusCall,
    [](auto self, const ipc::api::APICallT &msg, auto reply, uvw::PipeHandle &) -> auto {
    LOG_DEBUG << "handle StatusCall";
    reply->msg.Set(ipc::api::StatusReplyT());
    auto s = reply->msg.AsStatusReply();
    uint32_t i = 0;
    for (auto p : procManager.processes) {
      auto proc = make_unique<ipc::api::ProcessT>();
      proc->pid = p->getPid();
      proc->name = p->getName();
      proc->args = p->getArgs();
      s->processes.push_back(move(proc));
    }
    return reply;
  }
    )
  ->registerHandler(
    ipc::api::APICalls::CreateAuthenticatorCall,
    [&args](auto self, const ipc::api::APICallT &msg, auto reply, uvw::PipeHandle &pipe) -> auto {
    using PromT = promise<unique_ptr<ipc::api::APIReplyT>>;

    LOG_DEBUG << "handle CreateAuthenticatorCall";
    auto call = msg.msg.AsCreateAuthenticatorCall();
    if (!call) {
      LOG_ERROR << "Invalid call";
      throw exception();
    }

    PromT prom;
    reply->msg.Set(ipc::api::CreateAuthenticatorReplyT());
    auto s = reply->msg.AsCreateAuthenticatorReply();

    vector<string> arg_vec = proc::ProcManager::filterArgs(args, "--auth-");
    arg_vec.push_back(string("pn532_uart:") + call->ttyName);

    auto proc = procManager.create(
      ipc::auth::Server::GetFullyQualifiedName(),
      "auth",
      arg_vec,
      false
      );
    proc->setOnDestroy([self, deviceID = call->deviceID, &pipe](const proc::Proc &) {
      ipc::api::APIEventsUnion eventUnion;
      eventUnion.Set(ipc::api::AuthenticatorExitEventT());
      auto event = eventUnion.AsAuthenticatorExitEvent();

      event->deviceID = deviceID;

      self->sendEvent(eventUnion, pipe);
    });
    proc->run();

    // if (pending) {
    s->status = true;
    // }
    prom.set_value(move(reply));

    return prom.get_future();
  }
    );

  ipc::server->run();
  return 0;
}
