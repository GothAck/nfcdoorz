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
#include "lib/logging.hpp"
#include "lib/types.hpp"

#include "flatbuf/ipc-common_generated.h"
#include "flatbuf/ipc-auth_generated.h"
#include "flatbuf/ipc-api_generated.h"
#include "flatbuf/ipc-policy_generated.h"

#pragma once

namespace nfcdoorz::ipc {
  using base64 = cppcodec::base64_rfc4648;
  using PipePtr = std::shared_ptr<uvw::PipeHandle>;

  /*! \brief Base of all IPC classes.
   *  Contains the common required components for running IPC communications.
   */
  class IpcBase {
public:
    IpcBase() :
      loop(uvw::Loop::getDefault()),
      pipe(loop->resource<uvw::PipeHandle>(true)),
      idle(loop->resource<uvw::IdleHandle>()),
      nextId(0)
    {
    }
    /*! \brief Open a file descriptor.
     *  As libuv pipe does not support abstract socket opening, we DIY.
     */
    void open(int fd);
    /*! \brief Start event loop.
     *  This does not return, see UVW/libuv docs.
     */
    void run();

    /*! \brief Base for executables run in event loop idle.
     *  Used for cross thread execution in IPCClient.
     *  call registerIdleCall with an object subclassed from this.
     *  This executable will be rescheduled every event loop iteration
     *  until run() returns true.
     */
    struct Executable {
      friend class IpcBase;
      /*! \brief Set callback timeout.
       *  Override default timeout of this Executable.
       */
      virtual void setTimeout(uint32_t) final;

      /*! \brief Override this method.
       */
      virtual bool run();
private:
      virtual bool runWithTimeout() final;
      std::chrono::time_point<std::chrono::system_clock> start =
        std::chrono::system_clock::now();
      uint32_t timeout = 500;
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
