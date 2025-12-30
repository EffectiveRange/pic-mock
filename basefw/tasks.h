/*
 * File:   tasks.h
 * Author: fecja
 *
 * Created on November 25, 2023, 9:35 AM
 */

#ifndef TASKS_H
#define TASKS_H

#include <stdbool.h>
#include <stdint.h>
#include <xc.h>

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct task_descr_t;

typedef struct task_descr_t __far *task_descr_ptr_t;


typedef void (*__far task_run_fun)(task_descr_ptr_t);


struct task_descr_t {
  // initialized by the client
  task_run_fun task_fn;
  void __far *task_state;
  bool suspended;
  // initialized by the runtime
  task_descr_ptr_t next;
  ATOMIC_UINT16 run_count;
  ATOMIC_UINT16 run_count_isr;
};

void tasks_initialize(void);
void tasks_deinitialize(void);
void register_tasks_module(void);

// Add task to main thread
// the task descriptor will be passed to the task,
// so it can freely manipulate the opaque state
// and modify the continuation function for the next callback
// The callback must not set or unset the callback pointer!
// The only valid operation is to set to a different valid callback
// The state pointer can be manipulated freely
// This must be called once, during module initialization
// the task state can be suspended or resumed through the
// task descriptor
void add_task(task_descr_ptr_t);
void remove_task(task_descr_ptr_t);

// Safe to call from IRQ thread
void schedule_task_from_irq(task_descr_ptr_t taskd);
// entry point from main thread
void run_tasks(void);

#ifdef __cplusplus
}
#endif

#endif /* TASKS_H */
