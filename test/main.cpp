#define _GNU_SOURCE
#include <dlfcn.h>

#include <mutex>
#include <string>
#include <iostream>
#include <filesystem>

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include "main.hpp"
#include "lib/config.hpp"
#include "test/stub/stub.h"

using namespace nfcdoorz;
using namespace std;

map<string, int> call_count;
map<string, vector<vector<void *>>> calls;

#define STR2(s) #s
#define STR(s) STR2(s)
#define __CAT(A, B) A ## B
#define CAT(A, B) __CAT(A, B)

#define STUB_FUNC_DYNAMIC(NAME, RETURN_T, CALL_ARGS, ...) \
  typedef decltype(NAME)* CAT(NAME, _t); \
  RETURN_T NAME (__VA_ARGS__) { return ((CAT(NAME, _t))dlsym(RTLD_NEXT, STR(NAME))) CALL_ARGS; }

STUB_FUNC_DYNAMIC(
  mock_return,
  void,
  (name, retval),
  const char *name, void *retval)

STUB_FUNC_DYNAMIC(
  mock_clear_return,
  void,
  (name),
  const char *name)

vector<vector<void *>> &get_or_add_calls(string &key) {
  if (!calls.count(key))
    calls[key] = vector<vector<void *>>();
  return calls[key];
}

mutex mock_was_called_mutex;
string key;

void mock_was_called(const char *name, size_t num_args, ...) {
  mock_was_called_mutex.lock();
  key.clear();
  key.append(name);
  INFO("Mock called " << key);
  int count = 0;
  vector<vector<void *>> &fn_calls = get_or_add_calls(key);

  if (call_count.count(key))
    count = call_count[key];

  va_list ap;
  va_start(ap, num_args);
  vector<void *> args;
  while (num_args--) {
    args.push_back(va_arg(ap, void *));
  }
  va_end(ap);
  fn_calls.push_back(args);

  call_count[key] = count + 1;
  mock_was_called_mutex.unlock();
}

struct ResetConfigPath : Catch::TestEventListenerBase {
  using TestEventListenerBase::TestEventListenerBase; // inherit constructor

  virtual void testCaseEnded(Catch::TestCaseStats const &testCaseStats) override {
    config::decodePath.clear();
    call_count.clear();
    mock_clear_return(NULL);
  }

  virtual void testCaseStarting(Catch::TestCaseInfo const &testCaseInfo) override {
    config::decodePath.clear();
    call_count.clear();
    mock_clear_return(NULL);
  }
};

CATCH_REGISTER_LISTENER( ResetConfigPath )

int main(int argc, char *argv[]) {
  auto prog = filesystem::path(argv[0]);
  filesystem::current_path(prog.parent_path().c_str());
  cout << prog.filename().c_str() << endl;

  return Catch::Session().run(argc, argv);
}
