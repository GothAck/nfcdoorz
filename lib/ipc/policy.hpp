#include "client.hpp"

#pragma once

namespace nfcdoorz::ipc {

  struct IpcClientPolicy : IpcClient<ipc::policy::Server> {
    // inline auto getStatusCall() {
    //   return getCall<ipc::policy::StatusCallT, ipc::policy::StatusReplyT>();
    // }
    // inline auto getCreateAuthenticatorCall(std::string ttyName, std::string deviceID) {
    //   auto call = getCall<ipc::policy::CreateAuthenticatorCallT, ipc::policy::CreateAuthenticatorReplyT>();
    //   call.value->deviceID = deviceID;
    //   call.value->ttyName = ttyName;
    //   return call;
    // }
    // inline auto getDestroyAuthenticatorCall() {
    //   return getCall<ipc::policy::DestroyAuthenticatorCallT, ipc::policy::DestroyAuthenticatorReplyT>();
    // }
  };

}
