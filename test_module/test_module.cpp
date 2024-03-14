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

enum {
    LED = 7,
    ENC_0 = 6,
    ENC_1 = 5,
    BUT_R = 3,
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

void refresh_screen()
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
}

void update_screen()
{

    char buf[20];
    tf_e_paper_296x128_fill_display(&ep, TF_E_PAPER_296X128_COLOR_WHITE);

    tf_e_paper_296x128_draw_box(&ep,0+(encoder_state & 1)*148,0+((encoder_state & 2)>>1)*64,
                                    148+(encoder_state & 1)*148-1,64+((encoder_state & 2)>>1)*64-1,0,0);

    for(int i = 0; i<=3; i++) {
        if(relay_state[i]==1)    
            sprintf(buf, "Rel_%d:ON",i);
        else
            sprintf(buf, "Rel_%d:OFF",i);
        tf_e_paper_296x128_draw_text(&ep, 16+(i & 1)*150, 25+(i & 2)*30, TF_E_PAPER_296X128_FONT_12X32,
                                       TF_E_PAPER_296X128_COLOR_BLACK,
                                       TF_E_PAPER_296X128_ORIENTATION_HORIZONTAL,
                                       buf);
    }
    tf_e_paper_296x128_draw(&ep);
}

static void input_value_handler(struct TF_IO16V2 *io16_v2, uint8_t channel, bool changed, bool value, void *user_data)
{
    switch (channel) {
    case BUT_R:
        if(value == 0) {
            relay_state[encoder_state] = !relay_state[encoder_state];
        } else
            return; //ignore LOW to HIGH on button
        break;
    case ENC_0:
        enc_0_raw = value;
        break;
    case ENC_1:
        enc_1_raw = value;
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
    tf_io16_v2_set_configuration(&io, LED, 'o', false); //led
    tf_io16_v2_set_configuration(&io, BUT_R, 'i', true); //right button
    tf_io16_v2_set_configuration(&io, ENC_0, 'i', true); //encoder
    tf_io16_v2_set_configuration(&io, ENC_1, 'i', true); //encoder

    // read encoder start value
    tf_io16_v2_get_value(&io,in_val);
    enc_0_raw = in_val[ENC_0];
    enc_1_raw = in_val[ENC_1];

    // blink LED
    for (int i = 0; i < 10; ++i) {
        tf_hal_sleep_us(&hal, 50 * 1000);
        tf_io16_v2_set_selected_value(&io, LED, true);
        tf_hal_sleep_us(&hal, 50 * 1000);
        tf_io16_v2_set_selected_value(&io, LED, false);
    }

    // Configure callbacks
    tf_io16_v2_register_input_value_callback(&io, input_value_handler, NULL);
    tf_io16_v2_set_input_value_callback_configuration(&io, BUT_R, 100, true);
    tf_io16_v2_set_input_value_callback_configuration(&io, ENC_0, 100, true);
    tf_io16_v2_set_input_value_callback_configuration(&io, ENC_1, 100, true);

    initialized = true;
    logger.printfln("Test module initialized");
    tf_e_paper_296x128_set_update_mode(&ep, TF_E_PAPER_296X128_UPDATE_MODE_DELTA);
}

void TestModule::loop()
{
    //tf_hal_callback_tick(&hal, 0);
    tf_io16_v2_callback_tick(&io, 100);
    tf_hal_sleep_us(&hal, 100 * 1000);
      
    if(new_state){
        led_state = relay_state[encoder_state];
        tf_io16_v2_set_selected_value(&io, LED, led_state);

        tf_industrial_quad_relay_v2_set_value(&rel, relay_state);
        
        update_screen();
        new_state = 0;
    }
}
