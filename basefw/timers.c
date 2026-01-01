#include <stdbool.h>
#include <xc.h>

#include "application.h"
#include "tasks.h"
#include "timers.h"

#include "mcc_generated_files/timer/tmr0.h"

struct timer_state_t {
  timer_cb cb;
  uint16_t time_ms;
  /// irq safe variables
  volatile uint16_t time_ms_left;
  volatile bool armed;
  volatile bool expired;
};

struct timer_state_t timers[TIMERS_COUNT] = {};

volatile uint32_t timer_jiffies = 0;

uint32_t __reentrant timers_jiffies(void) {
  uint32_t a = 0;
  uint32_t b = 0;
  do {
    a = timer_jiffies;
    b = timer_jiffies;
  } while (a != b);
  return a;
}

void timers_arm(uint8_t idx, timer_cb cb, uint16_t time) {
  timers[idx].armed = false;
  timers[idx].cb = cb;
  timers[idx].time_ms = time;
  timers[idx].time_ms_left = time;
  timers[idx].expired = false;
  timers[idx].armed = true;
}

void __reentrant timers_dearm(uint8_t idx) { timers[idx].armed = false; }

void timers_expired() {
  timer_jiffies = timer_jiffies + 1;
  bool any_expired = false;
  for (int i = 0; i < TIMERS_COUNT; ++i) {
    struct timer_state_t *timer = &timers[i];
    if (timer->armed && !timer->expired) {
      uint16_t rem = timer->time_ms_left;
      timer->time_ms_left = rem > 0 ? rem - 1 : 0;
      if (timer->time_ms_left == 0) {
        timer->expired = true;
        any_expired = true;
      }
    }
  }
  if (any_expired) {
    task_schedule_from_irq(TIMER_TASK);
  }
  tmr_wheel_Reload();
}

bool timers_main() {
  tmr_wheel_Stop();
  tmr_wheel_TMRInterruptDisable();
  for (int i = 0; i < TIMERS_COUNT; ++i) {
    struct timer_state_t *timer = &timers[i];
    if (timer->expired) {
      timer->expired = false;
      timer->armed = false;
      timer->cb();
    }
  }
  tmr_wheel_TMRInterruptEnable();
  tmr_wheel_Start();
  return false;
}

void timers_intialize() {
  // 1 ms interrupts
  // TMR0 must be configured for 1MHz clock
  task_init(TIMER_TASK, timers_main, true);
  tmr_wheel_OverflowCallbackRegister(timers_expired);
  tmr_wheel_PeriodSet(TMR0_MAX_COUNT - 1000 + 1);
  tmr_wheel_Reload();
  tmr_wheel_Start();
}
