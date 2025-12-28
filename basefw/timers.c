#include "timers.h"
#include "debug.h"
#include "main.h"
#include "mcc_generated_files/system/system.h"
#include "mcc_generated_files/timer/tmr0.h"
#include "modules.h"
#include "tasks.h"
#include "xc.h"

#include <assert.h>
#include <limits.h>
#include <stdint.h>

#define MIN_TMR_DELTA                                                          \
  ((TMR_PRESCALE + SYS_CLOCK_FREQ_KHZ - 1) / SYS_CLOCK_FREQ_KHZ)

static struct tw_timer_t sentinel = {0, NULL, NULL, false, NULL, NULL, 0};

static struct timer_wheel_t {
  struct tw_timer_t *head;
} _wheel = {&sentinel};

static struct tw_timer_t *find_last_non_zero_prev(struct tw_timer_t *head) {
  struct tw_timer_t *curr = head;
  while (curr->next != &sentinel) {
    if (curr->next->time_ms != 0) {
      break;
    }
    curr = curr->next;
  }
  return curr;
}

static struct tw_timer_t *find_first_non_expired() {
  struct tw_timer_t *curr = _wheel.head;
  for (; curr != &sentinel && curr->expired; curr = curr->next) {
  }
  return curr;
}

// time ms is already clamped to TIMER_MAX_MS in add_timer()
static uint16_t time_to_tmr(uint16_t time_ms) {
  uint32_t val = ((uint32_t)time_ms * SYS_CLOCK_FREQ_KHZ) / TMR_PRESCALE;
  return (uint16_t)val;
}

void link_timer_before(struct tw_timer_t *timer, struct tw_timer_t *next) {
  timer->next = next;
  timer->prev = next->prev;
  if (next->prev != NULL) {
    next->prev->next = timer;
  } else {
    _wheel.head = timer;
  }
  next->prev = timer;
}
void link_timer_after(struct tw_timer_t *prev, struct tw_timer_t *timer) {
  prev->next->prev = timer;
  timer->next = prev->next;
  prev->next = timer;
  timer->prev = prev;
}

static void add_future_timer_to_wheel(uint16_t tmr_left,
                                      struct tw_timer_t *head,
                                      struct tw_timer_t *timer) {
  // later time
  // find position,
  // update subsequent time_us by subtracting the diff between
  uint16_t accu = tmr_left;
  // TODO: handle accu overflow
  uint16_t delta = 0;
  struct tw_timer_t *curr = head;

  while (curr->next != &sentinel) {
    if (timer->time_ms <= accu + curr->next->time_ms) {
      break;
    }
    accu += curr->next->time_ms;
    curr = curr->next;
  }

  delta = timer->time_ms - accu;
  link_timer_after(curr, timer);
  timer->time_ms = delta;
  if (timer->next != &sentinel) {
    timer->next->time_ms -= delta;
  }
}

void log_timers() {
  log_debug("timers:");
#if defined(MRHAT_FW_DEBUG_LOG_ENABLED)
  for (struct tw_timer_t *curr = _wheel.head; curr != &sentinel;
       curr = curr->next) {
    log_debug("  timer %p, time_ms %d, expired %d", curr, curr->time_ms,
              curr->expired);
  }
#endif
}

static void add_timer_unsafe(struct tw_timer_t *timer) {
  log_timers();
  uint16_t tmr_left = 0;
  uint16_t tmr_delta = 0;
  struct tw_timer_t *pos = NULL;
  struct tw_timer_t *prev_head = NULL;
  struct tw_timer_t *head = find_first_non_expired();
  timer->time_ms_orig =
      timer->time_ms < TIMER_MAX_MS ? timer->time_ms : TIMER_MAX_MS;
  timer->time_ms = time_to_tmr(timer->time_ms);
  timer->expired = false;
  log_debug("Adding timer %p with tmr value %d and time ms %d", timer,
            timer->time_ms, timer->time_ms_orig);

  if (head == &sentinel) {
    link_timer_before(timer, head);
    tmr_wheel_Write(UINT16_MAX - timer->time_ms);
  } else {
    // determine how much time until next timer firing
    // if there are still expired timers, it means the timer
    // is not armed, so tmr_left is 0
    tmr_left = head == _wheel.head ? UINT16_MAX - tmr_wheel_Read() : 0;
    if (head == _wheel.head && tmr_left - MIN_TMR_DELTA <= timer->time_ms &&
        timer->time_ms <= tmr_left + MIN_TMR_DELTA) {
      // practically same time, resolution is +- MIN_TMR_DELTA
      pos = find_last_non_zero_prev(head);
      timer->time_ms = 0;
      link_timer_after(pos, timer);
    } else if (head == _wheel.head &&
               timer->time_ms < tmr_left - MIN_TMR_DELTA) {
      // new arm
      tmr_wheel_Write(UINT16_MAX - timer->time_ms);
      head->time_ms -= timer->time_ms;
      link_timer_before(timer, head);
    } else {
      add_future_timer_to_wheel(tmr_left, head, timer);
    }
  }
  log_timers();
}

struct tw_timer_t *head_timer() { return _wheel.head; }
struct tw_timer_t *sentinel_timer() { return &sentinel; }

static struct tw_timer_t *find_timer(struct tw_timer_t *timer) {
  struct tw_timer_t *curr = _wheel.head;
  while (curr != &sentinel) {
    if (curr == timer) {
      break;
    }
    curr = curr->next;
  }
  return curr;
}

