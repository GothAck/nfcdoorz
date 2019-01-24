#include <err.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <nfc/nfc.h>
#include <nfc/nfc-types.h>
#include <freefare.h>
#include <yaml-cpp/yaml.h>

#include "nfc-utils.h"

#define MAX_DEVICE_COUNT 16

static nfc_device *pnd = NULL;
static nfc_context *context;

static void stop_polling(int sig) {
  (void) sig;
  if (pnd != NULL)
    nfc_abort_command(pnd);
  else {
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
}

static void print_usage(const char *progname) {
  printf("usage: %s [-v]\n", progname);
  printf("  -v\t verbose display\n");
}

void desfire(FreefareTag &tag);

int main(int argc, const char *argv[]) {
  bool verbose = false;

  signal(SIGINT, stop_polling);

  // Display libnfc version
  const char *acLibnfcVersion = nfc_version();

  printf("%s uses libnfc %s\n", argv[0], acLibnfcVersion);
  if (argc != 1) {
    if ((argc == 2) && (0 == strcmp("-v", argv[1]))) {
      verbose = true;
    } else {
      print_usage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  YAML::Node config = YAML::LoadFile("config.yaml");

  const uint8_t uiPollNr = 255;
  const uint8_t uiPeriod = 1;
  const nfc_modulation nmModulations[] = {
    { .nmt = NMT_ISO14443A, .nbr = NBR_106 },
    { .nmt = NMT_ISO14443B, .nbr = NBR_106 },
  };
  const size_t szModulations = sizeof(nmModulations) / sizeof(nfc_modulation);

  nfc_target nt;
  int res = 0;

  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }

  pnd = nfc_open(context, NULL);

  if (pnd == NULL) {
    ERR("%s", "Unable to open NFC device.");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));

  while(1) {

    printf("NFC device will poll during %ld ms (%u pollings of %lu ms for %" PRIdPTR " modulations)\n", (unsigned long) uiPollNr * szModulations * uiPeriod * 150, uiPollNr, (unsigned long) uiPeriod * 150, szModulations);
    if ((res = nfc_initiator_poll_target(pnd, nmModulations, szModulations, uiPollNr, uiPeriod, &nt))  < 0) {
      nfc_perror(pnd, "nfc_initiator_poll_target");
      nfc_close(pnd);
      nfc_exit(context);
      exit(EXIT_FAILURE);
    }

    if (res > 0) {
      printf("Tag found\n");
      print_nfc_target(&nt, verbose);
      // FreefareTag t = freefare_tag_new(pnd, nt);
      // printf("Freefare tag %s\n", freefare_get_tag_friendly_name(t));
      // if (freefare_get_tag_type(t) == MIFARE_DESFIRE) {
      //   desfire(t);
      // }
      printf("Waiting for card removing...");
      fflush(stdout);
      while (0 == nfc_initiator_target_is_present(pnd, NULL)) {}
      // freefare_free_tag(t);
      nfc_perror(pnd, "nfc_initiator_target_is_present");
      printf("done.\n");
    } else {
      printf("No target found.\n");
    }

  }

  nfc_close(pnd);
  nfc_exit(context);
  exit(EXIT_SUCCESS);
}

void desfire(FreefareTag &t) {
  if(mifare_desfire_connect(t) < 0) {
    return;
  }
  struct mifare_desfire_version_info info;
  if(mifare_desfire_get_version(t, &info) < 0) {
    return;
  }
  if (info.software.version_major < 1) {
		warnx("Found old DESFire, skipping");
		return;
  }

}
