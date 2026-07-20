#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include <string.h>
#include <u8g2.h>

extern int glyph_x;
extern int glyph_y;
extern int glyph_char;
extern u8g2_t u8g2;

uint8_t u8x8_byte_pico_hw_spi(u8x8_t*, uint8_t, uint8_t, void*);
uint8_t u8x8_gpio_and_delay_pico(u8x8_t*, uint8_t,uint8_t, void*);

void s_cls();
void s_draw();

void display_sequence();