static bool remove_timer_unsafe2(struct tw_timer_t *timer) {
  uint16_t tmr_val = tmr_wheel_Read();
  uint16_t tmr_val_check = tmr_val;
  uint16_t accu = 0;
  // remove head timer
  if (timer->prev == NULL) {
    _wheel.head = timer->next;
    timer->next->prev = NULL;
    // if not solo timer
    if (timer->next != &sentinel) {
      // when next timer is around the same time
      // then simply propagate the time_ms to next timer
      if (timer->next->time_ms == 0) {
        timer->next->time_ms = timer->time_ms;
      } else {
        // We already cap max timer value in add_timer()
        // therefore it's logically impossible
        // that the subsequent timer is less than the remaining time to fire
        assert(tmr_val >= timer->time_ms);
        // need to rearm timer (subtract from current tmr value)
        // already in tmr format
        tmr_val -= timer->next->time_ms;
        tmr_wheel_Write(tmr_val);
      }
    } else {
      // only the sentinel was left, no need to restart timer
      return false;
    }
  } else {
    // unlinking timer from the list
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    // propagating time_ms to next timer
    if (timer->next != &sentinel) {
      accu = timer->next->time_ms + timer->time_ms;
      if (accu < timer->next->time_ms) {
        timer->next->time_ms = UINT16_MAX;
      } else {
        timer->next->time_ms = accu;
      }
    }
  }
  return true;
}

static bool remove_timer_unsafe(struct tw_timer_t *timer) {
  if (find_timer(timer) != &sentinel) {
    return remove_timer_unsafe2(timer);
  }
  return false;
}

void remove_timer(struct tw_timer_t *timer) {
  tmr_wheel_Stop();
  tmr_wheel_TMRInterruptDisable();
  remove_timer_unsafe(timer);
  if (_wheel.head != &sentinel && !_wheel.head->expired) {
    tmr_wheel_TMRInterruptEnable();
    tmr_wheel_Start();
  }
}

void add_timer(struct tw_timer_t *timer) {
  tmr_wheel_Stop();
  tmr_wheel_TMRInterruptDisable();
  ISR_SAFE_BEGIN();
  if (find_timer(timer) != &sentinel) {
    remove_timer_unsafe2(timer);
  }
  add_timer_unsafe(timer);
  ISR_SAFE_END();
  tmr_wheel_TMRInterruptEnable();
  tmr_wheel_Start();
}

void TimerWheelMain(struct task_descr_t *taskd) {
  tmr_wheel_Stop();
  tmr_wheel_TMRInterruptDisable();
  struct tw_timer_t *curr = NULL;
  struct tw_timer_t *old_head = _wheel.head;
  struct tw_timer_t *new_head = _wheel.head;
  struct tw_timer_t *res = NULL;
  bool expired = false;
  int cb_count = 0;
  if (_wheel.head != &sentinel && _wheel.head->expired) {
    curr = _wheel.head;
    _wheel.head = _wheel.head->next;
    _wheel.head->prev = NULL;
    // propagate time ms to next expired timer
    // if timers were simultaneously expired
    if (_wheel.head->expired && _wheel.head->time_ms == 0) {
      _wheel.head->time_ms = curr->time_ms;
    }
    curr->time_ms = curr->time_ms_orig;
    log_debug("invoking callback %p", curr);
    res = curr->callback(curr);
    expired = true;
  }
  if (res != NULL) {
    add_timer_unsafe(res);
  }
  if (!_wheel.head->expired || _wheel.head == &sentinel) {
    if (_wheel.head != &sentinel) {
      // already in TMR format
      tmr_wheel_Write(UINT16_MAX - _wheel.head->time_ms);
      tmr_wheel_TMRInterruptEnable();
      tmr_wheel_Start();
    }
    taskd->suspended = true;
  }
}

PIC_ASM("GLOBAL _TimerWheelMain");

static struct task_descr_t timer_task = {
    .task_fn = TimerWheelMain,
    .task_state = NULL,
    .suspended = true,
    .next = NULL,
};

// mutally  exclusive with timer wheel main/add/remove timer
// functions as those stop the timer
static void on_timer_expired() {
  log_timers();
  ISR_SAFE_BEGIN();
  struct tw_timer_t *curr = _wheel.head;
  while (curr != &sentinel) {
    curr->expired = true;
    log_debug("expired: %p next: %p", curr, curr->next);
    curr = curr->next;
    if (curr->time_ms != 0) {
      break;
    }
  }
  ISR_SAFE_END();
  schedule_task_from_irq(&timer_task);
}

void timers_deinitialize() {
  tmr_wheel_Stop();
  tmr_wheel_TMRInterruptDisable();
  remove_task(&timer_task);
}

void timers_reset() {
  tmr_wheel_Stop();
  tmr_wheel_TMRInterruptDisable();
  _wheel.head = &sentinel;
  sentinel.prev = NULL;
  sentinel.next = NULL;
  sentinel.time_ms = 0;
  sentinel.expired = false;
}

void timers_initialize() {
  timers_deinitialize();
  add_task(&timer_task);
  timers_reset();
  tmr_wheel_OverflowCallbackRegister(on_timer_expired);
}

static struct module_t _timer_module = {
    .init = timers_initialize,
    .deinit = timers_deinitialize,
    .register_events = NULL,
    .deregister_events = NULL,
    .next = NULL,
};

void register_timer_module(void) { add_module(&_timer_module); }
