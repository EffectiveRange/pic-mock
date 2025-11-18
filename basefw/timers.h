/*
 * File:   timers.h
 * Author: emtszeb
 *
 * Created on August 2, 2024, 5:41 PM
 */
#ifndef TIMERS_H
#define TIMERS_H

#include <stdbool.h>
#include <stdint.h>

#define SYS_CLOCK_FREQ_KHZ 64000
#define TMR_PRESCALE 32768

#define TIMER_MAX_MS ((UINT16_MAX * TMR_PRESCALE / SYS_CLOCK_FREQ_KHZ) + 1)

#ifdef __cplusplus
extern "C" {
#endif
struct tw_timer_t {
  // client must set these
  uint16_t time_ms;
  struct tw_timer_t *(*callback)(struct tw_timer_t *);
  void *user_data; // user data is optional
  // used by the framework, must not be touched from client
  volatile bool expired;
  struct tw_timer_t *next;
  struct tw_timer_t *prev;
  uint16_t time_ms_orig;
};

void add_timer(struct tw_timer_t *timer);

void remove_timer(struct tw_timer_t *timer);

void timers_initialize(void);
void timers_deinitialize(void);

void register_timer_module(void);

void timers_reset(void);

struct tw_timer_t *head_timer(void);
struct tw_timer_t *sentinel_timer(void);

#ifdef __cplusplus
}
#endif

#endif /* TIMERS_H */
