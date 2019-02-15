#define _BSD_SOURCE
#include <termios.h>
#include <unistd.h>

#include <pty.h>
#include <seasocks/ResponseBuilder.h>
#include <seasocks/ResponseCode.h>
#include <nlohmann/json.hpp>

#include "lib/apps/api/handlers.hpp"
#include "lib/ipc-api.hpp"

void set_raw(int fd) {
  struct termios orig_termios;
  if (tcgetattr(fd, &orig_termios) < 0) {
    LOG_ERROR << "Failed to get termios";
  }
  cfmakeraw(&orig_termios);
  if (tcsetattr(fd, TCSANOW, &orig_termios) < 0) {
    LOG_ERROR << "Failed to set termios";
  }
}

namespace nfcdoorz::apps::api {
  using json = nlohmann::json;
  using namespace seasocks;
  using namespace std;
  using namespace nfcdoorz;


  std::shared_ptr<Response> MyPageHandler::status(const Request &request) {
    auto res = client.getStatusCall().gen();
    auto reply = res.get();

    json obj;
    json processes;
    for (auto &p: reply->processes) {
      json proc;
      proc["pid"] = p->pid;
      proc["name"] = p->name;
      proc["args"] = p->args;
      processes.push_back(proc);
    }

    obj["processes"] = processes;

    return Response::jsonResponse(obj.dump());
  }

  std::shared_ptr<Response> MyPageHandler::tty(const Request &request) {
    // if (!request.hasHeader("DeviceID")) {
    // return Response::error(ResponseCode::Unauthorized, "Unauthorized");
    // }
    return Response::unhandled();
  }

  std::shared_ptr<Response> MyPageHandler::handle(const Request &request) {
    // if (request.verb() != Request::Verb::Get)
    // return Response::unhandled();
    if (request.getRequestUri() == "/status") {
      return status(request);
    }
    if (request.getRequestUri() == "/tty") {
      return tty(request);
    }
    return Response::unhandled();
  }

  void TtySockHandler::onConnect(WebSocket *socket) {
    string devID = "yes";
    connections[devID] = socket;
    int master, slave;
    char name[128];
    memset(name, 0, 128);
    openpty(&master, &slave, nullptr, nullptr, nullptr);
    set_raw(master);
    set_raw(slave);
    ttyname_r(slave, name, 127);
    close(slave);

    auto ours = client.loop->resource<uvw::PollHandle>(master);
    ours->init();
    ours->start(uvw::Flags<uvw::PollHandle::Event>::from<
      uvw::PollHandle::Event::READABLE,
      uvw::PollHandle::Event::DISCONNECT
      >());

    fds[devID] = master;
    handles[devID] = ours;
    ours->on<uvw::ErrorEvent>([](const uvw::ErrorEvent &e, uvw::PollHandle &) {
      LOG_ERROR << e.what();
    });
    ours->on<uvw::PollEvent>([this, devID, socket, master](const uvw::PollEvent &ev, uvw::PollHandle &pipe) {
      if (ev.flags & uvw::PollHandle::Event::READABLE) {
        char *data = new char[256];
        auto len = read(master, data, 255);
        if (len <= 0) {
          return;
        }
        data[len] = 0;

        auto run = make_shared<Runnable>(socket, data, len);
        server.execute(run);
      }
      if (ev.flags & uvw::PollHandle::Event::DISCONNECT) {
        closeOut(devID);
      }
    });

    client.registerEventHandler(
      ipc::api::APIEvents::AuthenticatorExitEvent,
      [this, devID](auto &event) -> bool {
      auto exitEvent = event.AsAuthenticatorExitEvent();
      if (exitEvent->deviceID == devID) {
        closeOut(devID);
        return true;
      }
      return false;
    });

    auto res = client.getCreateAuthenticatorCall(name, devID).gen();
    auto reply = res.get();
    if (!reply->status) {
      LOG_ERROR << "Failed to create authenticator";
      socket->close();
      ours->close();
      close(master);
    } else {
      LOG_DEBUG << "Authenticator created";
    }
  }

  void TtySockHandler::onData(WebSocket *socket, const uint8_t *data, size_t length) {
    string devID = "yes"; // socket->getHeader("DeviceID");
    auto fd = fds.at(devID);
    size_t i = 0;
    while (i < length) {
      i += write(fd, data + i, length - i);
    }
  }

  void TtySockHandler::onDisconnect(WebSocket *socket) {
    LOG_DEBUG << "Disconnect";
    string devID = "yes"; // socket->getHeader("DeviceID");
    closeOut(devID);
  }

  void TtySockHandler::closeOut(std::string devID) {
    std::lock_guard<std::mutex> lock(close_mutex);
    LOG_DEBUG << "Close out";
    if (connections.count(devID)) {
      server.execute([connection = connections[devID]]() {
        connection->close();
      });
      connections.erase(devID);
    }
    if (handles.count(devID)) {
      handles[devID]->close();
      handles.erase(devID);
    }
    if (fds.count(devID)) {
      close(fds[devID]);
      fds.erase(devID);
    }
  }
}
