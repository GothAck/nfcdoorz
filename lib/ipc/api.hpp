#include "client.hpp"

#pragma once

namespace nfcdoorz::ipc {

  struct IpcClientAPI : IpcClient<ipc::api::Server> {
    inline auto getConfigCall() {
      return getCall<common::ConfigCallT, common::ConfigReplyT>();
    }
    inline auto getStatusCall() {
      return getCall<ipc::api::StatusCallT, ipc::api::StatusReplyT>();
    }
    inline auto getCreateAuthenticatorCall(std::string ttyName, std::string deviceID) {
      auto call = getCall<ipc::api::CreateAuthenticatorCallT, ipc::api::CreateAuthenticatorReplyT>();
      call.value->deviceID = deviceID;
      call.value->ttyName = ttyName;
      return call;
    }
    inline auto getDestroyAuthenticatorCall() {
      return getCall<ipc::api::DestroyAuthenticatorCallT, ipc::api::DestroyAuthenticatorReplyT>();
    }
  };

}
