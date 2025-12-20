/*
 * File:   main.h
 * Author: emtszeb
 *
 * Created on August 1, 2024, 8:38 AM
 */

#ifndef MAIN_H
#define MAIN_H

#include <xc.h>


#if !defined(MAIN_THREAD_YIELD)
#define MAIN_THREAD_YIELD()
#endif
#if !defined(MAIN_THREAD_CYCLE_BEGIN)
#define MAIN_THREAD_CYCLE_BEGIN()
#endif
#if !defined(MAIN_THREAD_CYCLE_END)
#define MAIN_THREAD_CYCLE_END()
#endif

#if !defined(MAIN_THREAD_EXCLUSIVE_BEGIN)
#define MAIN_THREAD_EXCLUSIVE_BEGIN()
#endif
#if !defined(MAIN_THREAD_EXCLUSIVE_END)
#define MAIN_THREAD_EXCLUSIVE_END()
#endif

#if !defined(ISR_SAFE_BEGIN)
#define ISR_SAFE_BEGIN()
#endif

#if !defined(ISR_SAFE_END)
#define ISR_SAFE_END()
#endif

#ifdef __cplusplus
extern "C" {
#endif

void init_application(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
