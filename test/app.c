


#include "modules.h"
#include "tasks.h"
#include "timers.h"
#include "i2c_regs.h"
#include "i2c_app.h"

#include <mcc_generated_files/i2c_client/i2c1.h>

void init_application(void) {
  register_i2c_app_module(I2C_CLIENT_LOCATION_SIZE,I2C1_Client_Initialize);
  register_timer_module();
  register_tasks_module();
  initialize_modules();
}