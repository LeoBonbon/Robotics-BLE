/**
 ****************************************************************************************
 *
 * @file user_periph_setup.c
 *
 * @brief Peripherals setup and initialization.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work.
 *
 ****************************************************************************************
 */
#include <stdbool.h>                 // Standard boolean types (true/false)
#include "rwip_config.h"             // SW configuration
#include "user_periph_setup.h"       // Peripheral configuration
#include "global_io.h"
#include "gpio.h"
#include "uart.h"                    // UART initialization
#include "pwm.h"
#include "app_easy_timer.h"          //Required for Software PWM (< 31 Hz)

// Global variables for Software PWM State Machine
timer_hnd sw_pwm_timer_id = EASY_TIMER_INVALID_TIMER;
uint8_t   sw_pwm_pin = 0;
uint16_t  sw_pwm_high_ticks = 0;
uint16_t  sw_pwm_low_ticks = 0;
bool      sw_pwm_is_high = false;

// Define 4 physically exposed pins on the HJ-580XP module
#define PORT_A GPIO_PORT_0
#define PIN_A  GPIO_PIN_4    // Physical pad P04

#define PORT_B GPIO_PORT_0
#define PIN_B  GPIO_PIN_5    // Physical pad P05

#define PORT_C GPIO_PORT_0
#define PIN_C  GPIO_PIN_6    // Physical pad P06

#define PORT_D GPIO_PORT_1
#define PIN_D  GPIO_PIN_3    // Physical pad P13

// Define the Status LED pin (Physical pad P11)
#define PORT_LED GPIO_PORT_1
#define PIN_LED  GPIO_PIN_1

#ifdef CFG_DEVELOPMENT_DEBUG
void GPIO_reservations(void)
{
#ifdef CFG_PRINTF_UART2
    RESERVE_GPIO(UART2_TX, UART2_TX_GPIO_PORT, UART2_TX_GPIO_PIN, PID_UART2_TX);
    RESERVE_GPIO(UART2_RX, UART2_RX_GPIO_PORT, UART2_RX_GPIO_PIN, PID_UART2_RX);
#endif
    // Reserve stimulation pads and status LED
    RESERVE_GPIO(STIM_A, PORT_A, PIN_A, PID_GPIO);
    RESERVE_GPIO(STIM_B, PORT_B, PIN_B, PID_GPIO);
    RESERVE_GPIO(STIM_C, PORT_C, PIN_C, PID_GPIO);
    RESERVE_GPIO(STIM_D, PORT_D, PIN_D, PID_GPIO);
    RESERVE_GPIO(STATUS_LED, PORT_LED, PIN_LED, PID_GPIO);
}
#endif // CFG_DEVELOPMENT_DEBUG

void set_pad_functions(void)
{
#ifdef CFG_PRINTF_UART2
    GPIO_ConfigurePin(UART2_TX_GPIO_PORT, UART2_TX_GPIO_PIN, OUTPUT, PID_UART2_TX, false);
    GPIO_ConfigurePin(UART2_RX_GPIO_PORT, UART2_RX_GPIO_PIN, INPUT, PID_UART2_RX, false);
#endif
    // Boot: set stimulation pads and status LED LOW
    GPIO_ConfigurePin(PORT_A, PIN_A, OUTPUT, PID_GPIO, false);
    GPIO_ConfigurePin(PORT_B, PIN_B, OUTPUT, PID_GPIO, false);
    GPIO_ConfigurePin(PORT_C, PIN_C, OUTPUT, PID_GPIO, false);
    GPIO_ConfigurePin(PORT_D, PIN_D, OUTPUT, PID_GPIO, false);
    GPIO_ConfigurePin(PORT_LED, PIN_LED, OUTPUT, PID_GPIO, false);
}

void periph_init(void)
{
    // Power up peripherals power domain
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
    while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP));

    SetBits16(CLK_16M_REG, XTAL16_BIAS_SH_ENABLE, 1);

    // ROM patch
    patch_func();

    // Init pads
    set_pad_functions();

