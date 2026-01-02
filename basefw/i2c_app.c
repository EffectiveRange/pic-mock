
#include "i2c_app.h"
#include "tasks.h"
#include "xc.h"

#include <mcc_generated_files/system/system.h>

#include <mcc_generated_files/i2c_client/i2c1.h>
#include <mcc_generated_files/i2c_client/i2c_client_interface.h>

struct i2c_reg_descr {
  uint8_t listener_task;
  uint8_t clear_on_read : 1;
  uint8_t written_by_isr : 1;
  uint8_t immutable : 1;
  uint8_t reserved : 5;
  uint8_t clear_on_read_mask;
  volatile uint8_t dirty_isr;
  uint8_t dirty_main;
  uint8_t invoke_task;
};

#include "i2c_regs.c"

// Private functions
//
// Private variable
// NOTES:
// BYTE 1 MSB is sticky bit

static struct i2c_state {
  uint8_t write_cnt_main;
  uint8_t txn_error_cnt_main;
  ///////////////////////////
  volatile uint8_t client_location;
  volatile uint8_t txn_error_cnt_isr;
  volatile bool is_client_location;
  volatile bool txn_error;
  volatile uint8_t write_cnt_isr;
} _i2c_state = {};

bool __reentrant isr_i2c_client_application(i2c_client_transfer_event_t event) {
  switch (event) {
  case I2C_CLIENT_TRANSFER_EVENT_ADDR_MATCH: // Address Match Event
    _i2c_state.txn_error = false;
    if (I2C1_Client.TransferDirGet() == I2C_CLIENT_TRANSFER_DIR_WRITE) {
      _i2c_state.is_client_location = true;
    }
    break;

  case I2C_CLIENT_TRANSFER_EVENT_RX_READY: // Read the data sent by I2C Host
  {
    uint8_t curr_byte = I2C1_Client.ReadByte();
    if (_i2c_state.is_client_location) {
      _i2c_state.client_location =
          curr_byte < I2C_CLIENT_LOCATION_SIZE ? curr_byte : 0;
      _i2c_state.is_client_location = false;
    } else {
      struct i2c_reg_descr *curr_descr =
          &i2c_reg_defs[_i2c_state.client_location];
      if (curr_descr->written_by_isr == 1 && !_i2c_state.txn_error) {
        curr_descr->dirty_isr = 1;
        _i2c_state.write_cnt_isr++;
        i2c_shadow_map[_i2c_state.client_location] = curr_byte;
      }
      _i2c_state.client_location =
          _i2c_state.client_location < I2C_CLIENT_LOCATION_SIZE - 1
              ? _i2c_state.client_location + 1
              : 0;
    }
    break;
  }

  case I2C_CLIENT_TRANSFER_EVENT_TX_READY:
    // Provide the Client data requested by the I2C Host
    {
      uint8_t curr_byte = i2c_shadow_map[_i2c_state.client_location];
      struct i2c_reg_descr *curr_descr =
          &i2c_reg_defs[_i2c_state.client_location];
      if (curr_descr->clear_on_read) {
        uint8_t new_val = curr_byte & curr_descr->clear_on_read_mask;
        if (new_val != curr_byte) {
          // update only the shadow map so that
          // there is no back-sync to the main map
          i2c_shadow_map[_i2c_state.client_location] = new_val;
          if (curr_descr->written_by_isr) {
            curr_descr->dirty_isr = 1;
            _i2c_state.write_cnt_isr++;
          }
        }
      }
      _i2c_state.client_location =
          _i2c_state.client_location < I2C_CLIENT_LOCATION_SIZE - 1
              ? _i2c_state.client_location + 1
              : 0;
      I2C1_Client.WriteByte(curr_byte);
      break;
    }

  case I2C_CLIENT_TRANSFER_EVENT_STOP_BIT_RECEIVED: // Stop Communication
    _i2c_state.client_location = 0x00;
    if (_i2c_state.txn_error) {
      _i2c_state.txn_error_cnt_isr++;
    }
    _i2c_state.txn_error = false;
    _i2c_state.is_client_location = false;
    task_schedule_from_irq(I2C_TASK);
    break;

  case I2C_CLIENT_TRANSFER_EVENT_ERROR: // Error Event Handler
    _i2c_state.txn_error = true;
    _i2c_state.txn_error_cnt_isr++;
    _i2c_state.client_location = 0x00;
    _i2c_state.is_client_location = false;
    i2c_client_error_t errorState = I2C1_Client.ErrorGet();
    i2c_shadow_map[REG_STAT_I2C_ERR_AND_STICKY_ADDR] |= errorState;
    i2c_reg_defs[REG_STAT_I2C_ERR_AND_STICKY_ADDR].dirty_isr = 1;
    if (errorState == I2C_CLIENT_ERROR_BUS_COLLISION) {
      i2c_shadow_map[REG_STAT_I2C_ERR_BUS_COLLISION_CNTR]++;
      i2c_reg_defs[REG_STAT_I2C_ERR_BUS_COLLISION_CNTR].dirty_isr = 1;
    } else if (errorState == I2C_CLIENT_ERROR_WRITE_COLLISION) {
      i2c_shadow_map[REG_STAT_I2C_ERR_WRITE_COLLISION_CNTR]++;
      i2c_reg_defs[REG_STAT_I2C_ERR_WRITE_COLLISION_CNTR].dirty_isr = 1;
    } else if (errorState == I2C_CLIENT_ERROR_RECEIVE_OVERFLOW) {
      i2c_shadow_map[REG_STAT_I2C_ERR_RECEIVE_OVERFLOW_CNTR]++;
      i2c_reg_defs[REG_STAT_I2C_ERR_RECEIVE_OVERFLOW_CNTR].dirty_isr = 1;
    } else if (errorState == I2C_CLIENT_ERROR_TRANSMIT_UNDERFLOW) {
      i2c_shadow_map[REG_STAT_I2C_ERR_TRANSMIT_UNDERFLOW_CNTR]++;
      i2c_reg_defs[REG_STAT_I2C_ERR_TRANSMIT_UNDERFLOW_CNTR].dirty_isr = 1;
    } else if (errorState == I2C_CLIENT_ERROR_READ_UNDERFLOW) {
      i2c_shadow_map[REG_STAT_I2C_ERR_READ_UNDERFLOW_CNTR]++;
      i2c_reg_defs[REG_STAT_I2C_ERR_READ_UNDERFLOW_CNTR].dirty_isr = 1;
    } else {
      i2c_shadow_map[REG_STAT_I2C_ERR_UNKNOWN_CNTR]++;
      i2c_reg_defs[REG_STAT_I2C_ERR_UNKNOWN_CNTR].dirty_isr = 1;
    }
    task_schedule_from_irq(I2C_TASK);

    break;

  default:
    break;
  }
  return true;
}

