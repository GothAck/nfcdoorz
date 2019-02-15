#include "ipc-client.hpp"

namespace nfcdoorz::ipc {
  using namespace std;

  bool IpcClientBase::connect(string base64_socket_addr) {
    auto connect_data = base64::decode(base64_socket_addr);

    int child_sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    const int optval = 1;
    if (setsockopt(child_sock, SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval)) == -1) {
      LOG_ERROR << "Failed to set SO_PASSCRED";
      return false;
    }

    struct sockaddr_un server_sock_addr;
    server_sock_addr.sun_family = AF_UNIX;
    memcpy(server_sock_addr.sun_path, connect_data.data(), connect_data.size());

    socklen_t server_sock_addr_len = offsetof(struct sockaddr_un, sun_path);
    server_sock_addr_len += connect_data.size();

    if (::connect(
      child_sock,
      (struct sockaddr *)&server_sock_addr,
      server_sock_addr_len
    ) == -1) {
      LOG_ERROR << "Failed to connect to parent socket";
      close(child_sock);
      return false;
    }

    setFd(child_sock);
    return true;
  }

  uint64_t IpcClientBase::getNextID() {
    return _next_id++;
  }

  void IpcClientBase::runThread() {
    _thread = thread([this]() {
      auto client = loop->resource<uvw::PipeHandle>(true);

      client->on<uvw::ErrorEvent>([](const uvw::ErrorEvent &e, uvw::PipeHandle &) { LOG_ERROR << e.what(); });
      client->on<uvw::CloseEvent>([](const uvw::CloseEvent &, uvw::PipeHandle &handle) { handle.close(); });
      client->on<uvw::EndEvent>([](const uvw::EndEvent &, uvw::PipeHandle &sock) { sock.close(); });
      client->on<uvw::DataEvent>([this](const uvw::DataEvent &ev, uvw::PipeHandle &sock) {
        onDataEvent(ev, sock);
      });
      async->on<uvw::AsyncEvent>([this, client](const uvw::AsyncEvent &, uvw::AsyncHandle &) {
        onAsyncEvent(client);
      });
      client->open(_fd);
      client->read();
      loop->run();
    });
  }

  // template<class APIServer>
  // template <typename CallType, typename ReplyType, typename ReturnTrait>
  // APIRepliesEnum IpcClient::Callable::replyEnum = ReturnSubtypeToEnum<ReplyType>();

}
