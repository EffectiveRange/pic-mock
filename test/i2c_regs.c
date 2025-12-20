#include "i2c_regs.h"
#include "i2c_regs_data.h"
#include "version.h"

#include <stddef.h>

// clang-format off
 static uint8_t main_i2c_reg_map[I2C_CLIENT_LOCATION_SIZE] = {
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
 // TODO: add from preprocessor
 FW_MRCM_MAJOR, // 0x11 // FW major version
 FW_MRCM_MINOR, // 0x12 // FW minor version
 FW_MRCM_PATCH // 0x13 // FW patch version
};

struct i2c_reg_descr i2c_reg_defs[I2C_CLIENT_LOCATION_SIZE]={
  //task, w, cor,  wi, imm,  rsv, cor_mask, di, dm, it,
  { NULL, 0, 0,    0,  1,    0,   0,        0,  0,  0, },    // 0x00  // always 0
  { NULL, 0, 1,    1,  0,    0,   0x80,     0,  0,  0, }, // 0x01  // REG_STAT_I2C_ERR_AND_STICKY_ADDR
  { NULL, 0, 1,    1,  0,    0,   0,        0,  0,  0, },    // 0x02  // REG_STAT_I2C_ERR_BUS_COLLISION_CNTR
  { NULL, 0, 1,    1,  0,    0,   0,        0,  0,  0, },    // 0x03  // REG_STAT_I2C_ERR_WRITE_COLLISION_CNTR
  { NULL, 0, 1,    1,  0,    0,   0,        0,  0,  0, },    // 0x04  // REG_STAT_I2C_ERR_RECEIVE_OVERFLOW_CNTR
  { NULL, 0, 1,    1,  0,    0,   0,        0,  0,  0, },    // 0x05  // REG_STAT_I2C_ERR_TRANSMIT_UNDERFLOW_CNTR
  { NULL, 0, 1,    1,  0,    0,   0,        0,  0,  0, },    // 0x06  // REG_STAT_I2C_ERR_READ_UNDERFLOW_CNTR
  { NULL, 0, 1,    1,  0,    0,   0,        0,  0,  0, },    // 0x07  // REG_STAT_I2C_ERR_UNKNOWN_CNTR
  { NULL, 1, 0,    0,  0,    0,   0,        0,  0,  0, },    // 0x08  // REG_CMD_HALT_ACTION
  { NULL, 0, 0,    0,  0,    0,   0,        0,  0,  0, },    // 0x09  // UNUSED
  { NULL, 0, 1,    0,  0,    0,   0,        0,  0,  0, },    // 0x0A  // REG_STAT_0_ADDR
  { NULL, 0, 1,    0,  0,    0,   0,        0,  0,  0, },    // 0x0B  // REG_BAT_STAT_0_ADDR
  { NULL, 0, 1,    0,  0,    0,   0,        0,  0,  0, },    // 0x0C  // REG_IBAT_ADDR[0]
  { NULL, 0, 1,    0,  0,    0,   0,        0,  0,  0, },    // 0x0D  // REG_IBAT_ADDR[1]
  { NULL, 1, 0,    0,  0,    0,   0,        0,  0,  0, },    // 0x0E
  { NULL, 1, 0,    0,  0,    0,   0,        0,  0,  0, },    // 0x0F
  { NULL, 1, 0,    0,  0,    0,   0,        0,  0,  0, },    // 0x10 // Reserved for testing
  { NULL, 0, 0,    0,  1,    0,   0,        0,  0,  0, },    // 0x11 // FW major version
  { NULL, 0, 0,    0,  1,    0,   0,        0,  0,  0, },    // 0x12 // FW minor version
  { NULL, 0, 0,    0,  1,    0,   0,        0,  0,  0, },    // 0x13 // FW patch version
};

static uint8_t shadow_i2c_reg_map[I2C_CLIENT_LOCATION_SIZE] = {};
static uint8_t shadow2_i2c_reg_map[I2C_CLIENT_LOCATION_SIZE] = {};


volatile uint8_t *i2c_reg_map = main_i2c_reg_map;
volatile uint8_t *i2c_shadow_map = shadow_i2c_reg_map;
// clang-format on
