#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "utils.h"
#include <malloc.h>

uint32_t getTotalHeap(void) {
   extern char __StackLimit, __bss_end__;
   
   return &__StackLimit  - &__bss_end__;
}
uint32_t getFreeHeap(void) {
   struct mallinfo m = mallinfo();

   return getTotalHeap() - m.uordblks;
}

void ramReport()
{
    printf("total heap: %d\n",getTotalHeap());
    printf("free heap: %d\n",getFreeHeap());

}

void util_reboot()
{
    watchdog_enable(1,1);
}

Interval::Interval(long tempDelay, int tempPulseWidth) {
    delay = tempDelay;
    if (tempPulseWidth == 0) {
        pulseWidth = 0; 
    } else {
        pulseWidth = (long)((tempDelay * (tempPulseWidth / 100.0)));
    }
    nextToggleTime = to_ms_since_boot(get_absolute_time()); 
    val = false; 
}
bool Interval::run(bool reset) {
    long currentTime = to_ms_since_boot(get_absolute_time());

    // This is the key change - add a maximum time delta check
    if(reset || (currentTime - nextToggleTime > delay * 3)) {
        // If we've been inactive for too long, just reset the timer
        // instead of trying to catch up with many rapid toggles
        nextToggleTime = currentTime;
        // Optional: reset to a known state if needed
        val = false;
    }

    if (currentTime >= nextToggleTime) {
        if (val) { 
            nextToggleTime += (pulseWidth == 0) ? delay : delay - pulseWidth;
        } else { 
            nextToggleTime += pulseWidth == 0 ? 1 : pulseWidth; 
        }
        val = !val; 
    }
    return val;
}
