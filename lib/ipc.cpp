#include "ipc.hpp"

namespace nfcdoorz::ipc {
  using namespace std;
  using namespace nfcdoorz;

  void IpcBase::open(int fd) {
    pipe->open(fd);
  }

  void IpcBase::run() {
    idle->on<uvw::ErrorEvent>([this](uvw::ErrorEvent &e, uvw::IdleHandle &){
      LOG_ERROR << "IdleHandle ErrorEvent";
    });
    idle->on<uvw::IdleEvent>([this](uvw::IdleEvent &, uvw::IdleHandle &){
      auto it = idle_calls.begin();
      while (it != idle_calls.end()) {
        auto result = (*it)->runWithTimeout();
        if (result) {
          it = idle_calls.erase(it);
        } else {
          it++;
        }
      }
    });
    idle->init();
    idle->start();
    loop->run();
  }

  void IpcBase::registerIdleCall(shared_ptr<Executable> exec) {
    idle_calls.push_back(exec);
  }
  
  void IpcBase::Executable::setTimeout(uint32_t new_timeout) {
    timeout = new_timeout;
  }

  bool IpcBase::Executable::runWithTimeout() {
    if (
      timeout > 0 &&
      std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start
      ).count() >= timeout
    ) {
      LOG_ERROR << "Idle Executable timed out";
      return true;
    }
    return run();
  }

  bool IpcBase::Executable::run() {
    return true;
  }

  optional<pair<PipePtr, PipePtr>> IpcBase::socketpair() {
    int unixSockets[2];
    if(::socketpair(AF_UNIX, SOCK_STREAM, 0, unixSockets) == -1) {
      return nullopt;
    }

    auto pipeHandles = pair(
      loop->resource<uvw::PipeHandle>(),
      loop->resource<uvw::PipeHandle>()
    );
    get<0>(pipeHandles)->init();
    get<0>(pipeHandles)->open(unixSockets[0]);
    get<1>(pipeHandles)->init();
    get<1>(pipeHandles)->open(unixSockets[1]);

    return pipeHandles;
  }
}
