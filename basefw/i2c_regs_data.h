/*
 * File:   i2c_reg_data.h
 * Author: emtszeb
 *
 * Created on August 11, 2024, 5:57 PM
 */

#ifndef I2C_REG_DATA_H
#define I2C_REG_DATA_H

#include "tasks.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint8_t *i2c_reg_map;
extern volatile uint8_t *i2c_shadow_map;

struct i2c_write_listener_t {
  struct task_descr_t *task;
  struct i2c_write_listener_t *next;
};

struct i2c_reg_descr {
  struct i2c_write_listener_t *task;
  uint8_t writable : 1;
  uint8_t clear_on_read : 1;
  uint8_t written_by_isr : 1;
  uint8_t immutable : 1;
  uint8_t reserved : 4;
  uint8_t clear_on_read_mask;
  volatile uint8_t dirty_isr;
  uint8_t dirty_main;
  uint8_t invoke_task;
};

extern struct i2c_reg_descr i2c_reg_defs[];

#ifdef __cplusplus
}
#endif

#endif /* I2C_REG_DATA_H */
