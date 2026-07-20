#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "pwm.pio.h"   // generated from pwm.pio
#include "IO.h"

//TODO set up all these pins as accesible
// 19 16 17 18
// 16 is reset L, 18 is reset R

bool IO_outState = false;
//int IO_outs[4] = {0, 0, 0, 0};
int IO_inPins[4] = {19,16,17,18};
bool IO_outStates[4] = {false, false, false, false};
int IO_ledPins[4]={27,26,21,5};

bool IO_inStates[4] = {false, false, false, false};
bool IO_inStatesMomentary[4] = {false, false, false, false};
bool IO_inFlags[4] = {false, false, false, false};

volatile bool IO_selfClockEnabledByHand[2] = {false, false};
volatile int IO_selfClockBpmByHand[2] = {0, 0};
volatile int IO_selfClockMultByHand[2] = {0, 0};

// Store wrap value globally to avoid recalculating
uint16_t IO_pwmWrap = 65535;

// Add current DAC channel variable
uint8_t IO_currentDACChannel = 0;

bool sharedPulse=false;
int sharedBpm=100;

Interval sharedInt = Interval(60000/(sharedBpm*8), 50);

typedef struct {
    bool initialized;
    PIO pio;
    int sm;
    uint offset;
} pio_pwm_chan_t;

static pio_pwm_chan_t __pio_pwm_by_pin[32] = {0};

static int _pio_index(PIO p) { return (p == pio1) ? 1 : 0; }
static int _pwm_prog_offset[2] = {-1, -1}; // per-PIO program offsets

static inline void pio_pwm_set_period(PIO pio, uint sm, uint32_t period) {
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_put_blocking(pio, sm, period);
    pio_sm_exec(pio, sm, pio_encode_pull(false, false));
    pio_sm_exec(pio, sm, pio_encode_out(pio_isr, 32));
    pio_sm_set_enabled(pio, sm, true);
}

static inline void pio_pwm_set_level(PIO pio, uint sm, uint32_t level) {
    pio_sm_put_blocking(pio, sm, level);
}

static bool _pio_pwm_claim(PIO &pio, int &sm) {
    // Try pio0, then pio1
    pio = pio0;
    sm = pio_claim_unused_sm(pio, false);
    if (sm < 0) {
        pio = pio1;
        sm = pio_claim_unused_sm(pio, false);
    }
    return sm >= 0;
}

//DAC Handling
class MCP4728 {
private:
    i2c_inst_t* i2c;
    uint8_t address;
    
public:
    enum Channel {
        CHANNEL_A = 0x00,
        CHANNEL_B = 0x02,
        CHANNEL_C = 0x04,
        CHANNEL_D = 0x06
    };
    
    MCP4728(i2c_inst_t* i2c_port, uint8_t addr = 0x60) : i2c(i2c_port), address(addr) {}
    
    bool writeChannel(Channel channel, uint16_t value) {
        value &= 0x0FFF;  // Ensure 12-bit range
        
        uint8_t output_buffer[3];
        output_buffer[0] = 0x40 | channel;  // Single write command
        output_buffer[1] = (0x80 | (value >> 8));  // Vref = VDD, gain = 1x, PD=normal, upper 4 bits
        output_buffer[2] = value & 0xFF;  // Lower 8 bits
        
        return i2c_write_blocking(i2c, address, output_buffer, 3, false) == 3;
    }

    bool configureAllChannels() {
        uint8_t config_cmd[9];
        config_cmd[0] = 0x80;  // Multi-write
        for (int i = 0; i < 4; i++) {
            config_cmd[i*2+1] = 0x80;  // Vref = VDD, gain = 1x, PD normal
            config_cmd[i*2+2] = 0x00;
        }
        return i2c_write_blocking(i2c, address, config_cmd, 9, false) == 9;
    }

    static Channel getChannelEnum(uint8_t channelNum) {
        switch(channelNum) {
            case 0: return CHANNEL_A;
            case 1: return CHANNEL_B;
            case 2: return CHANNEL_C;
            case 3: return CHANNEL_D;
            default: return CHANNEL_A;
        }
    }
};

static MCP4728* dac = nullptr;

void IO_init_pwm(uint pin) {
    if (pin < 32 && __pio_pwm_by_pin[pin].initialized) {
        return;
    }

    gpio_init(pin);

    // Claim a PIO and SM
    PIO pio;
    int sm;
    if (!_pio_pwm_claim(pio, sm)) {
        printf("PIO PWM: No free state machine available for pin %u\n", pin);
        return;
    }

    // Load the program into this PIO once
    int pidx = _pio_index(pio);
    if (_pwm_prog_offset[pidx] < 0) {
        _pwm_prog_offset[pidx] = pio_add_program(pio, &pwm_program);
    }

    // Init the program for this SM/pin
    pio_gpio_init(pio, pin);

    // Init the program for this SM/pin
    pwm_program_init(pio, sm, _pwm_prog_offset[pidx], pin);
    // Set the period (wrap). Keep previous behavior of 16-bit wrap.
    uint32_t period = (IO_pwmWrap == 0) ? ((1u << 16) - 1) : IO_pwmWrap;
    pio_pwm_set_period(pio, sm, period);

    // Start at 0 brightness
    pio_pwm_set_level(pio, sm, 0);

    // Bookkeeping
    if (pin < 32) {
        __pio_pwm_by_pin[pin].initialized = true;
        __pio_pwm_by_pin[pin].pio = pio;
        __pio_pwm_by_pin[pin].sm  = sm;
        __pio_pwm_by_pin[pin].offset = _pwm_prog_offset[pidx];
    }
}