#ifdef CFG_PRINTF_UART2
    SetBits16(CLK_PER_REG, UART2_ENABLE, 1);
    uart2_init(UART_BAUDRATE_115K2, 3);
#endif

    // Enable pads
    SetBits16(SYS_CTRL_REG, PAD_LATCH_EN, 1);
}

// ===============================================================================
// 3. STIMULATION CONTROL FUNCTIONS
// ===============================================================================
// SOFTWARE TIMER CALLBACK LOOP (For Frequencies < 31 Hz)
void software_pwm_cb(void) {
    if (sw_pwm_is_high) {
        // Pin is currently HIGH, pull it LOW
        switch(sw_pwm_pin) {
            case 0: GPIO_SetInactive(PORT_A, PIN_A); break;
            case 1: GPIO_SetInactive(PORT_B, PIN_B); break;
            case 2: GPIO_SetInactive(PORT_C, PIN_C); break;
            case 3: GPIO_SetInactive(PORT_D, PIN_D); break;
            case 4:
                GPIO_SetInactive(PORT_C, PIN_C);
                GPIO_SetInactive(PORT_A, PIN_A);
                break;
        }
        sw_pwm_is_high = false;
        // Schedule the LOW period (1 tick = 10ms)
        sw_pwm_timer_id = app_easy_timer(sw_pwm_low_ticks, software_pwm_cb);
    } else {
        // Pin is currently LOW, push it HIGH
        switch(sw_pwm_pin) {
            case 0: GPIO_SetActive(PORT_A, PIN_A); break;
            case 1: GPIO_SetActive(PORT_B, PIN_B); break;
            case 2: GPIO_SetActive(PORT_C, PIN_C); break;
            case 3: GPIO_SetActive(PORT_D, PIN_D); break;
            case 4:
                GPIO_SetActive(PORT_C, PIN_C);
                GPIO_SetActive(PORT_A, PIN_A);
                break;
        }
        sw_pwm_is_high = true;
        // Schedule the HIGH period (1 tick = 10ms)
        sw_pwm_timer_id = app_easy_timer(sw_pwm_high_ticks, software_pwm_cb);
    }
}

void pwm_output_OFF();

