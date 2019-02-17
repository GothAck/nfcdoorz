#include "client.hpp"

#pragma once

namespace nfcdoorz::ipc {

  struct IpcClientAuth : IpcClient<ipc::auth::Server> {
    inline auto getConfigCall() {
      return getCall<common::ConfigCallT, common::ConfigReplyT>();
    }
  };

}
