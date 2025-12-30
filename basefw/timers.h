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
#include <xc.h>

#define SYS_CLOCK_FREQ_KHZ 64000
#define TMR_PRESCALE 32768

#define TIMER_MAX_MS ((UINT16_MAX * TMR_PRESCALE / SYS_CLOCK_FREQ_KHZ) + 1)

#ifdef __cplusplus
extern "C" {
#endif

struct tw_timer_t;

typedef struct tw_timer_t __far *tw_timer_ptr_t;

typedef tw_timer_ptr_t (*__far timer_callback_t)(tw_timer_ptr_t);
struct tw_timer_t {
  // client must set these
  uint16_t time_ms;
  timer_callback_t callback;
  void *user_data; // user data is optional
  // used by the framework, must not be touched from client
  volatile bool expired;
  tw_timer_ptr_t next;
  tw_timer_ptr_t prev;
  uint16_t time_ms_orig;
};

void add_timer(tw_timer_ptr_t timer);
void remove_timer(tw_timer_ptr_t timer);

void timers_initialize(void);
void timers_deinitialize(void);

void register_timer_module(void);

void timers_reset(void);

tw_timer_ptr_t head_timer(void);
tw_timer_ptr_t sentinel_timer(void);

#ifdef __cplusplus
}
#endif

#endif /* TIMERS_H */
