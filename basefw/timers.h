/*
 * File:   timers.h
 * Author: fecja
 *
 * Created on December 31, 2025, 10:01 AM
 */

#ifndef TIMERS_H
#define TIMERS_H

#include <xc.h>

#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TIMERS_COUNT
#error "TIMERS_COUNT must be defined"
#endif

typedef void (*timer_cb)(void);

void __reentrant timers_arm(uint8_t idx, timer_cb cb, uint16_t time);
void __reentrant timers_dearm(uint8_t idx);

uint32_t __reentrant timers_jiffies(void);

void timers_intialize(void);

#ifdef __cplusplus
}
#endif

#endif /* TIMERS_H */
