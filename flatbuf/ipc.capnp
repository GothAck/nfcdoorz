@0xf4f468382e151499;

interface Base {
  ping @0 () -> (ok: Bool);
}

struct Tag {
  uid @0 :Data;
  result @1 :Bool;
}

struct AuthData {
  aid @0 :List(UInt8);
  key @1 :List(UInt8);
}

interface Auth {
  struct State {
    union {
      none @0 :Void;
      tag @1 :Tag;
    }
  }
  struct Action {
    union {
      poll @0 :Void;
      authApp @1 :AuthData;
    }
  }
  getAction @0 (state: State) -> Action;
}

interface API {
  struct Status {
    struct Proc {
      pid @0 :UInt32;
      name @1 :Text;
      exec @2 :Text;
      args @3 :List(Text);
    }
    processes @0 :List(Proc);
  }
  getStatus @0 () -> Status;
}

interface Main {
  auth @0 () -> (interface: Auth);
  api @1 () -> (interface: API);
}

# enum Action {
#   authenticate_app @0;
#   get_uid @1;
#   read_std_data_file @2;
#   read_backup_data_file @3;
#   read_value_file @4;
#   read_cyclic_record_file @5;
#   read_linear_record_file @6;
#   read_std_data_file @7;
#   read_backup_data_file @8;
#   read_value_file @9;
#   read_cyclic_record_file @10;
#   read_linear_record_file @11;
#   commit_transaction @12;
#   close @99;
# }
#
# struct TagAction {
#   index @0 :UInt32;
#   action @1 :Action;
#   keyType @2 :string;
#   keyData @3 :string;
#   appID @4 :(:Uint8, )
# }
#
# interface Tag {
#   begin @0 () -> TagAction;
#   result @1 (action: TagAction, ) -> TagAction;
# }
# interface Authentication {
#   receivedAuth @0 ()
# }
