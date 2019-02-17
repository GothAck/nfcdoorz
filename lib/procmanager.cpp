#include "procmanager.hpp"
#include "logging.hpp"

namespace nfcdoorz::manager::proc {
  using namespace std;

// optional<ProcManager> ProcManager::_singleton;
  decltype(ProcManager::processes) ProcManager::processes;

  Proc::~Proc() {
    if (_onDestroy) {
      (*_onDestroy)(*this);
    }
    LOG_WARNING << "Process deleted";
    _restart = false;
    kill(SIGKILL);
  }

  void Proc::kill(int signum) {
    if (_handle) {
      _handle->get()->kill(signum);
    }
  }

  vector<string> ProcManager::filterArgs(
    map<string, docopt::value> &args,
    const string &beginning
    ) {
    vector<string> ret;
    const size_t len = beginning.length();
    for (auto &a: args) {
      if (a.first.substr(0, len) == beginning) {
        ret.push_back(a.first + "=" + a.second.asString());
      }
    }
    int v = args["--verbose"].asLong();
    if (v) {
      string verbose = "-";
      for (int i = 0; i < v; i++) {
        verbose += "v";
      }
      ret.push_back(verbose);
    }

    return ret;
  }

  bool ProcManager::init(std::filesystem::path exec_path, std::shared_ptr<uvw::PipeHandle> server_pipe) {
    _exec_path = exec_path;
    _server_pipe = server_pipe;
    return true;
  }

  void ProcManager::killall() {
    for (auto &p: processes) {
      p->kill(SIGTERM);
    }
  }

  void Proc::exited() {
    // LOG_WARNING << "Process exited: " << _exec << " pid " << _pid;
    _running = false;

    if (_procManager.unregisterPid) {
      auto fn = *_procManager.unregisterPid;
      fn(*this);
    }
    if (_handle) {
      _handle->get()->close();
    }
    _handle = nullopt;
    bool to_remove = true;
    if (_restart) {
      if (++_failures < 5) {
        to_remove = false;
        run();
      }
      else {
        LOG_ERROR << "Process failed 5 times, refusing to restart";
      }
    }
    if (to_remove) {
      remove();
    }
  }

  void Proc::remove() {
    for (auto it = ProcManager::processes.begin(); it < ProcManager::processes.end(); it++) {
      if (it->get() == this) {
        ProcManager::processes.erase(it);
        break;
      }
    }
  }

  std::shared_ptr<Proc> ProcManager::create(
    const string &name,
    const string &exec,
    const vector<string> &args,
    bool restart,
    optional<shared_ptr<uvw::PipeHandle>> pipe,
    bool disable_server_pipe
    ) {
    string exec_full = _exec_path / exec;
    LOG_INFO << exec_full;
    auto proc = make_shared<Proc>(*this, name, exec_full, args, pipe, restart, disable_server_pipe);
    processes.push_back(proc);
    LOG_INFO << "wut";
    return proc;
  }

  void Proc::setPreExec(function<bool()> cb) {
    _preExec = cb;
  }

  bool Proc::run(optional<function<void(Proc &)>> callback) {
    if (_handle) {
      return false;
    }
    auto handle = _procManager.loop->resource<uvw::ProcessHandle>();
    _handle = handle;
    // int child_sock = -1;
    std::optional<std::string> server_pipe;
    if (_procManager._server_pipe && !_disable_server_pipe) {
      server_pipe = base64::encode(_procManager._server_pipe->get()->sock());
    }
    if (_pipe) {
      auto shared_pipe = *_pipe;
      LOG_INFO << "Handing over pipe";
      handle->stdio(
        *shared_pipe,
        uvw::Flags<uvw::ProcessHandle::StdIO>::from<
          uvw::ProcessHandle::StdIO::INHERIT_FD
          >()
        );
    }
    handle->stdio(
      1,
      uvw::Flags<uvw::ProcessHandle::StdIO>::from<
        uvw::ProcessHandle::StdIO::INHERIT_FD
        >()
      );
    handle->stdio(
      2,
      uvw::Flags<uvw::ProcessHandle::StdIO>::from<
        uvw::ProcessHandle::StdIO::INHERIT_FD
        >()
      );

    vector<string> args = _args;
    if (server_pipe) {
      args.push_back(string("--ipc-connect=").append(*server_pipe));
    }

    handle->on<uvw::ExitEvent>([this](uvw::ExitEvent &ev, uvw::ProcessHandle &) {
      LOG_ERROR << "uv exited " << ev.status << " " << ev.signal;
      exited();
    });
    handle->on<uvw::ErrorEvent>([this](uvw::ErrorEvent &e, uvw::ProcessHandle &) {
      LOG_ERROR << "uv error " << e.what();
      exited();
    });

    _procManager.idle->once<uvw::IdleEvent>([this, handle, args, callback](uvw::IdleEvent &, uvw::IdleHandle &) {
      const char **args_char = new const char *[args.size() + 2];
      args_char[0] = _exec.c_str();
      args_char[args.size() + 1] = nullptr;
      for (size_t i = 0; i < args.size(); i++) {
        args_char[i + 1] = args.at(i).c_str();
      }
      handle->spawn(_exec.c_str(), (char **) args_char);
      if (_procManager.registerPid) {
        auto fn = *_procManager.registerPid;
        fn(*this);
      }
      _running = true;
      if (callback) {
          (*callback)(*this);
      }
    });
    return true;
  }

  bool Proc::isRunning() {
    return _running;
  }

  const string&Proc::getName() const {
    return _name;
  }

  int Proc::getPid() const {
    if (_handle) {
      return _handle->get()->pid();
    }
    return -1;
  }

  const std::vector<std::string> Proc::getArgs() const {
    return _args;
  }

}
