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

extern volatile uint8_t __far *i2c_reg_map;
extern volatile uint8_t __far *i2c_shadow_map;

struct i2c_write_listener_t;
typedef struct i2c_write_listener_t __far *i2c_write_listener_ptr_t;

struct i2c_write_listener_t {
  task_descr_ptr_t task;
  i2c_write_listener_ptr_t next;
};

struct i2c_reg_descr {
  i2c_write_listener_ptr_t task;
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

/**************************************************
 *
 * BYTE 0x0 : ALWAYS ZERO
 *
 * ************************************************/
// BYTE 0x00 - ALWAYS ZERO
#define REG_ALWAYS_ZERO_ADDR 0x00

/**************************************************
 *
 * BYTE 0x1-0x9 : WRITABLE REGISTERS
 *
 * ************************************************/

// BYTE 0x1 STICKY BIY and I2C_ERR_STATUS
// reg 1: REG_VAL_I2C_CLIENT_ERRORs + STICKY bit ( i2c err + | 0x80 sticky bit)
//   bit0-6: i2c err status
//   bit7:   always 1
#define REG_STAT_I2C_ERR_AND_STICKY_ADDR 0x1
#define REG_VAL_I2C_CLIENT_ERROR_NONE 0
#define REG_VAL_I2C_CLIENT_ERROR_BUS_COLLISION 1
#define REG_VAL_I2C_CLIENT_ERROR_WRITE_COLLISION 2
#define REG_VAL_I2C_CLIENT_ERROR_RECEIVE_OVERFLOW 3
#define REG_VAL_I2C_CLIENT_ERROR_TRANSMIT_UNDERFLOW 4
#define REG_VAL_I2C_CLIENT_ERROR_READ_UNDERFLOW 5

#define REG_STAT_I2C_ERR_BUS_COLLISION_CNTR 0x2
#define REG_STAT_I2C_ERR_WRITE_COLLISION_CNTR 0x3
#define REG_STAT_I2C_ERR_RECEIVE_OVERFLOW_CNTR 0x4
#define REG_STAT_I2C_ERR_TRANSMIT_UNDERFLOW_CNTR 0x5
#define REG_STAT_I2C_ERR_READ_UNDERFLOW_CNTR 0x6
#define REG_STAT_I2C_ERR_UNKNOWN_CNTR 0x7

#ifdef __cplusplus
}
#endif

#endif /* I2C_REG_DATA_H */