void IO_write_pwm(uint pin, uint16_t level) {
    if (pin >= 32) return;

    if (!__pio_pwm_by_pin[pin].initialized) {
        IO_init_pwm(pin);
        if (!__pio_pwm_by_pin[pin].initialized) return;
    }

    PIO pio = __pio_pwm_by_pin[pin].pio;
    int sm   = __pio_pwm_by_pin[pin].sm;

    pio_pwm_set_level(pio, sm, (uint32_t)level);
}

void IO_init() {
    // Initialize input pins
    for(int i=0; i<4; i++) {
        gpio_init(IO_inPins[i]);
        gpio_set_dir(IO_inPins[i], GPIO_IN);
        gpio_pull_up(IO_inPins[i]);
    }

    // Initialize ALL LED pins with PIO PWM
    IO_init_pwm(IO_ledPins[0]); IO_write_pwm(IO_ledPins[0], 0); // 27
    IO_init_pwm(IO_ledPins[1]); IO_write_pwm(IO_ledPins[1], 0); // 26
    IO_init_pwm(IO_ledPins[2]); IO_write_pwm(IO_ledPins[2], 0); // 21
    IO_init_pwm(IO_ledPins[3]); IO_write_pwm(IO_ledPins[3], 0); // 5

    // Initialize I2C with error checking
    if (!i2c_init(i2c1, 400000)) { // 400kHz
        printf("Failed to initialize I2C\n");
        return;
    }

    // Configure I2C pins
    gpio_set_function(10, GPIO_FUNC_I2C);  // SDA
    gpio_set_function(11, GPIO_FUNC_I2C);  // SCL
    gpio_pull_up(10);
    gpio_pull_up(11);

    // Check DAC presence
    uint8_t rxdata;
    int ret = i2c_read_blocking(i2c1, 0x60, &rxdata, 1, false);
    if (ret < 0) {
        printf("No response from DAC at address 0x60\n");
        return;
    }

    dac = new MCP4728(i2c1);
    if (dac == nullptr) {
        printf("Failed to allocate DAC object\n");
        return;
    }

    if (!dac->configureAllChannels()) {
        printf("Failed to configure DAC channels\n");
    } else {
        printf("DAC channels configured successfully\n");
    }

    if (!dac->writeChannel(MCP4728::CHANNEL_A, 0)) {
        printf("Initial DAC write test failed\n");
        delete dac;
        dac = nullptr;
        return;
    }
    
    printf("DAC initialized successfully\n");
}

void IO_runPulse()
{
    sharedInt.run();
}

void IO_check(int pin) {
    IO_inStatesMomentary[pin] = false;

    int pinNum=IO_inPins[pin];

    if (gpio_get(pinNum) == 1) {
        IO_inStates[pin] = true;
    } else {
        IO_inStates[pin] = false;
    }

    if (gpio_get(pinNum) == 1 && !IO_inFlags[pin]) {
        IO_inFlags[pin] = true;
        IO_inStatesMomentary[pin] = true;
    }

    if (gpio_get(pinNum) == 0) {
        IO_inFlags[pin]= false;
    }
}

void IO_write(int hand) {
    if (dac != nullptr) {
        if (hand == 0) {
            dac->writeChannel(MCP4728::CHANNEL_A, IO_outStates[0] ? 4095 : 0);
            dac->writeChannel(MCP4728::CHANNEL_B, IO_outStates[1] ? 4095 : 0);

            IO_write_pwm(IO_ledPins[0], IO_outStates[0] ? IO_pwmWrap : 0); 
            IO_write_pwm(IO_ledPins[1], IO_outStates[1] ? IO_pwmWrap : 0);

        } else if (hand == 1) {

            dac->writeChannel(MCP4728::CHANNEL_C, IO_outStates[2] ? 4095 : 0);
            dac->writeChannel(MCP4728::CHANNEL_D, IO_outStates[3] ? 4095 : 0);

            IO_write_pwm(IO_ledPins[2], IO_outStates[2] ? IO_pwmWrap : 0); 
            IO_write_pwm(IO_ledPins[3], IO_outStates[3] ? IO_pwmWrap : 0);
        }
    }
}

void IO_writeDAC(uint8_t channel, uint16_t value) {
    if (dac != nullptr && channel < 4) {
        value = value > 4095 ? 4095 : value;
        
        switch(channel) {
            case 0: 
                dac->writeChannel(MCP4728::CHANNEL_A, value); 
                IO_write_pwm(IO_ledPins[0], value * 16); 
                break;

            case 1: 
                dac->writeChannel(MCP4728::CHANNEL_B, value); 
                IO_write_pwm(IO_ledPins[1], value * 16);
                break;

            case 2: 
                dac->writeChannel(MCP4728::CHANNEL_C, value); 
                IO_write_pwm(IO_ledPins[2], value * 16); 
                break;

            case 3: 
                dac->writeChannel(MCP4728::CHANNEL_D, value); 
                IO_write_pwm(IO_ledPins[3], value * 16);
                break;
        }
    }
}
