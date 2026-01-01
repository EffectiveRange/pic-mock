#include "tasks.h"

#include <stdbool.h>

struct task_descr_t {
  task_run_fun task_fn;
  bool running;
  uint8_t run_count;
  volatile uint8_t run_count_isr;
};

struct task_descr_t tasks[TASK_COUNT] = {};

void task_init(uint8_t idx, task_run_fun fun, bool initial_running) {
  tasks[idx].task_fn = fun;
  tasks[idx].running = initial_running;
  tasks[idx].run_count = 0;
  tasks[idx].run_count_isr = 0;
}

void __reentrant task_schedule_from_irq(uint8_t idx) {
  tasks[idx].run_count_isr = tasks[idx].run_count_isr + 1;
}

void __reentrant task_schedule(uint8_t idx) { tasks[idx].running = true; }

void task_main(void) {
  while (TASKS_MAIN_RUNNING) {
    for (int i = 0; i < TASK_COUNT; ++i) {
      uint8_t curr = tasks[i].run_count_isr;
      if (tasks[i].run_count != curr) {
        tasks[i].run_count = curr;
        tasks[i].running = true;
      }
      if (tasks[i].running) {
        tasks[i].running = tasks[i].task_fn();
      }
    }
  }
}
