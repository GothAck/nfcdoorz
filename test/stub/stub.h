#ifdef __cplusplus
extern "C" {
#endif

void mock_return(const char *name, void *retval);

void mock_clear_return(const char *name);

void mock_was_called(const char *name, size_t num_args, ...);

#ifdef __cplusplus
}
#endif
