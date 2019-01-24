#include <string>
#include <iostream>
#include <variant>

#include <catch.hpp>

#include "lib/nfc.hpp"

using namespace nfcdoorz;
using namespace std;

extern map<string, int> call_count;

TEST_CASE("Context calls nfc_init once and only once") {
  nfc::Context context;
  REQUIRE( context.init() );
  REQUIRE( call_count.count("nfc_init") == 1 );
  REQUIRE( call_count.at("nfc_init") == 1 );
  REQUIRE( context.init() );
  REQUIRE( call_count.at("nfc_init") == 1 );
}

TEST_CASE("Context calls nfc_exit upon destruction") {
  {
    nfc::Context context;
    REQUIRE( context.init() );
    SECTION("Calls nfc_exit upon destruction");
    REQUIRE( call_count.count("nfc_free") == 0 );
  }
  REQUIRE( call_count.count("nfc_exit") == 1 );
  REQUIRE( call_count.at("nfc_exit") == 1 );
}
