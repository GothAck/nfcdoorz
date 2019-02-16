#include <mutex>
#include <utility>
#include <functional>
#include <future>
#include <queue>
#include <map>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>

#include <uvw.hpp>
#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/minireflect.h>

#include "lib/logging.hpp"
#include "lib/types.hpp"
#include "ipc.hpp"

#pragma once

#define MANAGER_CLIENT_ARGS "--ipc-connect=BASE64_ADDRESS            Manager IPC Address"

namespace nfcdoorz::ipc {
  class IpcClientBase : public virtual IpcBase {
public:
    IpcClientBase() :
      IpcBase(),
      async(loop->resource<uvw::AsyncHandle>())
    {
      async->init();
    }
    bool connect(std::string base64_socket_addr);
    void setFd(int fd) {
      _fd = fd;
    }
    void runThread();


protected:
    uint64_t getNextID();
    virtual void onAsyncEvent(std::shared_ptr<uvw::PipeHandle>) = 0;
    virtual void onDataEvent(const uvw::DataEvent &ev, uvw::PipeHandle &sock) = 0;
    std::thread _thread;
    std::shared_ptr<uvw::AsyncHandle> async;
    int _fd;
    uint64_t _next_id = 0;
  };

  template<class APIServer>
  class IpcClient : public IpcClientBase, public virtual IpcBase {
public:
    using APICall = std::remove_pointer_t<decltype(std::declval<APIServer>().call())>;
    using APICallT = typename APICall::NativeTableType;
    using APICallsEnum = std::remove_pointer_t<decltype(std::declval<APICall>().msg_type())>;
    using APIReply = std::remove_pointer_t<decltype(std::declval<APIServer>().reply())>;
    using APIRepliesEnum = std::remove_pointer_t<decltype(std::declval<APIReply>().msg_type())>;
    using APIEventsEnum = std::remove_pointer_t<decltype(std::declval<APIReply>().event_type())>;
    using APIReplyT = typename APIReply::NativeTableType;
    using APIEventsUnion = decltype(APIReplyT::event);
    using EventCallback = std::function<bool (APIEventsUnion &)>;

    template<typename CallType, typename ReplyType>
    struct Callable {
      friend class IpcClient;
      CallType *value;
      std::optional<std::shared_ptr<uvw::PipeHandle>> sendPipe;

      Callable &set(std::function<void (CallType *)> cb) {
        cb(value);
        return *this;
      }

      std::future<std::unique_ptr<ReplyType>> gen() {
        auto res = c.makeCall(t, sendPipe);
        std::promise<std::unique_ptr<ReplyType>> prom;

        if (res.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
          LOG_ERROR << "Timeout";
          prom.set_exception(std::make_exception_ptr(std::exception()));
          return prom.get_future();
        }

        APIReplyT obj = res.get();
        if (obj.msg.type != static_cast<APIRepliesEnum>(call_enum)) {
          LOG_ERROR << "Incorrect return type";
          prom.set_exception(std::make_exception_ptr(std::exception()));
          return prom.get_future();
        }
        ReplyType *replyPtr = reinterpret_cast<ReplyType *>(obj.msg.value);
        auto ptr = std::unique_ptr<ReplyType>();
        ptr.reset(new ReplyType(std::move(*replyPtr)));
        // memcpy(ptr.get(), reply, sizeof(ReplyType));
        prom.set_value(std::move(ptr));

        return prom.get_future();
      }
private:
      Callable(uint64_t id, IpcClient &client) : c(client) {
        t.msg.Set(CallType());
        call_enum = t.msg.type;
        value = reinterpret_cast<CallType *>(t.msg.value);
        t.id = id;

        // Ensure reply is same position as call in flatbuffer union
        // Wish I could make this check at compile time
        auto replyEnum = APIRepliesEnum::NONE;
        {
          APIReplyT reptest;
          reptest.msg.Set(ReplyType());
          replyEnum = reptest.msg.type;
        }
        assert(replyEnum == static_cast<APIRepliesEnum>(call_enum));
      };
      APICallT t;
      APICallsEnum call_enum = APICallsEnum::NONE;
      IpcClient &c;
    };

    template<typename TCall, typename TRet>
    Callable<TCall, TRet> getCall() {
      std::lock_guard<std::mutex> lock(call_mutex);
      Callable<TCall, TRet> ret(getNextID(), *this);
      return ret;
    }

    void registerEventHandler(APIEventsEnum event_type, EventCallback cb) {
      if (!event_callbacks.count(event_type)) event_callbacks[event_type] = {};
      event_callbacks[event_type].push_back(std::move(cb));
    }

protected:
    std::mutex call_mutex;
    std::queue<std::pair<APICallT, std::optional<std::shared_ptr<uvw::PipeHandle>>>> calls_out_queue;
    std::map<uint64_t, std::promise<APIReplyT>> call_returns;
    std::map<APIEventsEnum, std::vector<EventCallback>> event_callbacks;
    // std::optional<std::promise<APIReplyT>> current_call;

    std::future<APIReplyT> makeCall(APICallT call, std::optional<std::shared_ptr<uvw::PipeHandle>> pipe) {
      std::promise<APIReplyT> prom;
      auto future = prom.get_future();

      call_returns[call.id] = std::move(prom);
      calls_out_queue.emplace(std::move(call), pipe);

      async->send();
      return future;
    }

    void onAsyncEvent(std::shared_ptr<uvw::PipeHandle> pipe) override {
      while (!calls_out_queue.empty()) {
        auto call_pair = calls_out_queue.front();
        calls_out_queue.pop();
        APICallT call = call_pair.first;
        auto optionalpipe = call_pair.second;
        flatbuffers::FlatBufferBuilder builder(1024);

        builder.Finish(APICall::Pack(builder, &call));

        size_t size = builder.GetSize();
        std::unique_ptr<char[]> data = std::make_unique<char[]>(size);
        memcpy(data.get(), builder.GetBufferPointer(), size);
        if (optionalpipe) {
          LOG_WARNING << "Sending with pipe";
          auto &send = *(optionalpipe->get());
          pipe->write(send, move(data), size);
        } else {
          pipe->write(move(data), size);
        }
      }
    }

    void onDataEvent(const uvw::DataEvent &ev, uvw::PipeHandle &sock) override {
      LOG_DEBUG << "onDataEvent";
      uint8_t *data = (uint8_t *) ev.data.get();
      flatbuffers::BufferRef<APIReply> buf(data, ev.length);
      if (!buf.Verify()) {
        LOG_ERROR << "verify failed";
        return;
      }
      APIReplyT reply;
      buf.GetRoot()->UnPackTo(&reply);

      // Handle reply
      if (reply.msg.type != APIRepliesEnum::NONE) {
        uint64_t call_id = reply.id;
        if (call_returns.count(call_id)) {
          auto &prom = call_returns[call_id];
          prom.set_value(std::move(reply));
          call_returns.erase(call_id);
        }
      }

      // Handle event
      if (reply.event.type != APIEventsEnum::NONE) {
        auto event_type = reply.event.type;
        if (event_callbacks.count(event_type)) {
          auto &cbs = event_callbacks[event_type];
          auto it = cbs.begin();
          while (it != cbs.end()) {
            if ((*it)(reply.event)) {
              it = cbs.erase(it);
            } else {
              it++;
            }
          }
        }
      }
    }
  };
}
