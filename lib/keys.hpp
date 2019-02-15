#include <array>

#include <freefare.h>

#pragma once

#define KEY_SIZE 16
#define AID_SIZE 3
#define SALT_SIZE 16

class KeyError {};

class KeyHolder {
public:
  KeyHolder(MifareDESFireKey key) : held_key(key), used(false) {
  }
  ~KeyHolder();
  operator MifareDESFireKey();
private:
  MifareDESFireKey held_key;
  bool used;
};

// class DESFireKey {
// protected:
// DESFireKey() {};
// };

// class DESFireAESKey : public DESFireKey {
// uint8_t _keydata[16];
// uint8_t _version;
// MifareDESFireKey key;
// public:
//
// DESFireAESKey(std::array<uint8_t, 16> keydata, uint8_t version = 0x0);
// ~DESFireAESKey();
//
// operator MifareDESFireKey();
// uint8_t operator[](int);
//
// bool diversify(std::array<uint8_t, AID_SIZE>, char *uid, uint8_t salt[SALT_SIZE]);
//
// };

int diversify_key(uint8_t base_key[KEY_SIZE], uint8_t aid[AID_SIZE], char *uid, uint8_t *new_key, uint8_t salt[SALT_SIZE]);

uint8_t char2int(char input);
