#include "ipc.hpp"

namespace nfcdoorz::ipc {

  class IpcServerBase : public virtual IpcBase {
  public:
    void listen();
  protected:
    virtual void handleCall(const int &pid, const uvw::DataEvent &ev, uvw::PipeHandle &sock) = 0;
  };

  template<typename I, typename... Rest>
  constexpr inline void initialize_map(std::map<std::string, auto> &map) {
    map.try_emplace(I::GetFullyQualifiedName());
    std::cout << I::GetFullyQualifiedName() << std::endl;
    if constexpr (sizeof...(Rest)) {
      initialize_map<Rest...>(map);
    }
  }

  // template<typename APIServer>
  // class IpcServiceHandler;

  template<typename... APIServers>
  class IpcServerMulti:
    public IpcServerBase,
    public virtual IpcBase,
    public std::enable_shared_from_this<IpcServerMulti<APIServers...>>
  {
    // virtual std::shared_ptr<IpcServerMulti> shared_from_this();

    std::shared_ptr<IpcServerMulti> getptr() {
        return std::enable_shared_from_this<IpcServerMulti<APIServers...>>::shared_from_this();
    }
    template<typename APIServer>
    class IpcServiceHandler:
      public std::enable_shared_from_this<IpcServiceHandler<APIServer>>
    {
      friend class IpcServerMulti;
      friend class IpcServerMulti<APIServers...>;
      std::shared_ptr<IpcServerMulti> _server;
      IpcServiceHandler(std::shared_ptr<IpcServerMulti> server): _server(server) {}
      std::shared_ptr<IpcServiceHandler> getptr() {
          return std::enable_shared_from_this<IpcServiceHandler<APIServer>>::shared_from_this();
      }
    public:
      using shared_ptr_self = std::shared_ptr<IpcServiceHandler<APIServer>>;
      using APICall = std::remove_pointer_t<decltype(std::declval<APIServer>().call())>;
      using APICallT = typename APICall::NativeTableType;
      using APICallTPtr = std::unique_ptr<APICallT>;
      using APICallsEnum = std::remove_pointer_t<decltype(std::declval<APICall>().msg_type())>;
      using APIReply = std::remove_pointer_t<decltype(std::declval<APIServer>().reply())>;
      using APIRepliesEnum = std::remove_pointer_t<decltype(std::declval<APIReply>().msg_type())>;
      using APIEventsEnum = std::remove_pointer_t<decltype(std::declval<APIReply>().event_type())>;
      using APIReplyT = typename APIReply::NativeTableType;
      using APIReplyTPtr = std::unique_ptr<APIReplyT>;
      using APIEventsUnion = decltype(APIReplyT::event);

      using MessageHandlerFuture_t = std::function<
        std::future<APIReplyTPtr> (shared_ptr_self, const APICallT &, APIReplyTPtr, uvw::PipeHandle &)
      >;
      using MessageHandler_t = std::function<
        APIReplyTPtr (shared_ptr_self, const APICallT &, APIReplyTPtr, uvw::PipeHandle &)
      >;

      shared_ptr_self registerHandler(APICallsEnum type, MessageHandlerFuture_t handler) {
        handlers[type] = handler;
        return getptr();
      }

      shared_ptr_self registerHandler(APICallsEnum type, MessageHandler_t handler) {
        handlers[type] = [handler](shared_ptr_self self, APICallT t, APIReplyTPtr r, uvw::PipeHandle &p) {
          std::promise<APIReplyTPtr> prom;
          prom.set_value(std::move(handler(self, t, std::move(r), p)));
          return prom.get_future();
        };
        return getptr();
      }

      void sendEvent(APIEventsUnion &event, uvw::PipeHandle &pipe) {
        APIReplyTPtr reply = std::make_unique<APIReplyT>();
        reply->event = event;

        flatbuffers::FlatBufferBuilder builder(1024);

        builder.Finish(APIReply::Pack(builder, reply.get()));

        size_t size = builder.GetSize();
        std::unique_ptr<char[]> data = std::make_unique<char[]>(size);
        memcpy(data.get(), builder.GetBufferPointer(), size);
        pipe.write(move(data), size);
      }

    protected:
      struct CallReplyCallback : public IpcServerMulti::Executable {
        CallReplyCallback(
          uint64_t _call_id,
          std::shared_ptr<std::future<APIReplyTPtr>> _future,
          uvw::PipeHandle &_pipe,
          shared_ptr_self _handler
        ):
          call_id(_call_id),
          future(_future),
          pipe(_pipe),
          handler(_handler)
          {
            setTimeout(1000);
          }

        uint64_t call_id;
        std::shared_ptr<std::future<APIReplyTPtr>> future;
        uvw::PipeHandle &pipe;
        shared_ptr_self handler;

        virtual bool run() override {
          // LOG_ERROR << "msg callback here";
          if (future->wait_for(std::chrono::seconds(0)) != std::future_status::timeout) {
            APIReplyTPtr reply = future->get();
            reply->id = call_id;

            flatbuffers::FlatBufferBuilder builder(1024);

            builder.Finish(APIReply::Pack(builder, reply.get()));

            size_t size = builder.GetSize();
            std::unique_ptr<char[]> data = std::make_unique<char[]>(size);
            memcpy(data.get(), builder.GetBufferPointer(), size);
            pipe.write(move(data), size);
            return true;
          }
          return false;
        }
      };

      void handleCall(const int &pid, const uvw::DataEvent &ev, uvw::PipeHandle &pipe) {
        LOG_DEBUG << "IpcServiceHandler::handleCall";
        uint8_t *data = (uint8_t *)ev.data.get();
        flatbuffers::BufferRef<APICall> buf(data, ev.length);
        if (!buf.Verify()) {
          std::cout << "verify failed" << std::endl;
          return;
        }

        typename APICall::NativeTableType call;
        buf.GetRoot()->UnPackTo(&call);
        uint64_t call_id = call.id;

        auto msg_type = call.msg.type;

        if (handlers.count(msg_type)) {
          auto reply = std::make_unique<APIReplyT>();
          auto future = std::make_shared<std::future<APIReplyTPtr>>(
            handlers[msg_type](
              getptr(),
              call,
              std::move(reply),
              pipe
            )
          );

          _server->registerIdleCall(std::make_shared<CallReplyCallback>(
            call_id, future, pipe, getptr()
          ));
        }
      }
      std::map<APICallsEnum, MessageHandlerFuture_t> handlers;
    };
    using VariantT = std::variant<std::monostate, std::shared_ptr<IpcServiceHandler<APIServers>>...>;
  public:
    IpcServerMulti() {
      initialize_map<APIServers...>(handlers);
    }

    template<typename APIServer, typename... Args>
    std::shared_ptr<IpcServiceHandler<APIServer>> registerServiceHandler(Args... args) {
      std::shared_ptr<IpcServiceHandler<APIServer>> handler(
        new IpcServiceHandler<APIServer>(getptr(), args...)
      );
      handlers[APIServer::GetFullyQualifiedName()] = handler;
      return handler;
    }

    void registerPid(const std::string &name, const int &pid) {
      pid_map[pid] = name;
    }

    void unregisterPid(const int &pid) {
      pid_map.erase(pid);
    }
  protected:
    virtual void handleCall(const int &pid, const uvw::DataEvent &ev, uvw::PipeHandle &sock) override {
      if (!pid_map.count(pid)) {
        LOG_ERROR << "Pid " << pid << " not registered";
        return;
      }
      auto pid_handler = pid_map.at(pid);
      if (!handlers.count(pid_handler)) {
        LOG_ERROR << "Pid " << pid << " has invalid handler " << pid_handler;
        return;
      }

      std::visit(
        overloaded {
          [] (std::monostate &handler) {
            // Ignore monostate
            LOG_ERROR << "handleCall hit a std::monostate";
          },
          [&pid, &ev, &sock] (auto &handler) {
            handler->handleCall(pid, ev, sock);
          }
        },
        handlers.at(pid_map.at(pid))
      );
    }
    std::map<std::string, VariantT> handlers;
    std::map<int, std::string> pid_map;
  };

  extern std::shared_ptr<IpcServerMulti<api::Server, auth::Server>> server;
}
