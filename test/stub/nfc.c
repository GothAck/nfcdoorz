#define _GNU_SOURCE
#include <dlfcn.h>

#include <nfc/nfc.h>

#include "stub.h"
#include "nfc.h"

void nfc_close(nfc_device *pnd) {
  mock_was_called(__FUNCTION__, 0);
}

void nfc_exit(nfc_context *context) {
  mock_was_called(__FUNCTION__, 0);
}

void nfc_free(void *p) {
  mock_was_called(__FUNCTION__, 0);
}

void nfc_init(nfc_context **context) {
  mock_was_called(__FUNCTION__, 0);
  *((uint8_t **)context) = 1;
}

size_t nfc_list_devices(nfc_context *context, nfc_connstring connstrings[], size_t connstrings_len) {
  mock_was_called(__FUNCTION__, 0);
  size_t count = 0;
  if (connstrings_len > 0)
    strcpy((char *)(connstrings + (count++)), "test1234:9999");
  if (connstrings_len > 1)
    strcpy((char *)(connstrings + (count++)), "test1234:9988");
  return count;
}

nfc_device *nfc_open(nfc_context *context, const nfc_connstring connstring) {
  mock_was_called(__FUNCTION__, 0);
  return 1;
}

int str_nfc_target(char **buf, const nfc_target *pnt, _Bool verbose) {
  mock_was_called(__FUNCTION__, 0);
  return 0;
}
