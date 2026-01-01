/*
 * File:   i2c_app.h
 * Author: emtszeb
 *
 * Created on August 2, 2024, 5:33 PM
 */

#ifndef I2C_APP_H
#define I2C_APP_H
#include <mcc_generated_files/i2c_client/i2c_client_types.h>
#include <stdbool.h>
#include <stdint.h>
#include <xc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_TRY_CNT 3U
#define I2C_TMOUT_MS 100U // todo check if lower is better

void i2c_app_initialize(void);

extern bool __reentrant
isr_i2c_client_application(i2c_client_transfer_event_t event);

void __reentrant i2c_client_write_register_byte(uint8_t address, uint8_t data);
void __reentrant i2c_client_write_register_bit(uint8_t address, uint8_t pos,
                                               uint8_t val);

#ifdef __cplusplus
}
#endif

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

#endif /* I2C_APP_H */
