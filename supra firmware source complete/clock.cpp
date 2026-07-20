// PCF8523 RTC chip integration

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "hardware/i2c.h"
#include "utils.h"
#include "clock.h"

// RTC device address
#define RTC_ADDR 0x68

// RTC register addresses
#define CONTROL_1_REG   0x00
#define CONTROL_2_REG   0x01
#define CONTROL_3_REG   0x02
#define SECONDS_REG     0x03
#define MINUTES_REG     0x04
#define HOURS_REG       0x05
#define DAYS_REG        0x06
#define WEEKDAYS_REG    0x07
#define MONTHS_REG      0x08
#define YEARS_REG       0x09

// Global time variables
static rtc_time_t current_time = {0};
static uint8_t raw_time[7];

// Global daySeed variable in MMDDYY format
int daySeed = 0;

// Global yearDay variable (0-364, day of year)
int yearDay = 0;
int year = 0;
int day = 0;
int hour = 0;
int month = 0;
int weekday = 0;
int minute = 0;
int second = 0;
bool is_leap_year=false;

// I2C pins
static int clk_sda_pin = 24;
static int clk_scl_pin = 25;

// BCD conversion utilities
static uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }

// Helper function for I2C writes
static bool i2c_write_reg(uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    int result = i2c_write_blocking(i2c0, RTC_ADDR, buf, 2, false);
    return (result > 0); // Return true if successful
}

void clock_reset() {
    printf("Resetting clock...\n");
    
    // Reset control registers to known state
    bool success = true;
    
    // Control 1: Stop the clock, disable the watchdog timer, standard frequency
    success &= i2c_write_reg(CONTROL_1_REG, 0x00);
    
    // Control 2: Battery switchover enabled, no interrupt
    success &= i2c_write_reg(CONTROL_2_REG, 0xA0);
    
    // Control 3: No additional features enabled
    success &= i2c_write_reg(CONTROL_3_REG, 0x00);
    
    // Start the clock
    success &= i2c_write_reg(CONTROL_1_REG, 0x00);
    
    if (!success) {
        printf("ERROR: Failed to reset clock\n");
    }
}

int clock_init() {
    printf("Initializing clock...\n");

    i2c_init(i2c0, 400 * 1000);
    gpio_set_function(clk_sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(clk_scl_pin, GPIO_FUNC_I2C);

    gpio_pull_up(clk_sda_pin);
    gpio_pull_up(clk_scl_pin);

    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(clk_sda_pin, clk_scl_pin, GPIO_FUNC_I2C));
    
    // Reset clock to ensure proper state
    clock_reset();
    
    // Read the current time to initialize our values
    clock_read();
    
    // Validate time values
    if (!clock_validate()) {
        printf("WARNING: RTC has invalid time values, consider setting time\n");
        return 1;
    } else {
        return 0;
    }
}

void clock_write(bool doit, int tWeekday, int tDay, int tMonth, int tYear, 
                int tHour, int tMinute, int tZone) {
    if (!doit) {
        return;
    }

    printf("Setting clock: %02d/%02d/%02d %02d:%02d (Weekday: %d, Zone: %d)\n", 
           tMonth, tDay, tYear, tHour, tMinute, tWeekday, tZone);
    
    // Validate input values
    if (tMinute < 0 || tMinute > 59 || 
        tHour < 0 || tHour > 23 ||
        tDay < 1 || tDay > 31 ||
        tMonth < 1 || tMonth > 12 ||
        tYear < 0 || tYear > 99 ||
        tWeekday < 0 || tWeekday > 6) {
        printf("ERROR: Invalid time parameters\n");
        return;
    }
    
    // Start with seconds at 0
    int second = 0;
    
    // Convert all values to BCD
    uint8_t bcd_values[7] = {
        bin2bcd(second),
        bin2bcd(tMinute),
        bin2bcd(tHour),
        bin2bcd(tDay),
        bin2bcd(tWeekday),
        bin2bcd(tMonth),
        bin2bcd(tYear)
    };
    
    // First stop the clock to ensure safe update
    i2c_write_reg(CONTROL_1_REG, 0x20); // Set STOP bit
    
    // Write time values to registers
    bool success = true;
    for (int i = 0; i < 7; i++) {
        success &= i2c_write_reg(SECONDS_REG + i, bcd_values[i]);
    }
    
    // Restart the clock
    i2c_write_reg(CONTROL_1_REG, 0x00);
    
    if (!success) {
        printf("ERROR: Failed to write time values\n");
        return;
    }
    
    // Read back the values to verify
    sleep_ms(10); // Brief delay to allow RTC to update
    clock_read();
    
    // Store the set values for verification
    current_time.second = 0;
    current_time.minute = tMinute;
    current_time.hour = tHour;
    current_time.day = tDay;
    current_time.weekday = tWeekday;
    current_time.month = tMonth;
    current_time.year = tYear;
    
    // Check values were set correctly
    rtc_time_t read_time = clock_get_time();
    if (read_time.minute != tMinute || read_time.hour != tHour || 
        read_time.day != tDay || read_time.month != tMonth) {
        printf("WARNING: Clock values don't match what was set!\n");
        printf("Set: %02d/%02d %02d:%02d, Read: %02d/%02d %02d:%02d\n",
               tMonth, tDay, tHour, tMinute,
               read_time.month, read_time.day, read_time.hour, read_time.minute);
    }
}

