#pragma once

#define _XC_H_
#define __bit unsigned char
#define __CCI__
#define _LIB_BUILD

#define __interrupt(...)

#include <pic18f16q20.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void execSleep(void);

#define __delay_us(x)

#define __nop()

#define __far

#define SLEEP() execSleep()
#define MAIN_THREAD_YIELD() execSleep()
#define MAIN_THREAD_CYCLE_BEGIN() _MAIN_THREAD_CYCLE_BEGIN()
#define MAIN_THREAD_CYCLE_END() _MAIN_THREAD_CYCLE_END()
#define MAIN_THREAD_EXCLUSIVE_BEGIN() _MAIN_THREAD_EXCLUSIVE_BEGIN()
#define MAIN_THREAD_EXCLUSIVE_END() _MAIN_THREAD_EXCLUSIVE_END()
#define ISR_SAFE_BEGIN() _MAIN_THREAD_EXCLUSIVE_BEGIN()
#define ISR_SAFE_END() _MAIN_THREAD_EXCLUSIVE_END()
#define PIC_ASM(x)

void _MAIN_THREAD_CYCLE_BEGIN(void);
void _MAIN_THREAD_CYCLE_END(void);
void _MAIN_THREAD_EXCLUSIVE_BEGIN(void);
void _MAIN_THREAD_EXCLUSIVE_END(void);

#ifdef __cplusplus
}
#endif