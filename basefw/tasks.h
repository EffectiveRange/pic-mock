/* 
 * File:   tasks.h
 * Author: fecja
 *
 * Created on December 31, 2025, 10:10 AM
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

typedef bool (*task_run_fun)(void); 
void task_init(uint8_t idx,task_run_fun fun,bool initial_running); 
void __reentrant task_schedule_from_irq(uint8_t idx);
void __reentrant task_schedule(uint8_t idx);
void task_main(void);

#ifdef	__cplusplus
}
#endif

#endif	/* TASKS_H */

