#include <iostream>
#include <stdint.h>
#include <string.h> // memcpy
#include <stdlib.h> //realloc

extern "C" {
#include <openssl/aes.h>
#include <openssl/cmac.h>
#include <openssl/evp.h>
}
#include "keys.hpp"

KeyHolder::~KeyHolder() {
  if (held_key) free(held_key);
}

KeyHolder::operator MifareDESFireKey() {
  if (used) throw KeyError();
  used = true;
  return held_key;
}

// DESFireAESKey::DESFireAESKey(
//   std::array<uint8_t, 16> keydata,
//   uint8_t version
// ) : _version(version), key(nullptr) {
//   memcpy(_keydata, keydata.data(), 16);
// }
//
// DESFireAESKey::~DESFireAESKey() {
//   if (key) free(key);
// }
//
// DESFireAESKey::operator MifareDESFireKey() {
//   if (key) free(key);
//   key = mifare_desfire_aes_key_new_with_version(_keydata, _version);
//   return key;
// }
//
// uint8_t DESFireAESKey::operator[](int index) {
//   return _keydata[index];
// }
//
// bool DESFireAESKey::diversify(std::array<uint8_t, AID_SIZE> aid, char *uid, uint8_t salt[SALT_SIZE]) {
//   uint8_t aid_data[3] = {0, 0, 0};
//   memcpy(aid_data, aid.data(), aid.size());
//   uint8_t *src = new uint8_t[16];
//   memcpy(src, _keydata, 16);
//   std::cout << "srckey: ";
//   for (uint8_t i = 0; i < 16; i++) std::cout << std::hex << (int) src[i] << ":";
//   std::cout << std::endl;
//
//   uint8_t *dest = new uint8_t[16];
//   memset(dest, 0, 16);
//   std::cout << "destkey: ";
//   for (uint8_t i = 0; i < 16; i++) std::cout << std::hex << (int) dest[i] << ":";
//   std::cout << std::endl;
//
//   if (diversify_key(src, aid_data, uid, dest, salt) < 0) {
//     delete src;
//     delete dest;
//     return false;
//   }
//
//   std::cout << "destkey: ";
//   for (uint8_t i = 0; i < 16; i++) std::cout << std::hex << (int) dest[i] << ":";
//   std::cout << std::endl;
//
//   memcpy(_keydata, dest, 16);
//   std::cout << "keydata: ";
//   for (uint8_t i = 0; i < 16; i++) std::cout << std::hex << (int) _keydata[i] << ":";
//   std::cout << std::endl;
//   if (key) {
//     free(key);
//     key = nullptr;
//   }
//
//   return true;
// }

int diversify_key(uint8_t base_key[KEY_SIZE], uint8_t aid[AID_SIZE], char *uid, uint8_t *new_key, uint8_t salt[SALT_SIZE]) {
  CMAC_CTX *ctx;
  int ret;

  ctx = CMAC_CTX_new();
  ret = CMAC_Init(ctx, base_key, KEY_SIZE, EVP_aes_128_cbc(), NULL);
  // TODO: Check the meaning of the return values
  if (ret != 1) {
    return -1;
  }
  size_t uid_len = strlen(uid);
  if (uid_len % 2) {
    CMAC_CTX_free(ctx);
    return -1;
  }
  std::cout << "Uid len: " << std::dec << uid_len << std::endl;
  uint8_t *uid_hex = new uint8_t[uid_len];

  for (uint8_t i = 0; i < uid_len; i += 2) {
    uid_hex[i / 2] = (char2int(uid[i]) << 4) | char2int(uid[i+1]);
  }

  uint8_t *data = new uint8_t[1 + uid_len + AID_SIZE + SALT_SIZE];
  memset(data, 0, 1 + uid_len + AID_SIZE + SALT_SIZE);
  data[0] = 0x01;
  memcpy(data + 1, uid_hex, 4);
  delete uid_hex;
  uid_hex = nullptr;
  memcpy(data + 1 + uid_len, aid, AID_SIZE);
  memcpy(data + 1 + uid_len + AID_SIZE, salt, SALT_SIZE);

  ret = CMAC_Update(ctx, data, 1 + uid_len + AID_SIZE + SALT_SIZE);
  delete data;
  data = nullptr;
  if (ret != 1) {
    CMAC_CTX_free(ctx);
    return -2;
  }

  size_t cmac_len;
  ret = CMAC_Final(ctx, new_key, &cmac_len);
  if (ret != 1) {
    CMAC_CTX_free(ctx);
    return -3;
  }

  CMAC_CTX_free(ctx);
  return 0;
}

uint8_t char2int(char input) {
  if(input >= '0' && input <= '9')
    return input - '0';
  if(input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if(input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  return 0;
}
