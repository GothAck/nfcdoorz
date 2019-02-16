#include "server.hpp"

namespace nfcdoorz::ipc {
  using namespace std;

  std::shared_ptr<IpcServerMulti<api::Server, auth::Server, policy::Server>> server =
    make_shared<IpcServerMulti<api::Server, auth::Server, policy::Server>>();

  void IpcServerBase::listen() {
    pipe->on<uvw::ListenEvent>([this](const uvw::ListenEvent &, uvw::PipeHandle &handle) {
      shared_ptr<uvw::PipeHandle> socket = handle.loop().resource<uvw::PipeHandle>(true);
      int pid = -1;
      handle.accept(*socket);
      {
        struct ucred ucred;
        socklen_t len = sizeof(struct ucred);
        if (getsockopt(socket->fileno(), SOL_SOCKET, SO_PEERCRED, &ucred, &len) == -1) {
          LOG_ERROR << "Get pid failed";
          socket->close();
          return;
        }
        pid = ucred.pid;
      }
      LOG_DEBUG << "Accept " << pid;

      socket->on<uvw::ErrorEvent>([](const uvw::ErrorEvent &e, uvw::PipeHandle &) {
        LOG_ERROR << "err: " << e.what();
      });
      socket->on<uvw::CloseEvent>([](const uvw::CloseEvent &, uvw::PipeHandle &pipe) {
        LOG_DEBUG << "disconnect";
        pipe.close();
      });
      socket->on<uvw::EndEvent>([](const uvw::EndEvent &, uvw::PipeHandle &pipe) {
        LOG_DEBUG << "end";
        pipe.close();
      });
      socket->on<uvw::DataEvent>([this, pid](const uvw::DataEvent &ev, uvw::PipeHandle &sock) {
        handleCall(pid, ev, sock);
      });

      socket->read();
    });

    pipe->listen();
  }
}
