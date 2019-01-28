#include "types.hpp"

void freekey(MifareDESFireKey *key) {
  mifare_desfire_key_free(*key);
}

void freederiver(MifareKeyDeriver *deriver) {
  mifare_key_deriver_free(*deriver);
}

void freeaid(MifareDESFireAID *aid) {
  free(*aid);
}
