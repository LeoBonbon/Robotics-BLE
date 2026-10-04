/**
****************************************************************************************
*
* @file user_custs1_impl.c
*
* @brief Peripheral project Custom1 Server implementation source code.
*
* Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
* program includes Confidential, Proprietary Information and is a Trade Secret of
* Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
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

#include "gpio.h"
#include "app_api.h"
#include "app.h"
#include "user_custs1_def.h"
#include "user_custs1_impl.h"
#include "user_peripheral.h"
#include "user_periph_setup.h"
#include "app_easy_timer.h"

// Bring in the hardware functions you just wrote
extern void pwm_output_ON(uint8_t pin_no, uint16_t freq, uint8_t duty);
extern void pwm_output_OFF(void);



/*
* GLOBAL VARIABLE DEFINITIONS
****************************************************************************************
*/

// Track the 500ms safety timer
timer_hnd stim_timer = EASY_TIMER_INVALID_TIMER;
// The callback that triggers after 500ms
void stim_timer_cb(void) {
    pwm_output_OFF();
    stim_timer = EASY_TIMER_INVALID_TIMER;
}

ke_msg_id_t timer_used;

/*
* FUNCTION DEFINITIONS
****************************************************************************************
*/

void user_custs1_ctrl_wr_ind_handler(ke_msg_id_t const msgid,
                                      struct custs1_val_write_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uint8_t val = 0;
    memcpy(&val, &param->value[0], param->length);
    if (val != CUSTS1_CP_ADC_VAL1_DISABLE)
    {
        timer_used = app_easy_timer(APP_PERIPHERAL_CTRL_TIMER_DELAY, app_adcval1_timer_cb_handler);
    }
    else
    {
        if (timer_used != 0xFFFF)
        {
            app_easy_timer_cancel(timer_used);
            timer_used = 0xFFFF;
        }
    }
}



void user_custs1_long_val_cfg_ind_handler(ke_msg_id_t const msgid,
                     struct custs1_val_write_ind const *param,
                     ke_task_id_t const dest_id,
                     ke_task_id_t const src_id)
{
}

void user_custs1_long_val_wr_ind_handler(ke_msg_id_t const msgid,
                     struct custs1_val_write_ind const *param,
                     ke_task_id_t const dest_id,
                     ke_task_id_t const src_id)
{
}

void user_custs1_long_val_ntf_cfm_handler(ke_msg_id_t const msgid,
                     struct custs1_val_write_ind const *param,
                     ke_task_id_t const dest_id,
                     ke_task_id_t const src_id)
{
}

void user_custs1_adc_val_1_cfg_ind_handler(ke_msg_id_t const msgid,
                      struct custs1_val_write_ind const *param,
                      ke_task_id_t const dest_id,
                      ke_task_id_t const src_id)
{
}

void user_custs1_adc_val_1_ntf_cfm_handler(ke_msg_id_t const msgid,
                      struct custs1_val_write_ind const *param,
                      ke_task_id_t const dest_id,
                      ke_task_id_t const src_id)
{
}

void user_custs1_button_cfg_ind_handler(ke_msg_id_t const msgid,
                    struct custs1_val_write_ind const *param,
                    ke_task_id_t const dest_id,
                    ke_task_id_t const src_id)
{
}

void user_custs1_button_ntf_cfm_handler(ke_msg_id_t const msgid,
                    struct custs1_val_write_ind const *param,
                    ke_task_id_t const dest_id,
                    ke_task_id_t const src_id)
{
}

void user_custs1_indicateable_cfg_ind_handler(ke_msg_id_t const msgid,
                       struct custs1_val_write_ind const *param,
                       ke_task_id_t const dest_id,
                       ke_task_id_t const src_id)
{
}

void user_custs1_indicateable_ind_cfm_handler(ke_msg_id_t const msgid,
                       struct custs1_val_write_ind const *param,
                       ke_task_id_t const dest_id,
                       ke_task_id_t const src_id)
{
}

void app_adcval1_timer_cb_handler()
{
  struct custs1_val_ntf_req* req = KE_MSG_ALLOC_DYN(CUSTS1_VAL_NTF_REQ,
                           TASK_CUSTS1,
                           TASK_APP,
                           custs1_val_ntf_req,
                           DEF_CUST1_ADC_VAL_1_CHAR_LEN);

    // ADC value to be sampled
    static uint16_t sample;
    sample = (sample <= 0xffff) ? (sample + 1) : 0;
    req->conhdl = app_env->conhdl;
    req->handle = CUST1_IDX_ADC_VAL_1_VAL;
    req->length = DEF_CUST1_ADC_VAL_1_CHAR_LEN;
    memcpy(req->value, &sample, DEF_CUST1_ADC_VAL_1_CHAR_LEN);
    ke_msg_send(req);

  if (ke_state_get(TASK_APP) == APP_CONNECTED)
  {
    // Set it once again until Stop command is received in Control Characteristic
    timer_used = app_easy_timer(APP_PERIPHERAL_CTRL_TIMER_DELAY, app_adcval1_timer_cb_handler);
  }
}

void user_custs1_led_wr_ind_handler(ke_msg_id_t const msgid,
                                    struct custs1_val_write_ind const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Check if the phone actually sent at least 3 bytes
    if (param->length >= 3) {
        
        // Extract the 3 bytes (0: Pin, 1: Frequency, 2: Duty Cycle)
        uint8_t pin_no = param->value[0];
        uint16_t freq  = param->value[1];
        uint8_t duty   = param->value[2];
        // Safety caps
        if (duty > 100) duty = 100;
        if (freq == 0) freq = 1; // Prevent divide-by-zero if user sends 0Hz
        // If a previous 500ms stimulation is still actively running, cancel it
        if (stim_timer != EASY_TIMER_INVALID_TIMER) {
            app_easy_timer_cancel(stim_timer);
            pwm_output_OFF();
        }
        // 1. Turn on the hardware for the requested pin
        pwm_output_ON(pin_no, freq, duty);
        // 2. Start the 500ms auto-off timer (50 units * 10ms = 500ms)
        stim_timer = app_easy_timer(50, stim_timer_cb);
    }
}

