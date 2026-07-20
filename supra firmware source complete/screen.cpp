#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include <string.h>
#include <u8g2.h>
#include "screen.h"

// I2C configuration
#define I2C_PORT i2c0

#define PIN_SDA 24  
#define PIN_SCL 25

//#define PIN_SDA 16
//#define PIN_SCL 17

#define I2C_SPEED 400*1000  // 400 kHz

int glyph_x=0;
int glyph_y=0;
int glyph_char=9616;

u8g2_t u8g2;

uint8_t u8x8_byte_pico_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    static uint8_t buffer[32];  // u8g2/u8x8 will never send more than 32 bytes between START_TRANSFER and END_TRANSFER
    static uint8_t buf_idx;
    uint8_t *data;
 
    switch(msg) {
        case U8X8_MSG_BYTE_SEND:
            data = (uint8_t *)arg_ptr;      
            while(arg_int > 0) {
                buffer[buf_idx++] = *data;
                data++;
                arg_int--;
            }      
            break;

        case U8X8_MSG_BYTE_INIT:
            // Initialize I2C
            i2c_init(I2C_PORT, I2C_SPEED);
            gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
            gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
            gpio_pull_up(PIN_SDA);
            gpio_pull_up(PIN_SCL);
            break;

        case U8X8_MSG_BYTE_SET_DC:
            // Ignored for I2C
            break;

        case U8X8_MSG_BYTE_START_TRANSFER:
            buf_idx = 0;
            break;

        case U8X8_MSG_BYTE_END_TRANSFER:
            // Send accumulated data via I2C
            i2c_write_blocking(I2C_PORT, u8x8_GetI2CAddress(u8x8) >> 1, buffer, buf_idx, false);
            break;

        default:
            return 0;
    }
    return 1;
}

uint8_t u8x8_gpio_and_delay_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch(msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            // Only initialize reset pin, as other pins are handled by I2C
            //gpio_init(PIN_RST);
            //gpio_set_dir(PIN_RST, GPIO_OUT);
            //gpio_put(PIN_RST, 1);
            break;

        case U8X8_MSG_DELAY_NANO:
            break;

        case U8X8_MSG_DELAY_100NANO:
            break;

        case U8X8_MSG_DELAY_10MICRO:
            break;

        case U8X8_MSG_DELAY_MILLI:
            sleep_ms(arg_int);
            break;

        case U8X8_MSG_GPIO_RESET:
            //gpio_put(PIN_RST, arg_int);
            break;

        default:
            u8x8_SetGPIOResult(u8x8, 1);
            break;
    }
    return 1;
}

void s_draw() {

    //u8g2_UpdateDisplay(&u8g2);
}

void s_cls() {
    //u8g2_ClearBuffer(&u8g2);
}

void display_sequence() {
    // Use the appropriate I2C setup function for your display
    // This example uses sh1106 - adjust according to your display type
    u8g2_Setup_sh1106_i2c_128x64_noname_f(&u8g2, U8G2_R2, u8x8_byte_pico_hw_i2c, u8x8_gpio_and_delay_pico);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    u8g2_ClearBuffer(&u8g2);
    u8g2_ClearDisplay(&u8g2);
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_SetFont(&u8g2, u8g2_font_t0_11_te);

    s_draw();
}
