#include <string>
#include <iostream>
#include <variant>
#include <vector>

#include <catch.hpp>

#include "lib/nfc.hpp"
#include "test/stub/stub.h"
#include "test/main.hpp"

using namespace nfcdoorz;
using namespace std;

void *gen_mock_tags() {
  void **mock_tags = (void **)malloc(sizeof(void *) * 2);
  mock_tags[0] = malloc(1024);
  mock_tags[1] = nullptr;
  return mock_tags;
}

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

TEST_CASE("Context::getDevices() calls nfc_list_devices") {
  nfc::Context context;
  context.init();

  vector<nfc::Device> devices = context.getDevices();

  REQUIRE( call_count.count("nfc_list_devices") == 1 );
  REQUIRE( call_count.at("nfc_list_devices") == 1 );
  REQUIRE( devices.size() == 2 );
}

TEST_CASE("Context::getDeviceMatching(\"123\") returns empty optional") {
  nfc::Context context;
  context.init();

  optional<nfc::Device> device = context.getDeviceMatching("123");

  REQUIRE( call_count.count("nfc_list_devices") == 1 );
  REQUIRE( call_count.at("nfc_list_devices") == 1 );
  REQUIRE( !device );
}

TEST_CASE("Context::getDeviceMatching(\"9999\") returns device") {
  nfc::Context context;
  context.init();

  optional<nfc::Device> device = context.getDeviceMatching("9999");

  REQUIRE( call_count.count("nfc_list_devices") == 1 );
  REQUIRE( call_count.at("nfc_list_devices") == 1 );
  REQUIRE( !!device );
}

TEST_CASE("Device::open() calls api") {
  nfc::Context context;
  context.init();
  optional<nfc::Device> device = context.getDeviceMatching("9999");
  REQUIRE( !!device );
  REQUIRE( call_count.count("nfc_open") == 0 );

  device->open();

  REQUIRE( call_count.count("nfc_open") == 1 );
  REQUIRE( call_count.at("nfc_open") == 1 );
}

TEST_CASE("Device::getTags() calls api") {
  nfc::Context context;
  context.init();
  optional<nfc::Device> device = context.getDeviceMatching("9999");
  REQUIRE( !!device );
  REQUIRE( call_count.count("freefare_get_tags") == 0 );

  device->open();
  vector<nfc::Tag> tags = device->getTags();
  REQUIRE( call_count.count("freefare_get_tags") == 1 );
  REQUIRE( call_count.at("freefare_get_tags") == 1 );
}

TEST_CASE("Tag returns type from api call") {
  enum freefare_tag_type tag_type = MIFARE_DESFIRE;
  void *mock_tags = gen_mock_tags();
  mock_return("freefare_get_tag_type", &tag_type);
  mock_return("freefare_get_tags", &mock_tags);


  nfc::Context context;
  context.init();
  optional<nfc::Device> device = context.getDeviceMatching("9999");
  REQUIRE( !!device );
  REQUIRE( call_count.count("freefare_get_tags") == 0 );

  device->open();
  vector<nfc::Tag> tags = device->getTags();
  REQUIRE( tags.size() > 0 );
  nfc::Tag tag = tags[0];
  REQUIRE( tag.getTagType() == MIFARE_DESFIRE );
}

TEST_CASE("Tag returns DESFireTagInterface") {
  enum freefare_tag_type mock_tag_type = MIFARE_DESFIRE;
  void *mock_tags = gen_mock_tags();
  mock_return("freefare_get_tag_type", &mock_tag_type);
  mock_return("freefare_get_tags", &mock_tags);

  nfc::Context context;
  context.init();
  optional<nfc::Device> device = context.getDeviceMatching("9999");
  device->open();
  vector<nfc::Tag> tags = device->getTags();

  nfc::Tag &tag = tags[0];
  REQUIRE( tag.getTagType() == MIFARE_DESFIRE );

  nfc::TagInterfaceVariant_t interface = tag.getTagInterfaceByType();

  nfc::DESFireTagInterface t = get<nfc::DESFireTagInterface>(interface);
}

TEST_CASE("Tag cannot return other tag interfaces, yet...") {
  void *mock_tags = gen_mock_tags();
  mock_return("freefare_get_tags", &mock_tags);

  nfc::Context context;
  context.init();
  optional<nfc::Device> device = context.getDeviceMatching("9999");
  device->open();
  vector<nfc::Tag> tags = device->getTags();
  nfc::Tag tag = tags[0];
  REQUIRE( tag.getTagType() != MIFARE_DESFIRE );

  REQUIRE_THROWS( tag.getTagInterfaceByType() );
}

TEST_CASE("Tag is castable to FreefareTag") {
  void *mock_tags = gen_mock_tags();
  mock_return("freefare_get_tags", &mock_tags);

  nfc::Context context;
  context.init();
  optional<nfc::Device> device = context.getDeviceMatching("9999");
  device->open();
  vector<nfc::Tag> tags = device->getTags();
  nfc::Tag tag = tags[0];
  FreefareTag fft = tag;
}

