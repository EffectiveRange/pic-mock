#include "tasks.h"
#include "common.h"
#include "main.h"
#include "mcc_generated_files/system/interrupt.h"
#include "mcc_generated_files/system/system.h"
#include "modules.h"
#include "xc.h"
#include <stdlib.h>

static ATOMIC_UINT16 _tasks_initialized = 0;

static struct task_descr_t _sentinel = {NULL, NULL, 1, NULL};

static struct TaskList_t {
  struct task_descr_t *task;
} _tasks_list = {&_sentinel};

void tasks_initialize() { _tasks_initialized = 1; }

void tasks_deinitialize(void) { _tasks_initialized = 0; }

void add_task(struct task_descr_t *taskd) {
  taskd->next = _tasks_list.task;
  taskd->run_count_isr = 0;
  taskd->run_count = 0;
  _tasks_list.task = taskd;
}

void remove_task(struct task_descr_t *taskd) {
  if (_tasks_list.task == taskd) {
    _tasks_list.task = taskd->next;
    return;
  }
  for (struct task_descr_t *t = _tasks_list.task; t != &_sentinel;
       t = t->next) {
    if (t->next == taskd) {
      t->next = t->next->next;
      break;
    }
  }
}

static void __idle() { MAIN_THREAD_YIELD(); };

void schedule_task_from_irq(struct task_descr_t *taskd) {
  ISR_SAFE_BEGIN();
  taskd->run_count_isr++;
  ISR_SAFE_END();
}

void run_tasks() {

  while (true) {
    MAIN_THREAD_CYCLE_BEGIN();
    if (!_tasks_initialized) {
      MAIN_THREAD_CYCLE_END();
      break;
    }
    bool task_run = false;
    for (struct task_descr_t *task = _tasks_list.task; task != &_sentinel;
         task = task->next) {
      if (!task->suspended) {
        task_run = true;
        task->task_fn(task);
      }
    }
    if (!task_run) {
      __idle();
    }
    // at the end of each cycle propagate from the IRQ thread suspensions
    // TSAN suppression added for this section
    for (struct task_descr_t *task = _tasks_list.task; task != &_sentinel;
         task = task->next) {
      ISR_SAFE_BEGIN();
      uint16_t irq_count = task->run_count_isr;
      ISR_SAFE_END();
      if (irq_count != task->run_count) {
        task->suspended = false;
        task->run_count = irq_count;
      }
    }
    MAIN_THREAD_CYCLE_END();
  }
}

static struct module_t tasks_module = {
    .init = tasks_initialize,
    .deinit = tasks_deinitialize,
    .register_events = NULL,
    .deregister_events = NULL,
    .next = NULL,
};

void register_tasks_module(void) { add_module(&tasks_module); }