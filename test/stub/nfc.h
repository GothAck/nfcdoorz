void nfc_close(nfc_device *pnd);
void nfc_exit(nfc_context *context) __attribute__((nonnull (1)));
void nfc_init(nfc_context **context) __attribute__((nonnull (1)));
size_t nfc_list_devices(nfc_context *context, nfc_connstring connstrings[], size_t connstrings_len) __attribute__((nonnull (1)));
nfc_device *nfc_open(nfc_context *context, const nfc_connstring connstring) __attribute__((nonnull (1)));
