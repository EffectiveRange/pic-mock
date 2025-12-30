#ifndef MODULES_H
#define MODULES_H

#include <stdbool.h>
#include <stdint.h>

#include <xc.h>

#ifdef __cplusplus
extern "C" {
#endif
struct module_t {
  // required
  void (*__far init)(void);
  void (*__far deinit)(void);
  // optional
  void (*__far register_events)(void);
  void (*__far deregister_events)(void);
  // initialized by framework
  struct module_t *next;
};

typedef struct module_t __far *module_ptr_t;

void add_module(module_ptr_t module);
void initialize_modules(void);
void deinitialize_modules(void);

#ifdef __cplusplus
}
#endif

#endif /* MODULES_H */
