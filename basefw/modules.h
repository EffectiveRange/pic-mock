#ifndef MODULES_H
#define MODULES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
struct module_t {
  // required
  void (*init)(void);
  void (*deinit)(void);
  // optional
  void (*register_events)(void);
  void (*deregister_events)(void);
  // initialized by framework
  struct module_t *next;
};

void add_module(struct module_t *module);
void initialize_modules(void);
void deinitialize_modules(void);

#ifdef __cplusplus
}
#endif

#endif /* MODULES_H */
