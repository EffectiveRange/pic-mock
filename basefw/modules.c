#include "modules.h"

#include <stdlib.h>

static struct {
  module_ptr_t head;
} _modules = {NULL};

void add_module(module_ptr_t module) {
  module->next = _modules.head;
  _modules.head = module;
}

void initialize_modules(void) {
  for (module_ptr_t mod = _modules.head; mod != NULL; mod = mod->next) {
    mod->init();
  }
  for (module_ptr_t mod = _modules.head; mod != NULL; mod = mod->next) {
    if (mod->register_events) {
      mod->register_events();
    }
  }
}
void deinitialize_modules(void) {
  for (module_ptr_t mod = _modules.head; mod != NULL; mod = mod->next) {
    if (mod->deregister_events) {
      mod->deregister_events();
    }
  }
  for (module_ptr_t mod = _modules.head; mod != NULL; mod = mod->next) {
    mod->deinit();
  }
}