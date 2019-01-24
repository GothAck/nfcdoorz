#include <string>
#include <iostream>
#include <variant>

#include <catch.hpp>

#include "lib/config.hpp"

using namespace nfcdoorz;
using namespace std;

TEST_CASE("Config load from valid file") {
  config::Config config;

  REQUIRE_NOTHROW(
    config = config::Config::load(
      "./config-good.yaml"
    )
  );
}

TEST_CASE("Stringify config") {
  config::Config config = config::Config::load(
    "./config-good.yaml"
  );

  stringstream sb("");
  sb << config << endl;

  REQUIRE(sb.str().length() > 2);
}

TEST_CASE("Parse basic config") {
  config::Config config;
  REQUIRE_NOTHROW(
    config = config::Config::parse(
      "picc:\n"
      "  key:\n"
      "    data: [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]\n"
      "    type: aes\n"
      "    diversify: True\n"
      "apps: []\n"
    )
  );

  REQUIRE(holds_alternative<config::Key::KeyAES>(config.picc.key.key));
}

TEST_CASE("Key is iterable") {
  config::Config config = config::Config::parse(
    "picc:\n"
    "  key:\n"
    "    data: [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]\n"
    "    type: aes\n"
    "    diversify: True\n"
    "apps: []\n"
  );

  stringstream sb("");
  visit([&sb](auto &key){
    for (auto &c: key.data) {
      sb << hex << c << endl;
    }
  }, config.picc.key.key);

  REQUIRE(sb.str().length() > 0);

  REQUIRE(holds_alternative<config::Key::KeyAES>(config.picc.key.key));
}

TEST_CASE("Config bad picc key") {
  config::Config config;
  bool thrown = false;

  try {
    config = config::Config::load(
      "./config-bad-picc-key.yaml"
    );
  } catch (config::ValidationException e) {
    thrown = true;
  }

  REQUIRE( thrown );

  REQUIRE(
    config::decodePath ==
    vector<string> {
      "config", "picc", "key", "(aes)", "data"
    }
  );
}

TEST_CASE("Config bad app - disparate app key types") {
  config::Config config;
  bool thrown = false;

  try {
    config = config::Config::load(
      "./config-bad-disparate-app-keys.yaml"
    );
  } catch (config::ValidationException e) {
    thrown = true;
  }

  REQUIRE( thrown );

  REQUIRE(
    config::decodePath ==
    vector<string> {
      "config", "apps", "app"
    }
  );
}
