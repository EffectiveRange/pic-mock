
#include "i2c_app.h"
#include "i2c_regs_data.h"
#include "main.h"
#include "mcc_generated_files/system/system.h"
#include "modules.h"
#include "tasks.h"
#include "timers.h"
#include "xc.h"

void I2C1_Close(void);
void I2C1_BusReset(void);

// Private functions
//
// Private variable
// NOTES:
// BYTE 1 MSB is sticky bit
static struct i2c_state {
  uint16_t write_cnt_main;
  uint8_t txn_error_cnt_main;
  ///////////////////////////
  volatile uint8_t client_location;
  volatile uint8_t txn_error_cnt_isr;
  volatile bool is_client_location;
  volatile bool txn_error;
  volatile uint16_t write_cnt_isr;
} _i2c_state = {};

static void i2c_app_main(struct task_descr_t *task);

static struct task_descr_t i2c_app_task = {
    .task_fn = i2c_app_main,
    .task_state = &_i2c_state,
    .suspended = true,
    .next = NULL,
};

bool isr_i2c_client_application(i2c_client_transfer_event_t event) {
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
      _i2c_state.client_location = curr_byte;
      _i2c_state.is_client_location = false;
    } else {
      struct i2c_reg_descr *curr_descr =
          &i2c_reg_defs[_i2c_state.client_location];
      if (curr_descr->writable == 1 && !_i2c_state.txn_error) {
        curr_descr->dirty_isr = 1;
        ISR_SAFE_BEGIN();
        _i2c_state.write_cnt_isr++;
        ISR_SAFE_END();
        i2c_shadow_map[_i2c_state.client_location] = curr_byte;
      }
      _i2c_state.client_location++;
    }
    break;
  }

  case I2C_CLIENT_TRANSFER_EVENT_TX_READY: // Provide the Client data requested
                                           // by the I2C Host
  {
    uint8_t curr_byte = i2c_shadow_map[_i2c_state.client_location];
    struct i2c_reg_descr *curr_descr =
        &i2c_reg_defs[_i2c_state.client_location];
    if (curr_descr->clear_on_read) {
      uint8_t new_val = curr_byte & curr_descr->clear_on_read_mask;
      if (new_val != curr_byte) {
        i2c_shadow_map[_i2c_state.client_location] = new_val;
        ISR_SAFE_BEGIN();
        curr_descr->dirty_isr = 1;
        _i2c_state.write_cnt_isr++;
        ISR_SAFE_END();
      }
    }
    _i2c_state.client_location++;
    I2C1_Client.WriteByte(curr_byte);
    break;
  }

  case I2C_CLIENT_TRANSFER_EVENT_STOP_BIT_RECEIVED: // Stop Communication
    _i2c_state.client_location = 0x00;
    if (_i2c_state.txn_error) {
      ISR_SAFE_BEGIN();
      _i2c_state.txn_error_cnt_isr++;
      ISR_SAFE_END();
    }
    _i2c_state.txn_error = false;
    _i2c_state.is_client_location = false;
    schedule_task_from_irq(&i2c_app_task);
    break;

  case I2C_CLIENT_TRANSFER_EVENT_ERROR: // Error Event Handler
    _i2c_state.txn_error = true;
    ISR_SAFE_BEGIN();
    _i2c_state.txn_error_cnt_isr++;
    ISR_SAFE_END();
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
    schedule_task_from_irq(&i2c_app_task);
    break;

  default:
    break;
  }
  return true;
}

void i2c_switch_mode(enum I2C1_Mode new_mode) {
  if (new_mode == I2C1_Current_Mode()) {
    return;
  }
  I2C1_Switch_Mode(new_mode);
  if (new_mode == I2C1_HOST_MODE) {
    // I2C1_Host_ReadyCallbackRegister(I2CSuccess);
    // I2C1_Host_CallbackRegister(I2CError);
    //        I2C_SEL_N_SetLow(); //disable PI from I2C bus
  } else if (new_mode == I2C1_CLIENT_MODE) {
    I2C1_Client.CallbackRegister(isr_i2c_client_application);
  }
}

void i2c_app_initialize(void) {
  for (int i = 0; i < I2C_CLIENT_LOCATION_SIZE; i++) {
    i2c_shadow_map[i] = i2c_reg_map[i];
  }
  add_task(&i2c_app_task);
  I2C1_Multi_Initialize();
  I2C1_Client.CallbackRegister(isr_i2c_client_application);
}

