#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include <string>
#include <filesystem>
#include <map>
#include <optional>
#include <functional>
#include <uvw.hpp>
#include <cppcodec/base64_rfc4648.hpp>

#include <docopt/docopt.h>

#pragma once

namespace nfcdoorz::manager::proc {
  class Proc;
  using base64 = cppcodec::base64_rfc4648;

  using ProcessCallbackFn = std::function<void (const Proc &proc)>;

  class ProcManager {
    friend class Proc;
    std::shared_ptr<uvw::Loop> loop;
    std::shared_ptr<uvw::IdleHandle> idle;
    std::optional<ProcessCallbackFn> registerPid;
    std::optional<ProcessCallbackFn> unregisterPid;
    std::filesystem::path _exec_path;
    std::optional<std::shared_ptr<uvw::PipeHandle>> _server_pipe;
public:
    ProcManager() :
      loop(uvw::Loop::getDefault()),
      idle(loop->resource<uvw::IdleHandle>())
    {
      idle->start();
    }

    bool init(std::filesystem::path exec_path, std::shared_ptr<uvw::PipeHandle> server_pipe);
    void killall();

    void setRegisterPid(ProcessCallbackFn fn) {
      registerPid = fn;
    }

    void setUnregisterPid(ProcessCallbackFn fn) {
      unregisterPid = fn;
    }

    std::shared_ptr<Proc> create(
      const std::string &name,
      const std::string &exec,
      const std::vector<std::string> &args,
      bool restart = false,
      std::optional<std::shared_ptr<uvw::PipeHandle>> pipe = std::nullopt,
      bool disable_server_pipe = false
      );

    // int server_sock;
    static std::vector<std::shared_ptr<Proc>> processes;

    static std::vector<std::string> filterArgs(
      std::map<std::string, docopt::value> &args,
      const std::string &beginning
      );
  };

  class Proc {
public:
    friend class ProcManager;
    Proc(
      ProcManager &procManager,
      const std::string &name,
      const std::string &exec,
      const std::vector<std::string> &args,
      std::optional<std::shared_ptr<uvw::PipeHandle>> pipe,
      bool restart,
      bool disable_server_pipe
      ) :
      _procManager(procManager),
      _name(name),
      _running(false),
      _exec(exec),
      _args(args),
      _restart(restart),
      _failures(0),
      _pipe(pipe),
      _disable_server_pipe(disable_server_pipe)
    {
    }
    ~Proc();
    void setPreExec(std::function<bool()> cb);
    void kill(int signum);
    bool run(std::optional<std::function<void(Proc &)>> callback = std::nullopt);
    bool isRunning();
    void remove();
    const std::string &getName() const;
    int getPid() const;
    const std::vector<std::string> getArgs() const;

    void setOnDestroy(ProcessCallbackFn fn) {
      _onDestroy = fn;
    }
private:
    void exited();
    ProcManager &_procManager;
    std::optional<std::shared_ptr<uvw::ProcessHandle>> _handle;
    std::string _name;
    std::string _exec;
    std::vector<std::string> _args;
    std::optional<std::function<bool()>> _preExec;
    bool _running;
    bool _restart;
    uint32_t _failures;
    std::optional<std::shared_ptr<uvw::PipeHandle>> _pipe;
    bool _disable_server_pipe;
    std::optional<ProcessCallbackFn> _onDestroy;
  };

}