// TURN ON FUNCTION (Hybrid Hardware / Software)
void pwm_output_ON(uint8_t pin_no, uint16_t freq, uint8_t duty) {
		
		// 1. Intercept the OFF command
    if (duty == 0) {
        pwm_output_OFF();
        return;
    }
		
    // 1. Clean up any currently running timers
    if (sw_pwm_timer_id != EASY_TIMER_INVALID_TIMER) {
        app_easy_timer_cancel(sw_pwm_timer_id);
        sw_pwm_timer_id = EASY_TIMER_INVALID_TIMER;
    }
    timer0_stop();
    SetBits16(CLK_PER_REG, TMR_ENABLE, 0);

    // 2. Set ALL target pins to standard GPIO Output (strictly LOW)
    // This prevents the previous hardware PWM pin from bleeding into the new one
    GPIO_ConfigurePin(PORT_A, PIN_A, OUTPUT, PID_GPIO, false);
    GPIO_ConfigurePin(PORT_B, PIN_B, OUTPUT, PID_GPIO, false);
    GPIO_ConfigurePin(PORT_C, PIN_C, OUTPUT, PID_GPIO, false);
    GPIO_ConfigurePin(PORT_D, PIN_D, OUTPUT, PID_GPIO, false);

    // 3. Traffic Cop: Route to Software PWM or Hardware PWM
    if (freq < 31) {
        // --- SOFTWARE PWM (10ms Ticks) ---
        // 1 sec = 100 ticks. Calculate total period.
        uint16_t total_ticks = 100 / freq; 
        if (total_ticks == 0) total_ticks = 1;
        
        sw_pwm_high_ticks = (total_ticks * duty) / 100;
        sw_pwm_low_ticks = total_ticks - sw_pwm_high_ticks;
        
        // Safety bounds
        if (sw_pwm_high_ticks == 0) sw_pwm_high_ticks = 1;
        if (sw_pwm_low_ticks == 0) sw_pwm_low_ticks = 1;
        
        sw_pwm_pin = pin_no;
        
        // Jumpstart the pulse loop
        switch(pin_no) {
            case 0: GPIO_SetActive(PORT_A, PIN_A); break;
            case 1: GPIO_SetActive(PORT_B, PIN_B); break;
            case 2: GPIO_SetActive(PORT_C, PIN_C); break;
            case 3: GPIO_SetActive(PORT_D, PIN_D); break;
            case 4: 
                GPIO_SetActive(PORT_C, PIN_C);
                GPIO_SetActive(PORT_A, PIN_A);
                break;
        }
        sw_pwm_is_high = true;
        sw_pwm_timer_id = app_easy_timer(sw_pwm_high_ticks, software_pwm_cb);
        
    } else {
        // --- HARDWARE PWM (2MHz Fast Clock) ---
        // Switch the pins from GPIO control to internal hardware PWM control
        switch(pin_no) {
            case 0: GPIO_ConfigurePin(PORT_A, PIN_A, OUTPUT, PID_PWM0, false); break;
            case 1: GPIO_ConfigurePin(PORT_B, PIN_B, OUTPUT, PID_PWM0, false); break;
            case 2: GPIO_ConfigurePin(PORT_C, PIN_C, OUTPUT, PID_PWM0, false); break;
            case 3: GPIO_ConfigurePin(PORT_D, PIN_D, OUTPUT, PID_PWM0, false); break;
            case 4: 
                GPIO_ConfigurePin(PORT_C, PIN_C, OUTPUT, PID_PWM0, false);
                GPIO_ConfigurePin(PORT_A, PIN_A, OUTPUT, PID_PWM0, false);
                break;
        }

        uint32_t total_ticks = 2000000 / freq;
        uint16_t high_ticks = (total_ticks * duty) / 100;
        uint16_t low_ticks = total_ticks - high_ticks;
        
        // Safety bounds
        if (high_ticks == 0) high_ticks = 1;
        if (low_ticks == 0) low_ticks = 1;

        timer0_set_pwm_high_counter(high_ticks);
        timer0_set_pwm_low_counter(low_ticks);
        
        SetBits16(CLK_PER_REG, TMR_ENABLE, 1);
        set_tmr_div(CLK_PER_REG_TMR_DIV_8);
        timer0_init(TIM0_CLK_FAST, PWM_MODE_ONE, TIM0_CLK_NO_DIV);
        timer0_start();
    }

    // 4. Turn ON Status LED
    GPIO_SetActive(PORT_LED, PIN_LED);
}

// TURN OFF FUNCTION
void pwm_output_OFF(void) {
    // 1. Cancel Software PWM if it is running
    if (sw_pwm_timer_id != EASY_TIMER_INVALID_TIMER) {
        app_easy_timer_cancel(sw_pwm_timer_id);
        sw_pwm_timer_id = EASY_TIMER_INVALID_TIMER;
    }

    // 2. Stop Hardware Timer if it is running
    timer0_stop();
    SetBits16(CLK_PER_REG, TMR_ENABLE, 0);

    // 3. Disconnect PWM and pull all stimulation pads strictly LOW
    GPIO_ConfigurePin(PORT_A, PIN_A, OUTPUT, PID_GPIO, false);
    GPIO_ConfigurePin(PORT_B, PIN_B, OUTPUT, PID_GPIO, false);
    GPIO_ConfigurePin(PORT_C, PIN_C, OUTPUT, PID_GPIO, false);
    GPIO_ConfigurePin(PORT_D, PIN_D, OUTPUT, PID_GPIO, false);

    // 4. Turn OFF Status LED
    GPIO_SetInactive(PORT_LED, PIN_LED);
}
