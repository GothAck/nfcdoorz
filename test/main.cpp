#include <string>
#include <iostream>
#include <filesystem>

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include "lib/config.hpp"
#include "test/stub/stub.h"

using namespace nfcdoorz;
using namespace std;

map<string, int> call_count;

void mock_was_called(const char *name, ...) {
  cout << "mock_was_called: " << name << endl;
  int count = 0;
  if (call_count.count(name))
    count = call_count[name];
  call_count[name] = count + 1;
}

struct ResetConfigPath : Catch::TestEventListenerBase {
  using TestEventListenerBase::TestEventListenerBase; // inherit constructor

  virtual void testCaseEnded(Catch::TestCaseStats const &testCaseStats) override {
    config::decodePath.clear();
    call_count.clear();
  }

  virtual void testCaseStarting(Catch::TestCaseInfo const &testCaseInfo) override {
    config::decodePath.clear();
    call_count.clear();
  }
};

CATCH_REGISTER_LISTENER( ResetConfigPath )

int main(int argc, char *argv[]) {
  auto prog = filesystem::path(argv[0]);
  filesystem::current_path(prog.parent_path().c_str());
  cout << prog.filename().c_str() << endl;

  return Catch::Session().run(argc, argv);
}
