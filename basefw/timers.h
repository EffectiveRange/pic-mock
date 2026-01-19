/*
 * File:   timers.h
 * Author: fecja
 *
 * Created on December 31, 2025, 10:01 AM
 */

#ifndef TIMERS_H
#define TIMERS_H

#include "application.h"
#include <stdint.h>
#include <xc.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TIMERS_COUNT
#error "TIMERS_COUNT must be defined"
#endif

#ifndef TIMERS_TICK_COUNT_FOR_UNIT
#error "TIMERS_TICK_COUNT_FOR_UNIT must be defined"
#endif

typedef void (*timer_cb)(void);

/**
 * @ingroup timer
 * @brief Arms (or re-arms) the specified timer with a callback.
 *
 * Must be called from main thread context only.
 *
 * Re-arming an already armed timer is allowed and replaces its configuration.
 * If an expiry callback from a previous arming is already pending delivery,
 * it will be suppressed (generation-token based cancellation).
 *
 * Timing note: if arming overlaps the 1 ms TMR0 tick ISR, the first decrement
 * may be skipped, so the expiry may occur up to 1 ms later than the requested
 * time (bounded slippage of +1 tick).
 *
 * @param idx  Index of the timer (0..TIMERS_COUNT-1).
 * @param cb   Callback invoked from main thread context when the timer expires.
 * @param time Expiry time in time units defined by TIMERS_TICK_COUNT_FOR_UNIT.
 */
void __reentrant timers_arm(uint8_t idx, timer_cb cb, uint16_t time);

/**
 * @ingroup timer
 * @brief Cancels (de-arms) the specified timer.
 *
 * Must be called from main thread context only.
 *
 * This function guarantees that, after it returns, no expiry callback for
 * this timer will be delivered unless the timer is re-armed. If a callback
 * was already pending delivery from a previous expiry, it is suppressed
 * (generation-token based cancellation).
 *
 * @param idx Index of the timer (0..TIMERS_COUNT-1).
 */
void __reentrant timers_dearm(uint8_t idx);

/**
 * @ingroup timer
 * @brief Returns a monotonically increasing tick counter in units defined by
 * TIMERS_TICK_COUNT_FOR_UNIT.
 *
 * Must be called from main thread context only.
 *
 * The returned value increments every 1 ms while the timer tick ISR is running.
 * Wraps around after 2^32 ticks (~49.7 days).
 */
uint32_t __reentrant timers_jiffies(void);

/**
 * @ingroup timer
 * @brief Initializes the timer subsystem for the 1 ms timer tick.
 *
 * Must be called only once from main thread context during system
 * initialization, before any other timer API is used and global interrupts
 * are enabled.
 *
 * This function initializes all timer slots to a disarmed state, resets
 * internal bookkeeping, registers the TMR0 overflow ISR callback, and
 * configures/enables the hardware timer used as the 1 ms tick source.
 *
 * @note No timer expiry processing occurs until global interrupts are enabled
 *       by the application.
 */
void timers_initialize(void);

#ifdef __cplusplus
}
#endif

#endif /* TIMERS_H */
