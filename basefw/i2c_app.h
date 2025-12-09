/*
 * File:   i2c_app.h
 * Author: emtszeb
 *
 * Created on August 2, 2024, 5:33 PM
 */

#ifndef I2C_APP_H
#define I2C_APP_H
#include <stdint.h>
#include <stdbool.h>
#include <mcc_generated_files/i2c_client/i2c_client_types.h>

#ifdef __cplusplus
extern "C" {
#endif



#define I2C_TRY_CNT 3U
#define I2C_TMOUT_MS 100U // todo check if lower is better

void i2c_app_initialize();
void i2c_app_deinitialize(void);
void register_i2c_app_module(uint8_t client_register_cnt, void (*initcb)(void));

extern bool isr_i2c_client_application(i2c_client_transfer_event_t event);

struct i2c_write_listener_t;

void i2c_register_write_listener(uint8_t address,
                                 struct i2c_write_listener_t *task);
void i2c_deregister_write_listener(uint8_t address,
                                   struct i2c_write_listener_t *task);

void i2c_client_write_register_byte(uint8_t address, uint8_t data);
void i2c_client_write_register_bit(uint8_t address, uint8_t pos, uint8_t val);

#ifdef __cplusplus
}
#endif

#endif /* I2C_APP_H */
