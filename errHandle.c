#include "errHandle.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void err_msg(const char *file, const char *function, int line, const char *type,
             const char *fmt, ...) {
  time_t timer;
  time(&timer);
  char *t = ctime(&timer);
  t[strlen(t) - 1] = '\0';
  fprintf(stderr, "%s ", t); /* 現在時刻を表示 */

  fprintf(stderr, "%d ", getpid()); /* プロセスIDを出力 */
  fprintf(stderr, "%s %s %d %s ", file, function, line, type);

  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap); /* fmtの書式に従って可変個数引数を順次呼び出す */
  va_end(ap);

  fprintf(stderr, " %s(%d)", strerror(errno),
          errno); /* エラーメッセージを出力 */
  fputc('\n', stderr);
}