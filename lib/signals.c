#include <signal.h>

#include "lib/signals.h"

uint8_t sig_interrupted = 0;

void sig_handler(int signo __attribute__((unused))) {
  sig_interrupted = 1;
}

void setup_signals() {
  struct sigaction action;
  action.sa_handler = sig_handler;
  action.sa_flags = 0;
  sigemptyset(&action.sa_mask);
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTERM, &action, NULL);
}
