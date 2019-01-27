#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "stub.h"

#define MAX_RETURN_VALUES 128

size_t return_values_count = 0;
const char *return_values_names[MAX_RETURN_VALUES];
void *return_values[MAX_RETURN_VALUES];

void *get_mock_return_value(const char *name) {
  for (size_t i = 0; i < return_values_count; i++) {
    if (!strcmp(name, return_values_names[i])) {
      return return_values[i];
    }
  }
  return NULL;
}

void mock_return(const char *name, void *retval) {
  void **ptr = NULL;
  for (size_t i = 0; i < return_values_count; i++) {
    if (!strcmp(name, return_values_names[i])) {
      ptr = &return_values[i];
    }
  }
  if (!ptr) {
    if (return_values_count == MAX_RETURN_VALUES) return;
    ptr = &return_values[return_values_count];
    return_values_names[return_values_count++] = name;
  }
  *ptr = retval;
}

void mock_clear_return(const char *name) {
  if (!name) {
    return_values_count = 0;
    return;
  }
  uint8_t deleting = 0;
  for (size_t i = 0; i < return_values_count; i++) {
    if (deleting) {
      if (i > 0) {
        return_values_names[i - 1] = return_values_names[i];
        return_values[i - 1] = return_values[i];
      }
    } else if (!strcmp(name, return_values_names[i])) {
      deleting = 1;
    }
  }
  if (deleting) return_values_count--;
}