TEST_CASE("KeyDES constuctable") {
  nfc::KeyDES key1;

  REQUIRE( key1.size == 8 );
  REQUIRE( key1.data.size() == 8 );

  REQUIRE( all_of(key1.data.begin(), key1.data.end(), [](uint8_t c){ return c == 0; }) );

  nfc::KeyDES key2({0, 1, 2, 3, 4, 5, 6, 7});

  for (uint8_t i = 0; i < 8; i++)
    REQUIRE( key2.data[i] == i );

  REQUIRE_FALSE( all_of(key2.data.begin(), key2.data.end(), [](uint8_t c){ return c == 0; }) );
}

TEST_CASE("KeyDES castable") {
  void *mock_key = malloc(128);
  mock_return("mifare_desfire_des_key_new", &mock_key);

  nfc::KeyDES key2({0, 1, 2, 3, 4, 5, 6, 7});

  MifareDESFireKey key = key2;

  REQUIRE (key2.data.data());

  REQUIRE( calls.count("mifare_desfire_des_key_new") );
  REQUIRE( calls["mifare_desfire_des_key_new"].size() == 1 );
  REQUIRE( calls["mifare_desfire_des_key_new"][0][0] == key2.data.data() );

  REQUIRE( mock_key == (void *)key );
}

TEST_CASE("KeyAES key deriver not called when key.diversify == false") {
  enum freefare_tag_type tag_type = MIFARE_DESFIRE;
  void *mock_tags = gen_mock_tags();
  void *mock_key = malloc(128);
  mock_return("freefare_get_tag_type", &tag_type);
  mock_return("freefare_get_tags", &mock_tags);
  mock_return("mifare_desfire_aes_key_new", &mock_key);

  void *mock_deriver = malloc(128);
  mock_return("mifare_key_deriver_new_an10922", &mock_deriver);
  int success = 0;
  mock_return("mifare_key_deriver_begin", &success);
  mock_return("mifare_key_deriver_update_uid", &success);
  mock_return("mifare_key_deriver_end", &mock_key);

  nfc::Context context;
  context.init();
  optional<nfc::Device> device = context.getDeviceMatching("9999");
  device->open();
  vector<nfc::Tag> tags = device->getTags();
  REQUIRE( call_count.count("freefare_get_tags") );
  REQUIRE( call_count.at("freefare_get_tags") == 1 );
  REQUIRE ( tags.size() );
  nfc::Tag &tag = tags[0];

  nfc::KeyAES key;

  key.diversify = false;

  REQUIRE( all_of(key.data.begin(), key.data.end(), [](uint8_t c){ return c == 0; }) );

  MifareDESFireKey retkey = key.deriveKey(tag, nullptr);
  REQUIRE_FALSE( call_count.count("mifare_key_deriver_begin") );
  REQUIRE_FALSE( call_count.count("mifare_key_deriver_update_uid") );
  REQUIRE_FALSE( call_count.count("mifare_key_deriver_update_aid") );
  REQUIRE_FALSE( call_count.count("mifare_key_deriver_end") );
  REQUIRE_FALSE( call_count.count("mifare_key_deriver_free") );

  REQUIRE_FALSE( retkey );
}

TEST_CASE("KeyAES key deriver called when diversify == true") {
  enum freefare_tag_type tag_type = MIFARE_DESFIRE;
  void *mock_tags = gen_mock_tags();
  void *mock_key = malloc(128);
  mock_return("freefare_get_tag_type", &tag_type);
  mock_return("freefare_get_tags", &mock_tags);
  mock_return("mifare_desfire_aes_key_new", &mock_key);

  void *mock_deriver = malloc(128);
  mock_return("mifare_key_deriver_new_an10922", &mock_deriver);
  int success = 0;
  mock_return("mifare_key_deriver_begin", &success);
  mock_return("mifare_key_deriver_update_uid", &success);
  mock_return("mifare_key_deriver_end", &mock_key);

  nfc::Context context;
  context.init();
  optional<nfc::Device> device = context.getDeviceMatching("9999");
  device->open();
  vector<nfc::Tag> tags = device->getTags();
  REQUIRE( call_count.count("freefare_get_tags") );
  REQUIRE( call_count.at("freefare_get_tags") == 1 );
  REQUIRE ( tags.size() );
  nfc::Tag &tag = tags[0];

  nfc::KeyAES key;

  key.diversify = true;

  REQUIRE( all_of(key.data.begin(), key.data.end(), [](uint8_t c){ return c == 0; }) );

  MifareDESFireKey retkey = key.deriveKey(tag, nullptr);
  REQUIRE( call_count.count("mifare_key_deriver_begin") );
  REQUIRE( call_count.at("mifare_key_deriver_begin") == 1 );
  REQUIRE( call_count.count("mifare_key_deriver_update_uid") );
  REQUIRE( call_count.at("mifare_key_deriver_update_uid") == 1 );
  REQUIRE_FALSE( call_count.count("mifare_key_deriver_update_aid") );
  REQUIRE( call_count.count("mifare_key_deriver_end") );
  REQUIRE( call_count.at("mifare_key_deriver_end") == 1 );
  REQUIRE( call_count.count("mifare_key_deriver_free") );
  REQUIRE( call_count.at("mifare_key_deriver_free") == 1 );

  REQUIRE( retkey == mock_key );
}
