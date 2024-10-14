#include "test_module.h"

#include <Arduino.h>
#include "bindings/hal_common.h"
//#include "task_scheduler.h"
#include "bindings/bricklet_io16_v2.h"
#include "bindings/bricklet_industrial_quad_relay_v2.h"
#include "bindings/bricklet_e_paper_296x128.h"
#include "event_log.h"

static TF_IO16V2 io;
static TF_IndustrialQuadRelayV2 rel;
static TF_EPaper296x128 ep;

extern TF_HAL hal;

bool in_val[16];    
bool led_state = 0;
bool relay_state[4] = {0,0,0,0};
bool enc_1_raw;
bool enc_0_raw;
int8_t encoder_state = 0;
bool new_state = 1;
bool kill_state = 1;
bool enab_state = 0;

enum {
    LED_L= 6, //A6
    LED_R= 5, //A5
    ENC_0 = 10, //B3
    ENC_1 = 11, //B2
    ENC_B = 12, //B4
    BUT_L = 9, //B0
    BUT_R = 8, //B1
    KILL = 14, //B6
    ENAB = 12 //B4
};

void TestModule::pre_setup()
{
}

void encoder_evaluate(uint8_t channel)
{
    if(channel == ENC_0)
    {
        if(enc_0_raw == 0)
            encoder_state += enc_1_raw ? -1 : 1;
        else
            encoder_state += enc_1_raw ? +1 : -1;
    }
    if(channel == ENC_1)
    {
        if(enc_1_raw == 0)
            encoder_state += enc_0_raw ? 1 : -1;
        else
            encoder_state += enc_0_raw ? -1 : 1;
    }
    if(encoder_state == -1) encoder_state = 3;
    if(encoder_state == 4) encoder_state = 0;
}

/*void refresh_screen()
{
    tf_e_paper_296x128_set_update_mode(&ep, TF_E_PAPER_296X128_UPDATE_MODE_DEFAULT);
    tf_e_paper_296x128_fill_display(&ep, TF_E_PAPER_296X128_COLOR_WHITE);
    tf_e_paper_296x128_draw(&ep);
    tf_hal_sleep_us(&hal, 2000 * 1000);
    tf_e_paper_296x128_fill_display(&ep, TF_E_PAPER_296X128_COLOR_BLACK);
    tf_e_paper_296x128_draw(&ep);
    tf_hal_sleep_us(&hal, 2000 * 1000);
    tf_e_paper_296x128_fill_display(&ep, TF_E_PAPER_296X128_COLOR_RED);
    tf_e_paper_296x128_draw(&ep);
    tf_hal_sleep_us(&hal, 2000 * 1000);
}*/

void update_screen()
{

    char buf[20];
    tf_e_paper_296x128_fill_display(&ep, TF_E_PAPER_296X128_COLOR_WHITE);

                                // x start     y start     x end     y end
    tf_e_paper_296x128_draw_box(&ep,0+(encoder_state & 1)*148,      0+((encoder_state & 2)>>1)*40,
                                    148+(encoder_state & 1)*148-1,  40+((encoder_state & 2)>>1)*40-1,0,0);

    for(int i = 0; i<=3; i++) {
        if(relay_state[i]==1)    
            sprintf(buf, "REL %d:ON ",i+1);
        else
            sprintf(buf, "REL %d:OFF",i+1);
        tf_e_paper_296x128_draw_text(&ep, 16+(i & 1)*150, 10+(i & 2)*20, TF_E_PAPER_296X128_FONT_12X32,
                                       TF_E_PAPER_296X128_COLOR_BLACK,
                                       TF_E_PAPER_296X128_ORIENTATION_HORIZONTAL,
                                       buf);
    }

    tf_e_paper_296x128_draw_text(&ep, 7, 97, TF_E_PAPER_296X128_FONT_12X32,
                                       TF_E_PAPER_296X128_COLOR_BLACK,
                                       TF_E_PAPER_296X128_ORIENTATION_HORIZONTAL,
                                       "V1:00.0 V2:00.0 V3:00.0");
    tf_e_paper_296x128_draw(&ep);
}