// called exclusively from the main thread

static void reconcile_i2c_writes() {
  for (int i = 0; i < I2C_CLIENT_LOCATION_SIZE; i++) {
    uint8_t dirty = i2c_reg_defs[i].dirty_isr;
    if (i2c_reg_defs[i].immutable == 0 && !i2c_reg_defs[i].written_by_isr) {
      i2c_shadow_map[i] = i2c_reg_map[i];
    } else if (i2c_reg_defs[i].written_by_isr && dirty == 1) {
      i2c_reg_map[i] = i2c_shadow_map[i];
      // invoke registered task
      i2c_reg_defs[i].invoke_task = 1;
    }
    if (dirty) {
      i2c_reg_defs[i].dirty_isr = 0;
    }
    i2c_reg_defs[i].dirty_main = 0;
  }
}

// called only from main thread

static void notify_write_listeners(bool writes_happened) {
  if (writes_happened) {
    for (int i = 0; i < I2C_CLIENT_LOCATION_SIZE; i++) {
      if (i2c_reg_defs[i].invoke_task == 1) {
        i2c_reg_defs[i].invoke_task = 0;
        if (i2c_reg_defs[i].listener_task != INVALID_TASK) {
          task_schedule(i2c_reg_defs[i].listener_task);
        }
      }
    }
  }
}

// called only from main thread

bool i2c_app_main() {
  bool writes_happened = false;
  uint8_t isr_wr_cnt = _i2c_state.write_cnt_isr;
  uint8_t txn_error_cnt = _i2c_state.txn_error_cnt_isr;
  bool error_happened = _i2c_state.txn_error_cnt_main != txn_error_cnt;
  if (isr_wr_cnt != _i2c_state.write_cnt_main || error_happened) {
    reconcile_i2c_writes();
    writes_happened = true;
  }
  _i2c_state.write_cnt_main = isr_wr_cnt;
  _i2c_state.txn_error_cnt_main = txn_error_cnt;
  notify_write_listeners(writes_happened);
  return false;
}

// called only from main thread

void __reentrant i2c_client_write_register_byte(uint8_t address, uint8_t data) {
  if (i2c_reg_defs[address].immutable || i2c_reg_defs[address].written_by_isr) {
    return;
  }
  i2c_reg_defs[address].dirty_main = 1;
  i2c_reg_map[address] = data;
  _i2c_state.write_cnt_main--;
  task_schedule(I2C_TASK);
}

// called only from main thread

void __reentrant i2c_client_write_register_bit(uint8_t address, uint8_t pos,
                                               uint8_t val) {
  if (i2c_reg_defs[address].immutable || i2c_reg_defs[address].written_by_isr) {
    return;
  }
  i2c_reg_defs[address].dirty_main = 1;
  if (val) {
    i2c_reg_map[address] |= (1 << pos);
  } else {
    i2c_reg_map[address] &= ~(1 << pos);
  }
  _i2c_state.write_cnt_main--;
  task_schedule(I2C_TASK);
}

void i2c_app_initialize() {
  for (int i = 0; i < I2C_CLIENT_LOCATION_SIZE; i++) {
    i2c_shadow_map[i] = i2c_reg_map[i];
  }
  I2C1_Client.CallbackRegister(isr_i2c_client_application);
  task_init(I2C_TASK, i2c_app_main, false);
}
