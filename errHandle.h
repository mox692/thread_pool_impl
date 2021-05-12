#ifndef ERROR
#define ERROR(fmt, ...)                                                        \
  err_msg(__FILE__, __FUNCTION__, __LINE__, "error", fmt, ##__VA_ARGS__)
#endif
#ifndef WARNNING
#define WARNNING(fmt, ...)                                                     \
  err_msg(__FILE__, __FUNCTION__, __LINE__, "warnning", fmt, ##__VA_ARGS__)
#endif

#ifndef err_msg
void err_msg(const char *file, const char *function, int line, const char *type,
             const char *fmt, ...);
#endif