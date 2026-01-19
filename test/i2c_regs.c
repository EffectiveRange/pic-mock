#include "application.h"
#include "version.h"

#include <stddef.h>

// clang-format off
static uint8_t i2c_reg_map[I2C_CLIENT_LOCATION_SIZE] = {
 0x00, // 0x00, // always 0
 0x80, // 0x01  // REG_STAT_I2C_ERR_AND_STICKY_ADDR
 0x00, // 0x02  // REG_STAT_I2C_ERR_BUS_COLLISION_CNTR
 0x00, // 0x03  // REG_STAT_I2C_ERR_WRITE_COLLISION_CNTR
 0x00, // 0x04  // REG_STAT_I2C_ERR_RECEIVE_OVERFLOW_CNTR
 0x00, // 0x05  // REG_STAT_I2C_ERR_TRANSMIT_UNDERFLOW_CNTR
 0x00, // 0x06  // REG_STAT_I2C_ERR_READ_UNDERFLOW_CNTR
 0x00, // 0x07  // REG_STAT_I2C_ERR_UNKNOWN_CNTR
 0x00, // 0x08  // REG_CMD_HALT_ACTION
 0x00, // 0x09  // UNUSED
 0x00, // 0x0A  // REG_STAT_0_ADDR
 0x00, // 0x0B  // REG_BAT_STAT_0_ADDR
 0x00, // 0x0C  // REG_IBAT_ADDR[0]
 0x00, // 0x0D  // REG_IBAT_ADDR[1]
 0x00, // 0x0E  // UNUSED
 0x00, // 0x0F  // UNUSED
 0x00, // 0x10  // UNUSED
 ER_FW_MAJOR, // 0x11 // FW major version
 ER_FW_MINOR, // 0x12 // FW minor version
 ER_FW_PATCH // 0x13 // FW patch version
};

struct i2c_reg_descr i2c_reg_defs[I2C_CLIENT_LOCATION_SIZE]= {
    //task,         
    //             cor,
    //                wi,
    //                   imm,
    //                      rsv,
    //                       cor_mask,
    //                            di,
    //                               dm,
    //                                  it,
    { INVALID_TASK, 0, 0, 1, 0, 0, 0, 0, 0,}, // 0x00  // always 0
    { INVALID_TASK, 1, 1, 0, 0, 0x80, 0, 0, 0,}, // 0x01  // REG_STAT_I2C_ERR_AND_STICKY_ADDR
    { INVALID_TASK, 1, 1, 0, 0, 0, 0, 0, 0,}, // 0x02  // REG_STAT_I2C_ERR_BUS_COLLISION_CNTR
    { INVALID_TASK, 1, 1, 0, 0, 0, 0, 0, 0,}, // 0x03  // REG_STAT_I2C_ERR_WRITE_COLLISION_CNTR
    { INVALID_TASK, 1, 1, 0, 0, 0, 0, 0, 0,}, // 0x04  // REG_STAT_I2C_ERR_RECEIVE_OVERFLOW_CNTR
    { INVALID_TASK, 1, 1, 0, 0, 0, 0, 0, 0,}, // 0x05  // REG_STAT_I2C_ERR_TRANSMIT_UNDERFLOW_CNTR
    { INVALID_TASK, 1, 1, 0, 0, 0, 0, 0, 0,}, // 0x06  // REG_STAT_I2C_ERR_READ_UNDERFLOW_CNTR
    { INVALID_TASK, 1, 1, 0, 0, 0, 0, 0, 0,}, // 0x07  // REG_STAT_I2C_ERR_UNKNOWN_CNTR
    { INVALID_TASK, 0, 0, 0, 0, 0, 0, 0, 0,}, // 0x08  // REG_CMD_HALT_ACTION
    { INVALID_TASK, 0, 0, 0, 0, 0, 0, 0, 0,}, // 0x09  // UNUSED
    { INVALID_TASK, 1, 0, 0, 0, 0, 0, 0, 0,}, // 0x0A  // REG_STAT_0_ADDR
    { INVALID_TASK, 1, 0, 0, 0, 0, 0, 0, 0,}, // 0x0B  // REG_BAT_STAT_0_ADDR
    { INVALID_TASK, 1, 0, 0, 0, 0, 0, 0, 0,}, // 0x0C  // REG_IBAT_ADDR[0]
    { INVALID_TASK, 1, 0, 0, 0, 0, 0, 0, 0,}, // 0x0D  // REG_IBAT_ADDR[1]
    { INVALID_TASK, 0, 0, 0, 0, 0, 0, 0, 0,}, // 0x0E
    { INVALID_TASK, 0, 0, 0, 0, 0, 0, 0, 0,}, // 0x0F
    { INVALID_TASK, 0, 0, 0, 0, 0, 0, 0, 0,}, // 0x10 // Reserved for testing
    { INVALID_TASK, 0, 0, 1, 0, 0, 0, 0, 0,}, // 0x11 // FW major version
    { INVALID_TASK, 0, 0, 1, 0, 0, 0, 0, 0,}, // 0x12 // FW minor version
    { INVALID_TASK, 0, 0, 1, 0, 0, 0, 0, 0,}, // 0x13 // FW patch version
};

static volatile uint8_t i2c_shadow_map[I2C_CLIENT_LOCATION_SIZE] = {};

// clang-format on