void clock_read() {
    // For this particular device, we send the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing.
    uint8_t reg = SECONDS_REG;
    int result;
    
    // Write the starting register
    result = i2c_write_blocking(i2c0, RTC_ADDR, &reg, 1, true); // true to keep master control of bus
    if (result < 0) {
        printf("ERROR: Failed to write register address (%d)\n", result);
        return;
    }
    
    // Read 7 bytes starting from SECONDS_REG
    result = i2c_read_blocking(i2c0, RTC_ADDR, raw_time, 7, false);
    if (result < 0) {
        printf("ERROR: Failed to read time (%d)\n", result);
        return;
    }
    
    // Clear the clock stop flag if it's set (indicates power loss)
    if (raw_time[0] & 0x80) {
        printf("WARNING: Clock stop detected (power loss)\n");
        // Clear the clock stop flag
        uint8_t new_seconds = raw_time[0] & 0x7F;
        i2c_write_reg(SECONDS_REG, new_seconds);
    }
    
    // Convert BCD values to binary
    current_time.second = bcd2bin(raw_time[0] & 0x7F); // Mask out the clock stop flag
    current_time.minute = bcd2bin(raw_time[1] & 0x7F);
    current_time.hour = bcd2bin(raw_time[2] & 0x3F);
    current_time.day = bcd2bin(raw_time[3] & 0x3F); 
    current_time.weekday = raw_time[4] & 0x07;
    current_time.month = bcd2bin(raw_time[5] & 0x1F);
    current_time.year = bcd2bin(raw_time[6]);
    
    // Validate values are in reasonable ranges
    if (!clock_validate()) {
        printf("WARNING: Invalid time values detected\n");
        // Could attempt to reset clock here
    }
    
    // Update the daySeed global variable with MMDDYY format
    daySeed = (current_time.month * 10000) + (current_time.day * 100) + current_time.year;
    
    // Calculate yearDay (day of year, 0-364)
    // Array of days in each month (for non-leap year)
    const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Check if it's a leap year
    bool is_leap_year_local = (current_time.year % 4 == 0) && 
                       ((current_time.year % 100 != 0) || (current_time.year % 400 == 0));
    
    // Calculate day of year
    yearDay = current_time.day - 1; // Start with days in current month (0-indexed)
    // Add days from previous months
    for (int m = 0; m < current_time.month - 1; m++) {
        yearDay += days_in_month[m];
        // Add leap day if February in a leap year
        if (m == 1 && is_leap_year_local) {
            yearDay += 1;
        }
    }
    is_leap_year=is_leap_year_local;
    year=current_time.year;
    day=current_time.day;
    hour=current_time.hour;
    month=current_time.month;
    weekday=current_time.weekday;
    minute=current_time.minute;
    second=current_time.second;
    
    printf("%02d/%02d/%02d %02d:%02d:%02d (daySeed: %d, yearDay: %d)\n", 
           current_time.month, current_time.day, current_time.year, 
           current_time.hour, current_time.minute, current_time.second,
           daySeed, yearDay);
}

rtc_time_t clock_get_time() {
    return current_time;
}

bool clock_validate() {
    // Check if values are in valid ranges
    if (current_time.second < 0 || current_time.second > 59 ||
        current_time.minute < 0 || current_time.minute > 59 ||
        current_time.hour < 0 || current_time.hour > 23 ||
        current_time.day < 1 || current_time.day > 31 ||
        current_time.weekday < 0 || current_time.weekday > 6 ||
        current_time.month < 1 || current_time.month > 12 ||
        current_time.year < 0 || current_time.year > 99) {
        return false;
    }
    
    return true;
}
