/* 
 * File:   tasks.h
 * Author: fecja
 *
 * Created on December 31, 2025, 10:10 AM
 */

/**
 * @file tasks.h
 * @ingroup scheduler
 * @brief Simple cooperative task scheduler with ISR-triggered dispatch.
 *
 * This module implements a lightweight cooperative task scheduler intended
 * for single-core microcontrollers. Tasks are executed only from main thread
 * context and are never invoked from interrupt context.
 *
 * Tasks may be scheduled from interrupt context or main thread context.
 * Multiple scheduling requests may be coalesced; tasks are guaranteed to run
 * at least once after being scheduled, but the exact number of runs is not
 * guaranteed.
 */

#ifndef TASKS_H
#define	TASKS_H

#include "application.h"

#include <stdbool.h>
#include <xc.h>

#ifdef	__cplusplus
extern "C" {
#endif

  
#ifndef TASK_COUNT
#error "TASK_COUNT must be defined"
#endif

/**
 * @ingroup scheduler
 * @brief Task entry function type.
 *
 * Task functions are executed from main thread context only.
 * They must not block indefinitely and should return promptly.
 *
 * Tasks are executed in a one-shot manner: each scheduling request causes
 * at most one invocation, unless the task explicitly reschedules itself.
 */
typedef void (*task_run_fun)(void); 

/**
 * @ingroup scheduler
 * @brief Initializes a task descriptor.
 *
 * Must be called from main thread context during system initialization,
 * before `task_main()` is entered.
 *
 * @param idx Index of the task (0..TASK_COUNT-1).
 * @param fun Task entry function.
 * @param initial_running If true, the task will be executed once when
 *                        `task_main()` starts.
 */
void task_init(uint8_t idx,task_run_fun fun,bool initial_running);

/**
 * @ingroup scheduler
 * @brief Schedules a task from interrupt context.
 *
 * This function is safe to call from any interrupt service routine.
 *
 * Multiple calls to this function for the same task may be coalesced;
 * the task is guaranteed to be executed at least once after being scheduled,
 * but not necessarily once per call.
 *
 * @param idx Index of the task (0..TASK_COUNT-1).
 */
void __reentrant task_schedule_from_irq(uint8_t idx);

/**
 * @ingroup scheduler
 * @brief Schedules a task from main thread context.
 *
 * This function schedules the specified task to be executed once
 * by the scheduler. If the task is already pending execution,
 * the request is coalesced.
 *
 * @param idx Index of the task (0..TASK_COUNT-1).
 */
void __reentrant task_schedule(uint8_t idx);

/**
 * @ingroup scheduler
 * @brief Runs the cooperative task scheduler.
 *
 * This function must be called from main thread context and does not return
 * while the scheduler is active.
 *
 * Tasks are executed sequentially in index order. Each scheduled task is
 * executed at most once per scheduling event. Tasks may reschedule themselves
 * or other tasks.
 *
 * No task is ever executed recursively, and tasks are never executed from
 * interrupt context.
 */
void task_main(void);

#ifdef	__cplusplus
}
#endif

#endif	/* TASKS_H */

