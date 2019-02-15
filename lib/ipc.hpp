#include <iostream>
#include <utility>
#include <functional>
#include <future>
#include <deque>
#include <map>
#include <memory>
#include <variant>
#include <type_traits>
#include <typeinfo>
#include <cxxabi.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

#include <sys/signal.h>

#include <cppcodec/base64_rfc4648.hpp>
#include <uvw.hpp>
#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/minireflect.h>

#include <iostream>
#include "logging.hpp"
#include "types.hpp"

#include "flatbuf/ipc-auth_generated.h"
#include "flatbuf/ipc-api_generated.h"

#pragma once

namespace nfcdoorz::ipc {
  using base64 = cppcodec::base64_rfc4648;
  using PipePtr = std::shared_ptr<uvw::PipeHandle>;

  class IpcBase {
public:
    IpcBase() :
      loop(uvw::Loop::getDefault()),
      pipe(loop->resource<uvw::PipeHandle>(true)),
      idle(loop->resource<uvw::IdleHandle>()),
      nextId(0)
    {
    }
    void open(int fd);
    void run();

    struct Executable {
      std::chrono::time_point<std::chrono::system_clock> start =
        std::chrono::system_clock::now();
      uint32_t timeout = 500;
      void setTimeout(uint32_t);
      bool runWithTimeout();
      virtual bool run();
    };

    void registerIdleCall(std::shared_ptr<Executable> exec);
    std::optional<std::pair<PipePtr, PipePtr>> socketpair();
    std::shared_ptr<uvw::Loop> loop;
    std::shared_ptr<uvw::IdleHandle> idle;
    PipePtr pipe;
    uint64_t nextId;
protected:
    std::list<std::shared_ptr<Executable>> idle_calls;
  };

}
