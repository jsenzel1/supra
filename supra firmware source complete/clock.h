#ifndef CLOCK_H
#define CLOCK_H

#include <stdbool.h>
#include <stdint.h>

// RTC time structure
typedef struct {
    int second;
    int minute;
    int hour;
    int day;
    int weekday;
    int month;
    int year;
} rtc_time_t;


// Global date seed in MMDDYY format
extern int daySeed;

// Day of year (0-364)
extern int yearDay;

extern int hour;
extern int minute;
extern int second;

extern int day;
extern int weekday;

extern int month;
extern int year;

extern bool is_leap_year;

// Initialize the clock hardware
int clock_init(void);

// Reset the clock to default settings
void clock_reset(void);

// Write time to the RTC
// Parameters:
//   doit - boolean to enable/disable the write operation
//   tWeekday - Day of week (0-6, Sunday=0)
//   tDay - Day of month (1-31)
//   tMonth - Month (1-12)
//   tYear - Year (0-99)
//   tHour - Hour (0-23)
//   tMinute - Minute (0-59)
//   tZone - Time zone offset
void clock_write(bool doit, int tWeekday, int tDay, int tMonth, 
                int tYear, int tHour, int tMinute, int tZone);

// Read current time from the RTC and update the global daySeed with date in MMDDYY format
void clock_read(void);

// Get the current time values
rtc_time_t clock_get_time(void);

// Validate the current RTC values
bool clock_validate(void);

#endif // CLOCK_H
