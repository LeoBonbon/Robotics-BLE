/**
 ****************************************************************************************
 *
 * @file user_periph_setup.c
 *
 * @brief Peripherals setup and initialization.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"             // SW configuration
#include "user_periph_setup.h"       // peripheral configuration
#include "global_io.h"
#include "gpio.h"
#include "uart.h"                    // UART initialization
#include "pwm.h"




/**
 ****************************************************************************************
 * @brief Each application reserves its own GPIOs here.
 *
 * @return void
 ****************************************************************************************
 */

 // Define 4 physically exposed pins on the HJ-580XP module
#define PORT_A GPIO_PORT_0
#define PIN_A  GPIO_PIN_4    // Physical pad P04

#define PORT_B GPIO_PORT_0
#define PIN_B  GPIO_PIN_5    // Physical pad P05

#define PORT_C GPIO_PORT_0
#define PIN_C  GPIO_PIN_6    // Physical pad P06

#define PORT_D GPIO_PORT_1
#define PIN_D  GPIO_PIN_3    // Physical pad P13

// Track which port and pin is currently active
GPIO_PORT active_port = PORT_A;
GPIO_PIN  active_pin  = PIN_A;



#ifdef CFG_DEVELOPMENT_DEBUG

void GPIO_reservations(void)
{
/*
* Globally reserved GPIOs reservation
*/

/*
* Application specific GPIOs reservation. Used only in Development mode (#if DEVELOPMENT_DEBUG)

i.e.
    RESERVE_GPIO(DESCRIPTIVE_NAME, GPIO_PORT_0, GPIO_PIN_1, PID_GPIO);    //Reserve P_01 as Generic Purpose I/O
*/
#ifdef CFG_PRINTF_UART2
    RESERVE_GPIO(UART2_TX, UART2_TX_GPIO_PORT, UART2_TX_GPIO_PIN, PID_UART2_TX);
    RESERVE_GPIO(UART2_RX, UART2_RX_GPIO_PORT, UART2_RX_GPIO_PIN, PID_UART2_RX);
#endif
   // Reserve all 4 physical pins
    RESERVE_GPIO(STIM_A, PORT_A, PIN_A, PID_GPIO);
    RESERVE_GPIO(STIM_B, PORT_B, PIN_B, PID_GPIO);
    RESERVE_GPIO(STIM_C, PORT_C, PIN_C, PID_GPIO);
    RESERVE_GPIO(STIM_D, PORT_D, PIN_D, PID_GPIO);
}
#endif // CFG_DEVELOPMENT_DEBUG

void set_pad_functions(void)        // set gpio port function mode
{
#ifdef CFG_PRINTF_UART2
    GPIO_ConfigurePin(UART2_TX_GPIO_PORT, UART2_TX_GPIO_PIN, OUTPUT, PID_UART2_TX, false);
    GPIO_ConfigurePin(UART2_RX_GPIO_PORT, UART2_RX_GPIO_PIN, INPUT, PID_UART2_RX, false);
#endif
     // BOOT REQUIREMENT: Initialize all 4 physical pins as plain GPIO held LOW (false)
    GPIO_ConfigurePin(PORT_A, PIN_A, OUTPUT, PID_GPIO, false);
    GPIO_ConfigurePin(PORT_B, PIN_B, OUTPUT, PID_GPIO, false);
    GPIO_ConfigurePin(PORT_C, PIN_C, OUTPUT, PID_GPIO, false);
    GPIO_ConfigurePin(PORT_D, PIN_D, OUTPUT, PID_GPIO, false);
}
void periph_init(void)
{
    // Power up peripherals' power domain
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
    while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP));

    SetBits16(CLK_16M_REG, XTAL16_BIAS_SH_ENABLE, 1);

    //rom patch
    patch_func();

    //Init pads
    set_pad_functions();

    // (Re)Initialize peripherals
    // i.e.
    //  uart_init(UART_BAUDRATE_115K2, 3);

#ifdef CFG_PRINTF_UART2
    SetBits16(CLK_PER_REG, UART2_ENABLE, 1);
    uart2_init(UART_BAUDRATE_115K2, 3);
#endif

   // Enable the pads
    SetBits16(SYS_CTRL_REG, PAD_LATCH_EN, 1);
}

// TURN ON FUNCTION
void pwm_output_ON(uint8_t pin_no, uint16_t freq, uint8_t duty) {
    // 1. Map the Bluetooth byte (0-3) to the physical port and pin
    switch(pin_no) {
        case 0: active_port = PORT_A; active_pin = PIN_A; break;
        case 1: active_port = PORT_B; active_pin = PIN_B; break;
        case 2: active_port = PORT_C; active_pin = PIN_C; break;
        case 3: active_port = PORT_D; active_pin = PIN_D; break;
        default: return; // Exit if an invalid pin is requested
    }

    // 2. Dynamically route the PWM hardware to the requested pin
    GPIO_ConfigurePin(active_port, active_pin, OUTPUT, PID_PWM0, false);

    // 3. Calculate timer ticks
    uint32_t total_ticks = 2000000 / freq;
    uint16_t high_ticks = (total_ticks * duty) / 100;
    uint16_t low_ticks = total_ticks - high_ticks;
    
    if (high_ticks == 0) high_ticks = 1;
    if (low_ticks == 0) low_ticks = 1;

    // 4. Load the timer and start the PWM output
    timer0_set_pwm_high_counter(high_ticks);
    timer0_set_pwm_low_counter(low_ticks);
    
    SetBits16(CLK_PER_REG, TMR_ENABLE, 1);
    
    set_tmr_div(CLK_PER_REG_TMR_DIV_8); 
    timer0_init(TIM0_CLK_FAST, PWM_MODE_ONE, TIM0_CLK_NO_DIV);
    
  
    
    timer0_start();
}
// TURN OFF FUNCTION
void pwm_output_OFF(void) {
    // 1. Stop the hardware timer
    timer0_stop();
    SetBits16(CLK_PER_REG, TMR_ENABLE, 0);

    // 2. Revert the active pin back to a standard GPIO and pull it LOW
    GPIO_ConfigurePin(active_port, active_pin, OUTPUT, PID_GPIO, false);
}