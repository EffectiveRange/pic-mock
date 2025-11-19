#ifndef MRHAT_DEBUG_H_INCLUDED
#define MRHAT_DEBUG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MRHAT_FW_DEBUG_LOG_ENABLED)

#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static inline void log_debug(const char *const msg, ...) {
  va_list args;
  va_start(args, msg);

  long ms;  // Milliseconds
  time_t s; // Seconds
  struct timespec spec;

  clock_gettime(CLOCK_REALTIME, &spec);

  s = spec.tv_sec;
  ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
  if (ms > 999) {
    s++;
    ms = 0;
  }
  printf("%" PRIdMAX ".%03ld:", (intmax_t)s, ms);
  vprintf(msg, args);
  printf("\n");
  va_end(args);
}
#else

static inline void log_debug(const char *const msg, ...) {
  // No-op
}

#endif

#ifdef __cplusplus
}
#endif

#endif // MRHAT_DEBUG_H_INCLUDED