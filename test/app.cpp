

#include "i2c_app.h"
#include "tasks.h"
#include "timers.h"

#include <mcc_generated_files/system/system.h>

void test_main(void) {
  // no need for system initializ here in the test context
  timers_intialize();
  i2c_app_initialize();
  // If using interrupts in PIC18 High/Low Priority Mode you need to enable the
  // Global High and Low Interrupts If using interrupts in PIC Mid-Range
  // Compatibility Mode you need to enable the Global Interrupts Use the
  // following macros to:

  // Enable the Global Interrupts
  INTERRUPT_GlobalInterruptHighEnable();
  INTERRUPT_GlobalInterruptLowEnable();
  task_main();
}