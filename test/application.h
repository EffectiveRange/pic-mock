/*
 * File:   application.h
 * Author: fecja
 *
 * Created on December 31, 2025, 10:01 AM
 */

#ifndef APPLICATION_H
#define APPLICATION_H

#ifdef __cplusplus
extern "C" {
#endif

#define TIMERS_TICK_COUNT_FOR_UNIT 3
#define TIMERS_COUNT 5
#define TIMER_0 0
#define TIMER_1 1
#define TIMER_2 2
#define TIMER_3 3
#define TIMER_4 4

#define TASK_COUNT 2
#define TIMER_TASK 0
#define I2C_TASK 1

#define INVALID_TASK 0xFF

#define I2C_CLIENT_LOCATION_SIZE 32

#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_H */
