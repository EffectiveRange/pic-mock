#include <string.h>
#include <xc.h>

#include "application.h"
#include "main.h"
#include "tasks.h"
#include "timers.h"

#include "mcc_generated_files/system/pins.h"
#include "mcc_generated_files/timer/tmr0.h"

struct timer_state_t {
  volatile timer_cb cb;
  volatile uint16_t time_units;
  // latched in from time_units when client arms timer
  volatile uint16_t time_units_left;
  // signals if timer is armed
  volatile uint8_t armed;
  // signals if timer is expired and pending callback delivery
  volatile uint8_t expired;
  // protect agains multibyte variable modification
  volatile uint8_t seq_lock;
  // generation id for synchronous cancellation
  // ensuring the callback is not delivered if the
  // client cancels or rearms timer when there is a pending
  // delivery
  volatile uint16_t gen;
};

struct timer_state_t timers[TIMERS_COUNT] = {};

struct pending_expiry {
  timer_cb cb;
  uint16_t gen;
};

typedef struct pending_expiry pending_expiry_t;

pending_expiry_t expired_cbs[TIMERS_COUNT] = {};

#define NULL_FN ((timer_cb)0)

volatile uint32_t timer_jiffies = 0;

// only called from main thread
uint32_t __reentrant timers_jiffies(void) {
  uint32_t a;
  uint32_t b;
  do {
    a = timer_jiffies;
    b = timer_jiffies;
  } while (a != b);
  return a;
}

PIC_ASM("GLOBAL _timers_jiffies");

// only called from main thread
void __reentrant timers_arm(uint8_t idx, timer_cb cb, uint16_t time) {
  struct timer_state_t *timer = &timers[idx];
  // seq_lock is a busy flag (bit0).
  // ISR skips timers while main is updating multi-byte fields.
  timer->seq_lock = 1;
  timer->armed = 0;
  timer->cb = cb;
  timer->time_units = time;
  timer->time_units_left = time;
  ++timer->gen;
  timer->expired = 0;
  timer->armed = 1;
  timer->seq_lock = 0;
}

void __reentrant timers_dearm(uint8_t idx) {
  struct timer_state_t *timer = &timers[idx];
  timer->armed = 0;
  ++timer->gen;
}

PIC_ASM("GLOBAL _timers_dearm");

// only called by TMR0 interrupt

void timers_expired() {
  //    GPIO_RA4_SetHigh();
  ++timer_jiffies;
  uint8_t any_expired = 0;
  for (uint8_t i = 0; i < TIMERS_COUNT; ++i) {
    struct timer_state_t *timer = &timers[i];
    if (!timer->armed || timer->expired)
      continue;
    // main thread is touching this timer
    // skip it
    if (timer->seq_lock & 1)
      continue;

    uint16_t rem = timer->time_units_left;
    if (rem > 0) {
      --rem;
    }
    timer->time_units_left = rem;
    if (!rem) {
      timer->expired = 1;
      any_expired = 1;
    }
  }
  if (any_expired) {
    task_schedule_from_irq(TIMER_TASK);
  }
  //    GPIO_RA4_SetLow();
}

// main thread loop function

void timers_main() {
  for (uint8_t i = 0; i < TIMERS_COUNT; ++i) {
    struct timer_state_t *timer = &timers[i];
    pending_expiry_t *pending = &expired_cbs[i];
    pending->cb = NULL_FN;
    if (timer->armed && timer->expired) {
      timer->armed = 0;
      timer->expired = 0;
      pending->cb = timer->cb;
      pending->gen = timer->gen;
    }
  }
  // the callbacks are invoked after processing all
  // expiry in case if any of the callbacks would rearm a timer
  // with potentially a different callback
  for (uint8_t i = 0; i < TIMERS_COUNT; ++i) {
    struct timer_state_t *timer = &timers[i];
    pending_expiry_t *pending = &expired_cbs[i];
    if (pending->cb && timer->gen == pending->gen) {
      pending->cb();
    }
  }
}

void timers_initialize() {
  task_init(TIMER_TASK, timers_main, true);
  memset(timers, 0, sizeof(timers));
  memset(expired_cbs, 0, sizeof(expired_cbs));
  tmr_wheel_OverflowCallbackRegister(timers_expired);
  tmr_wheel_PeriodSet(TMR0_MAX_COUNT - TIMERS_TICK_COUNT_FOR_UNIT + 1);
  tmr_wheel_Write(TMR0_MAX_COUNT - TIMERS_TICK_COUNT_FOR_UNIT + 1);
  tmr_wheel_TMRInterruptEnable();
  tmr_wheel_Start();
}
