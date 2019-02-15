#include <memory>
#include <map>
#include <string>

#include <seasocks/PageHandler.h>
#include <seasocks/Response.h>
#include <seasocks/Request.h>
#include <seasocks/Server.h>
#include <seasocks/WebSocket.h>
#include "lib/ipc-api.hpp"

#pragma once

namespace nfcdoorz::apps::api {

  class MyPageHandler : public seasocks::PageHandler {
    std::shared_ptr<seasocks::Response> tty(const seasocks::Request &request);
    nfcdoorz::ipc::IpcClientAPI &client;
public:
    MyPageHandler(nfcdoorz::ipc::IpcClientAPI &_client) : client(_client) {
    };
    virtual std::shared_ptr<seasocks::Response> handle(const seasocks::Request &request) override;

private:
    std::shared_ptr<seasocks::Response> status(const seasocks::Request &request);
  };

  class TtySockHandler : public seasocks::WebSocket::Handler {
    nfcdoorz::ipc::IpcClientAPI &client;
public:
    TtySockHandler(nfcdoorz::ipc::IpcClientAPI &_client, seasocks::Server &_server) :
      client(_client),
      server(_server)
    {
    }

    struct Runnable : seasocks::Server::Runnable {
      Runnable(seasocks::WebSocket *ws, char *data, size_t length) :
        websocket(ws),
        len(length),
        dataPtr((uint8_t *) malloc(length))
      {
        if (!dataPtr) throw std::exception();
        memcpy(dataPtr, data, length);
      }
      ~Runnable() {
        free(dataPtr);
      }

      seasocks::WebSocket *websocket;
      size_t len;
      uint8_t *dataPtr;

      void run() {
        websocket->send(dataPtr, len);
      }
    };

    void onConnect(seasocks::WebSocket *socket) override;
    void onData(seasocks::WebSocket *socket, const uint8_t *data, size_t length) override;
    void onDisconnect(seasocks::WebSocket *socket) override;

private:
    seasocks::Server &server;
    std::map<std::string, seasocks::WebSocket *> connections;
    std::map<std::string, int> fds;
    std::map<std::string, std::shared_ptr<uvw::PollHandle>> handles;

    std::mutex close_mutex;
    void closeOut(std::string devID);
  };
}
