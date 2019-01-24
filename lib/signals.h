#include <stdint.h>
#include <stddef.h>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t sig_interrupted;

void setup_signals();

#ifdef __cplusplus
}
#endif
