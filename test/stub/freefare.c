#define _GNU_SOURCE
#include <stdlib.h>
#include <freefare.h>

#include "stub.h"
#include "stub_internal.h"
#include "freefare.h"

#define RETURN_MOCKED(TYPE) { \
  void *value = get_mock_return_value(__FUNCTION__); \
  if (value) return *((TYPE *)value); }

FreefareTag *freefare_get_tags(nfc_device *pnd) {
  mock_was_called(__FUNCTION__, 0);
  RETURN_MOCKED(FreefareTag *);
  return NULL;
}

void freefare_free_tags(FreefareTag *tags) {
  mock_was_called(__FUNCTION__, 0);
  free(tags);
}

enum freefare_tag_type freefare_get_tag_type(FreefareTag tag) {
  mock_was_called(__FUNCTION__, 0);
  void * value = get_mock_return_value(__FUNCTION__);
  RETURN_MOCKED(enum freefare_tag_type);
  return FELICA;
}

MifareDESFireKey mifare_desfire_des_key_new(const uint8_t value[8]) {
  mock_was_called(__FUNCTION__, 1, value);
  RETURN_MOCKED(MifareDESFireKey);
  return NULL;
}

MifareKeyDeriver mifare_key_deriver_new_an10922(MifareDESFireKey master_key, MifareKeyType output_key_type) {
  mock_was_called(__FUNCTION__, 2, master_key, output_key_type);
  RETURN_MOCKED(MifareKeyDeriver);
  return NULL;
}
int mifare_key_deriver_begin(MifareKeyDeriver deriver) {
  mock_was_called(__FUNCTION__, 1, deriver);
  RETURN_MOCKED(int);
  return -1;
}
int mifare_key_deriver_update_uid(MifareKeyDeriver deriver, FreefareTag tag) {
  mock_was_called(__FUNCTION__, 2, deriver, tag);
  RETURN_MOCKED(int);
  return -1;
}
int mifare_key_deriver_update_aid(MifareKeyDeriver deriver, MifareDESFireAID aid) {
  mock_was_called(__FUNCTION__, 2, deriver, aid);
  RETURN_MOCKED(int);
  return -1;
}
MifareDESFireKey mifare_key_deriver_end(MifareKeyDeriver deriver) {
  mock_was_called(__FUNCTION__, 1, deriver);
  RETURN_MOCKED(MifareDESFireKey);
  return NULL;
}
void mifare_key_deriver_free(MifareKeyDeriver state) {
  mock_was_called(__FUNCTION__, 1, state);
}
