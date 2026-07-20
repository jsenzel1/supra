#pragma once
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "utils.h"

// Externs used across the project
extern int IO_outPin;
extern int IO_outs[];
extern int IO_inPins[];
extern bool IO_outStates[];
extern int IO_ledPins[];

extern bool IO_inStates[];
extern bool IO_inStatesMomentary[];
extern bool IO_inFlags[];

// Per-hand self-clock enable flag (set by modules that generate their own clock).
// Cleared each core1 loop before module runFast() calls.
extern volatile bool IO_selfClockEnabledByHand[2];
// When IO_selfClockEnabledByHand[hand] is true, these reflect the current clock settings.
extern volatile int IO_selfClockBpmByHand[2];
extern volatile int IO_selfClockMultByHand[2];

// Legacy PWM fields kept for API compatibility (PIO now used underneath)
extern int IO_pwmPin;
extern uint IO_pwmSlice;
extern uint16_t IO_pwmWrap;

// Shared pulse/clock state
extern Interval sharedInt;
extern int sharedBpm;
extern bool sharedPulse;

// Public API (names unchanged)
void IO_init();
void IO_check(int pin);
void IO_write(int hand);
void IO_writeDAC(uint8_t channel, uint16_t value);
void IO_runPulse();

// New PIO-backed LED PWM (replaces old MCU PWM internally, same outward use)
void IO_init_pwm(uint pin);
void IO_write_pwm(uint pin, uint16_t level);
