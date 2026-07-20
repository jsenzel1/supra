// In morsedwords.h
#define PROGMEM __attribute__((section(".rodata")))

// Declare the array with PROGMEM attribute
extern const char* const words[] PROGMEM;
