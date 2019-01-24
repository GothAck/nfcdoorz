#define _GNU_SOURCE
#include <dlfcn.h>

#include <nfc/nfc.h>

#include "stub.h"
#include "nfc.h"

void nfc_close(nfc_device *pnd) {
  mock_was_called(__FUNCTION__);
}

void nfc_exit(nfc_context *context) {
  mock_was_called(__FUNCTION__);
}

void nfc_free(void *p) {
  mock_was_called(__FUNCTION__);
}

void nfc_init(nfc_context **context) {
  mock_was_called(__FUNCTION__);
  *((uint8_t **)context) = 1;
}

size_t nfc_list_devices(nfc_context *context, nfc_connstring connstrings[], size_t connstrings_len) {
  mock_was_called(__FUNCTION__);
  return 0;
}

nfc_device *nfc_open(nfc_context *context, const nfc_connstring connstring) {
  mock_was_called(__FUNCTION__);
  return NULL;
}

int str_nfc_target(char **buf, const nfc_target *pnt, _Bool verbose) {
  mock_was_called(__FUNCTION__);
  return 0;
}
