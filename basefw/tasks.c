#include "tasks.h"
#include "main.h"

#include <stdbool.h>

struct task_descr_t {
    task_run_fun task_fn;
    bool running;
    uint8_t run_count;
    volatile uint8_t run_count_isr;
};

struct task_descr_t tasks[TASK_COUNT] = {};

void task_init(uint8_t idx, task_run_fun fun, bool initial_running) {
    struct task_descr_t* task = &tasks[idx];
    task->task_fn = fun;
    task->running = initial_running;
    task->run_count = 0;
    task->run_count_isr = 0;
}

void __reentrant task_schedule_from_irq(uint8_t idx) {
    ++tasks[idx].run_count_isr;
}

void __reentrant task_schedule(uint8_t idx) {
    tasks[idx].running = true;
}

void task_main(void) {
    while (TASKS_MAIN_RUNNING) {
        for (uint8_t i = 0; i < TASK_COUNT; ++i) {
            struct task_descr_t* task = &tasks[i];
            uint8_t curr = task->run_count_isr;
            if (task->run_count != curr) {
                task->run_count = curr;
                task->running = true;
            }
            if (task->running) {
                task->running = false;
                // clear running first in case
                // cb reschedules it
                task->task_fn();
            }
        }
    }
}
