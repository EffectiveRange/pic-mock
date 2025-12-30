/*
 * MAIN Generated Driver File
 *
 * @file main.c
 *
 * @defgroup main MAIN
 *
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.0
 */

/*
� [2024] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip
    software and any derivatives exclusively with Microchip products.
    You are responsible for complying with 3rd party license terms
    applicable to your use of 3rd party software (including open source
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.?
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR
    THIS SOFTWARE.
 */
#include "mcc_generated_files/system/system.h"

#include "main.h"
#include "tasks.h"
#include "timers.h"

#include "app.c"
#include "bq_handler.c"
#include "i2c_regs.c"
#include "led_controller.c"
#include "i2c_app.c"
#include "modules.c"
#include "tasks.c"
#include "timers.c"


/*
    Main application
 */

/*

#define PWM_LED_PERIOD_CNT 256U
#define PWM_LED_CHARGING_TIME_MS 10

static bool test_init = false;
uint8_t dcount = 0;
static uint16_t charging_duty_cycles[65] = { 
    PWM_LED_PERIOD_CNT / 256,
    PWM_LED_PERIOD_CNT / 256,
    PWM_LED_PERIOD_CNT / 128, 
    PWM_LED_PERIOD_CNT / 128, 
    PWM_LED_PERIOD_CNT / 128, 
    PWM_LED_PERIOD_CNT / 64,  
    PWM_LED_PERIOD_CNT / 64,     
    PWM_LED_PERIOD_CNT / 64,  
    PWM_LED_PERIOD_CNT / 64,  
    PWM_LED_PERIOD_CNT / 32, 
    PWM_LED_PERIOD_CNT / 32, 
    PWM_LED_PERIOD_CNT / 32, 
    PWM_LED_PERIOD_CNT / 32, 
    PWM_LED_PERIOD_CNT / 16,    
    PWM_LED_PERIOD_CNT / 16, 
    PWM_LED_PERIOD_CNT / 16, 
    PWM_LED_PERIOD_CNT / 16,
    PWM_LED_PERIOD_CNT / 16,    
    PWM_LED_PERIOD_CNT / 16, 
    PWM_LED_PERIOD_CNT / 16, 
    PWM_LED_PERIOD_CNT / 16, 
    PWM_LED_PERIOD_CNT / 8,
    PWM_LED_PERIOD_CNT / 8, 
    PWM_LED_PERIOD_CNT / 8,
    PWM_LED_PERIOD_CNT / 8,    
    PWM_LED_PERIOD_CNT / 4,
    PWM_LED_PERIOD_CNT / 4,    
    PWM_LED_PERIOD_CNT / 2,
    PWM_LED_PERIOD_CNT / 2,
    PWM_LED_PERIOD_CNT / 2,
    PWM_LED_PERIOD_CNT,
    PWM_LED_PERIOD_CNT,
    PWM_LED_PERIOD_CNT / 2,    
    PWM_LED_PERIOD_CNT / 4,
    PWM_LED_PERIOD_CNT / 4,
    PWM_LED_PERIOD_CNT / 8,
    PWM_LED_PERIOD_CNT / 8, 
    PWM_LED_PERIOD_CNT / 8,
    PWM_LED_PERIOD_CNT / 8,      
    PWM_LED_PERIOD_CNT / 16,    
    PWM_LED_PERIOD_CNT / 16, 
    PWM_LED_PERIOD_CNT / 16, 
    PWM_LED_PERIOD_CNT / 16,
    PWM_LED_PERIOD_CNT / 16,    
    PWM_LED_PERIOD_CNT / 16, 
    PWM_LED_PERIOD_CNT / 16, 
    PWM_LED_PERIOD_CNT / 16,
    PWM_LED_PERIOD_CNT / 16,   
    PWM_LED_PERIOD_CNT / 32, 
    PWM_LED_PERIOD_CNT / 32, 
    PWM_LED_PERIOD_CNT / 32, 
    PWM_LED_PERIOD_CNT / 32, 
    PWM_LED_PERIOD_CNT / 64,  
    PWM_LED_PERIOD_CNT / 64,     
    PWM_LED_PERIOD_CNT / 64,  
    PWM_LED_PERIOD_CNT / 64,  
    PWM_LED_PERIOD_CNT / 128, 
    PWM_LED_PERIOD_CNT / 128, 
    PWM_LED_PERIOD_CNT / 128, 
    PWM_LED_PERIOD_CNT / 256,
    PWM_LED_PERIOD_CNT / 256,  
    PWM_LED_PERIOD_CNT / 256,
    PWM_LED_PERIOD_CNT / 256,    
    PWM_LED_PERIOD_CNT / 256,
    PWM_LED_PERIOD_CNT / 256, 
};


tw_timer_ptr_t change_duty(tw_timer_ptr_t t){
    uint16_t idx = dcount/2;
    uint16_t duty = (charging_duty_cycles[idx] + charging_duty_cycles[idx+1])/2;
    PWR_LED_PWM_SetSlice1Output1DutyCycleRegister(duty);
    PWR_LED_PWM_SetSlice1Output2DutyCycleRegister(duty);
    PWR_LED_PWM_LoadBufferRegisters();
    if(dcount >=127){
        dcount=0;
    }else{
        dcount = dcount +1;
    }
    return t;
}

struct tw_timer_t test_timer=
{.callback=change_duty,.time_ms=PWM_LED_CHARGING_TIME_MS};

void test_task(task_descr_ptr_t t){
    if(!test_init){
        PWR_LED_PWM_Enable();
        PWR_LED_PWM_WritePeriodRegister(PWM_LED_PERIOD_CNT-1);
        PWR_LED_PWM_SetSlice1Output1DutyCycleRegister(0);
        PWR_LED_PWM_SetSlice1Output2DutyCycleRegister(0);
        PWR_LED_PWM_LoadBufferRegisters();
        add_timer(&test_timer);
        test_init = true;
    }
    t->suspended = true;
}

struct task_descr_t test={
  .task_fn = test_task
};

*/

int main() {
  SYSTEM_Initialize();

  init_application();
//  add_task(&test);
  // If using interrupts in PIC18 High/Low Priority Mode you need to enable the
  // Global High and Low Interrupts If using interrupts in PIC Mid-Range
  // Compatibility Mode you need to enable the Global Interrupts Use the
  // following macros to:

  // Enable the Global Interrupts
  INTERRUPT_GlobalInterruptHighEnable();
  INTERRUPT_GlobalInterruptLowEnable();

  run_tasks();
  return 0;
}

