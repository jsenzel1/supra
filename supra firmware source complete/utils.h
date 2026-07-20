#ifndef UTILS_H
#define UTILS_H

#include "pico/stdlib.h"
#include "hardware/clocks.h"

extern void ramReport();

class Interval {
public:
    long delay;
    long pulseWidth;
    long nextToggleTime;
    bool val;


    // Constructor declaration
    Interval(long tempDelay, int tempPulseWidth);

    // Method to be called every frame
    bool run(bool reset=false);

    void util_reboot();
};

#endif // UTILS_H