// called excluseively from the main thread
// with interrupts disabled
static void reconcile_i2c_writes(bool txn_error) {
  struct i2c_reg_descr *curr_descr = &i2c_reg_defs[0];
  for (int i = 0; i < I2C_CLIENT_LOCATION_SIZE; i++, curr_descr++) {
    // if any error happened, then always prefer the main register
    // map content
    if (txn_error && curr_descr->written_by_isr != 1) {
      i2c_shadow_map[i] = i2c_reg_map[i];
    } else if (curr_descr->writable == 1) {
      // prefer writes from host
      if (curr_descr->dirty_isr == 1) {
        i2c_reg_map[i] = i2c_shadow_map[i];
        // invoke registered task
        curr_descr->invoke_task = 1;
      } else {
        i2c_shadow_map[i] = i2c_reg_map[i];
      }
    } else if (curr_descr->written_by_isr && curr_descr->dirty_isr == 1) {
      i2c_reg_map[i] = i2c_shadow_map[i];
      // invoke registered task
      curr_descr->invoke_task = 1;
    } else if (curr_descr->immutable != 1 && curr_descr->dirty_main == 1) {
      i2c_shadow_map[i] = i2c_reg_map[i];
    }
    curr_descr->dirty_isr = 0;
    curr_descr->dirty_main = 0;
  }
}

static void notify_write_listeners(bool writes_happened) {
  if (writes_happened) {
    struct i2c_reg_descr *curr_descr = &i2c_reg_defs[0];
    for (int i = 0; i < I2C_CLIENT_LOCATION_SIZE; i++, ++curr_descr) {
      if (curr_descr->invoke_task == 1) {
        curr_descr->invoke_task = 0;
        for (struct i2c_write_listener_t *curr_task = curr_descr->task;
             curr_task != NULL; curr_task = curr_task->next) {
          curr_task->task->suspended = false;
        }
      }
    }
  }
}

static void i2c_app_main(struct task_descr_t *task) {
  bool writes_happened = false;
  bool error_happened = false;
  INTERRUPT_GlobalInterruptLowDisable();
  INTERRUPT_GlobalInterruptHighDisable();
  MAIN_THREAD_EXCLUSIVE_BEGIN();
  uint16_t isr_wr_cnt = _i2c_state.write_cnt_isr;
  uint16_t txn_error_cnt = _i2c_state.txn_error_cnt_isr;
  error_happened = _i2c_state.txn_error_cnt_main != txn_error_cnt;
  if (isr_wr_cnt != _i2c_state.write_cnt_main || error_happened) {
    reconcile_i2c_writes(error_happened);
    writes_happened = true;
  }
  _i2c_state.write_cnt_main = isr_wr_cnt;
  _i2c_state.txn_error_cnt_main = txn_error_cnt;
  MAIN_THREAD_EXCLUSIVE_END();
  INTERRUPT_GlobalInterruptHighEnable();
  INTERRUPT_GlobalInterruptLowEnable();
  i2c_app_task.suspended = true;
  notify_write_listeners(writes_happened);
}

void i2c_app_deinitialize(void) { remove_task(&i2c_app_task); }

void i2c_register_write_listener(uint8_t address,
                                 struct i2c_write_listener_t *task) {
  if (address >= I2C_CLIENT_LOCATION_SIZE) {
    return;
  }
  struct i2c_reg_descr *curr_descr = &i2c_reg_defs[address];
  task->next = curr_descr->task;
  curr_descr->task = task;
}

void i2c_deregister_write_listener(uint8_t address,
                                   struct i2c_write_listener_t *task) {

  // TODO: implement
}

void i2c_client_write_register_byte(uint8_t address, uint8_t data) {
  struct i2c_reg_descr *curr_descr = &i2c_reg_defs[address];
  curr_descr->dirty_main = 1;
  i2c_reg_map[address] = data;
  _i2c_state.write_cnt_main--;
  i2c_app_task.suspended = false;
}

void i2c_client_write_register_bit(uint8_t address, uint8_t pos, uint8_t val) {
  struct i2c_reg_descr *curr_descr = &i2c_reg_defs[address];
  curr_descr->dirty_main = 1;
  if (val) {
    i2c_reg_map[address] |= (1 << pos);
  } else {
    i2c_reg_map[address] &= ~(1 << pos);
  }
  _i2c_state.write_cnt_main--;
  i2c_app_task.suspended = false;
}

static struct module_t _module = {
    .init = i2c_app_initialize,
    .deinit = i2c_app_deinitialize,
    .register_events = NULL,
    .deregister_events = NULL,
    .next = NULL,
};
void register_i2c_app_module() { add_module(&_module); }