static void input_value_handler(struct TF_IO16V2 *io16_v2, uint8_t channel, bool changed, bool value, void *user_data)
{
    switch (channel) {
    case BUT_L:
        if(value == 0) {
            relay_state[0] = !relay_state[0];
        } else
            return; //ignore LOW to HIGH on button
        break;
    case BUT_R:
        if(value == 0) {
            relay_state[1] = !relay_state[1];
            //kill_state = !kill_state;
            //enab_state = !enab_state;
        } else
            return; //ignore LOW to HIGH on button
        break;
    case ENC_0:
        enc_0_raw = value;
        break;
    case ENC_1:
        enc_1_raw = value;
        break;
    case ENC_B:
        if(value == 0) {
            relay_state[encoder_state] = !relay_state[encoder_state];
            //kill_state = !kill_state;
            //enab_state = !enab_state;
        } else
            return; //ignore LOW to HIGH on button
        break;
    default:
        break;
    }
    encoder_evaluate(channel);
    new_state = 1;
}

void TestModule::setup()
{
    // Create devices objects
    tf_io16_v2_create(&io, NULL, &hal);
    tf_industrial_quad_relay_v2_create(&rel, NULL, &hal);
    tf_e_paper_296x128_create(&ep, NULL, &hal);

    // Configure screen
    tf_e_paper_296x128_set_update_mode(&ep, TF_E_PAPER_296X128_UPDATE_MODE_DEFAULT);
    tf_e_paper_296x128_fill_display(&ep, TF_E_PAPER_296X128_COLOR_BLACK);
    tf_e_paper_296x128_draw(&ep);

    // Configure channels input/out
    tf_io16_v2_set_configuration(&io, LED_L, 'o', false); //led left
    tf_io16_v2_set_configuration(&io, LED_R, 'o', false); //led right
    tf_io16_v2_set_configuration(&io, KILL, 'o', false); //kill output
    tf_io16_v2_set_configuration(&io, ENAB, 'o', false); //enable output
    tf_io16_v2_set_configuration(&io, BUT_R, 'i', true); //right button
    tf_io16_v2_set_configuration(&io, BUT_L, 'i', true); //left button
    tf_io16_v2_set_configuration(&io, ENC_0, 'i', true); //encoder ch1
    tf_io16_v2_set_configuration(&io, ENC_1, 'i', true); //encoder ch2
    tf_io16_v2_set_configuration(&io, ENC_B, 'i', true); //encoder push button

    // read encoder start value
    tf_io16_v2_get_value(&io,in_val);
    enc_0_raw = in_val[ENC_0];
    enc_1_raw = in_val[ENC_1];

    // blink LED
   /* for (int i = 0; i < 10; ++i) {
        tf_hal_sleep_us(&hal, 50 * 1000);
        tf_io16_v2_set_selected_value(&io, LED, true);
        tf_hal_sleep_us(&hal, 50 * 1000);
        tf_io16_v2_set_selected_value(&io, LED, false);
    } */

    // Configure callbacks
    tf_io16_v2_register_input_value_callback(&io, input_value_handler, NULL);
    tf_io16_v2_set_input_value_callback_configuration(&io, BUT_R, 100, true);
    tf_io16_v2_set_input_value_callback_configuration(&io, ENC_0, 100, true);
    tf_io16_v2_set_input_value_callback_configuration(&io, ENC_1, 100, true);
    tf_io16_v2_set_input_value_callback_configuration(&io, BUT_L, 100, true);
    tf_io16_v2_set_input_value_callback_configuration(&io, ENC_B, 100, true);


    initialized = true;
    logger.printfln("Test module initialized");
    tf_e_paper_296x128_set_update_mode(&ep, TF_E_PAPER_296X128_UPDATE_MODE_BLACK_WHITE);
}

void TestModule::loop()
{
    //tf_hal_callback_tick(&hal, 0);
    tf_io16_v2_callback_tick(&io, 100);
    tf_hal_sleep_us(&hal, 100 * 1000);
      
    if(new_state){
        //led_state = enab_state;
        tf_io16_v2_set_selected_value(&io, LED_L, relay_state[0]);
        tf_io16_v2_set_selected_value(&io, LED_R, relay_state[1]);

        //tf_io16_v2_set_selected_value(&io, KILL, kill_state);
        //tf_io16_v2_set_selected_value(&io, ENAB, enab_state);

        tf_industrial_quad_relay_v2_set_value(&rel, relay_state);
        
        update_screen();
        new_state = 0;
    }
}
