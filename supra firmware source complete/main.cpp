// main.cpp
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/timer.h"
#include "Clock.h"   
#include "Module.h"  
#include "Squigl.h"  
#include "Seqo.h"    
#include "Melo.h"    
#include "morsed.h"    
#include "Screen.h"  
#include "Controls.h"
#include "IO.h"
#include "menus.cpp"
#include "fs.h"
#include "reflash_flags.h"
#include "pico/stdio.h"
#include "pico_hal.h"
#include "stdinit.h"
#include "bitmaps.c"
#include "anims.cpp"

#include "tusb.h"

// Dev-only escape hatch: set to 1 (via CMake `SUPRA_BYPASS_DATE_SET=1`)
// to skip the date/time menu entirely during rapid reflashing.
#ifndef SUPRA_BYPASS_DATE_SET
#define SUPRA_BYPASS_DATE_SET 0
#endif

// Forward declarations of module classes
class Seqo;
class Morsed;

// Create module instances for each hand
// Left hand modules (hand ID 0)
Melo meloLeft(0);
Squigl squiglLeft(0);
Seqo seqoLeft(0);
Morsed morsedLeft(0);

// Right hand modules (hand ID 1)
Melo meloRight(1);
Squigl squiglRight(1);
Seqo seqoRight(1);
Morsed morsedRight(1);

// Pointers to active modules (only one per hand)
Module* leftModule = &morsedLeft;    
Module* rightModule = &squiglRight; 

// Shared state between cores
volatile bool running = true;
int lookHand = 0;  // 0 = left hand, 1 = right hand
int lh = 20;
int rh = 6;
int frameCounter = 0;

// Menu system variables
bool menuActive = false;
int menuSelection = 0;
const char* menuItems[] = {"Abacus", "Stamp", "Cryptex", "Chime"};
const int menuItemCount = 4;

// Button press state tracking
bool button0WasPressed = false;
uint64_t button0PressStartTime = 0;
const int LONG_PRESS_TIME = 350; // ms for long press detection
bool longPressActivated = false;

int lhHoldTemp=0;
int rhHoldTemp=0;
bool shouldWritePrefs=false;
bool shouldReadPrefs=true;
bool suppressPrefWrite=false;
int prefsHandToWrite=0;
int prefsValueToWrite=0;

// Core 1 entry point - handles high-frequency DAC updates
void core1_entry() {
    sleep_ms(100);
    while (running) {
        IO_selfClockEnabledByHand[0] = false;
        IO_selfClockEnabledByHand[1] = false;
        leftModule->runFast();
        rightModule->runFast();
    }
}

// Function to display menu using u8g2 library
void displayMenu() {
    u8g2_ClearBuffer(&u8g2);

    //u8g2_DrawBox(&u8g2,42,0,85,64);
    
    // Draw menu title
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    u8g2_SetFontPosTop(&u8g2);

    int xoff=74*(1-lookHand);

    u8g2_SetDrawColor(&u8g2, 1);
    if(lookHand==0)
    {
        u8g2_DrawStr(&u8g2, xoff, 5, "L Hand:");
        u8g2_DrawXBM(&u8g2,1,1,53,62,lhbmp);
    } else {
        u8g2_DrawStr(&u8g2, 0, 5, "R Hand:");
        u8g2_DrawXBM(&u8g2,76,1,53,62,rhbmp);
    }
    
    // Draw menu items with selection indicator
    int yOffset = 20;
    int lineHeight = 10;
    
    for (int i = 0; i < menuItemCount; i++) {
        int y = yOffset + (i * lineHeight);
        
        // Highlight selected item
        if (i == menuSelection) {
            // Draw selection highlight (inverted box)
            u8g2_SetDrawColor(&u8g2, 1);
            u8g2_DrawBox(&u8g2, 0+xoff, y, 41, lineHeight);
            u8g2_SetDrawColor(&u8g2, 0);  // Set to inverse color for text
            
            // Draw selection arrow
            //u8g2_SetFont(&u8g2, u8g2_font_unifont_t_symbols);
            //u8g2_DrawGlyph(&u8g2, 10, y + 1, 9658);  // Right arrow symbol
        } else {
            u8g2_SetDrawColor(&u8g2, 1);  // Normal color for text
        }
        
        // Draw menu item text
        u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
        u8g2_DrawStr(&u8g2, 0+xoff, y, menuItems[i]);
        
        // Reset draw color to normal
        u8g2_SetDrawColor(&u8g2, 1);
    }
    
    // Update the display
    u8g2_UpdateDisplay(&u8g2);
}

void applyModuleSelection() {
    //selectAnim();
    // Get current hand module pointer

    
    Module** currentHandModule = (lookHand == 0) ? &leftModule : &rightModule;

    if (menuSelection < 0) {
        menuSelection = 0;
    } else if (menuSelection >= menuItemCount) {
        menuSelection = 0;
    }
    
    // Determine which module to activate based on the current hand and menu selection
    if (lookHand == 0) {  // Left hand

        switch (menuSelection) {
            case 0: // Seqo
                *currentHandModule = &seqoLeft;
                break;
                
            case 1: // Squigl
                *currentHandModule = &squiglLeft;
                break;
                
            case 2: // Morsed
                *currentHandModule = &morsedLeft;
                break;
                
            case 3: // Melo
                *currentHandModule = &meloLeft;
                break;
        }

    } else {  // Right hand
        gpio_put(21,0);
        gpio_put(5,0);
        switch (menuSelection) {
            case 0: // Seqo
                *currentHandModule = &seqoRight;
                break;
                
            case 1: // Squigl
                *currentHandModule = &squiglRight;
                break;
                
            case 2: // Morsed
                *currentHandModule = &morsedRight;
                break;
                
            case 3: // Melo
                *currentHandModule = &meloRight;
                break;
        }
    }

   
    
    // Always reinitialize the module when selected from the menu
    // This ensures all internal state is properly reset
    printf("Changed module for %s hand to %s - reinitializing...\n", 
           lookHand == 0 ? "left" : "right", 
           menuItems[menuSelection]);
    
    // Reinitialize the new module
    (*currentHandModule)->initialize();
    
    // Auto-close the menu when a selection is made
    menuActive = false;
  
    if(lookHand==0)
    {
        lhHoldTemp=menuSelection;
    } 

    if(lookHand==1)
    {
        rhHoldTemp=menuSelection;
    } 

    if (!suppressPrefWrite) {
        prefsHandToWrite = lookHand;
        prefsValueToWrite = menuSelection;
        shouldWritePrefs = true;
    }

    gpio_put(27,0);
    gpio_put(26,0);
    gpio_put(21,0);
    gpio_put(5,0);

   
}

// Main function runs on core 0
int main() {
    // Initialize stdio for debugging
    stdio_init_all();

    u8g2_ClearBuffer(&u8g2);
    display_sequence();

    //u8g2_SetFont(&u8g2, u8g2_font_logisoso42_tf);
    u8g2_SetFont(&u8g2, u8g2_font_logisoso26_tf);
    u8g2_SetFontMode(&u8g2, 1);
    int start=12;
    int spacing=random(17,22);
    int y=48;
    int rl=30;
    int ru=60;
    int rx=0;
    int ry=0;
    int delay=12;

    //y=random(rl,ru);
   
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2, start, y,"S");
    sleep_ms(delay);
    u8g2_UpdateDisplay(&u8g2);

    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2, start+(spacing), y,"U");
    sleep_ms(delay);
    u8g2_UpdateDisplay(&u8g2);

    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2, start+(spacing*2), y,"P");
    sleep_ms(delay);
    u8g2_UpdateDisplay(&u8g2);

    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2, start+(spacing*3), y,"R");
    sleep_ms(delay);
    u8g2_UpdateDisplay(&u8g2);

    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2, start+(spacing*4), y,"A");
    sleep_ms(delay);
    u8g2_UpdateDisplay(&u8g2);
    
    int rxl=10; 
    int rxh=80; 

    int ryl=30; 
    int ryh=50; 

    //sleep_ms(1000);
    
    con_init();  

    //while (!tud_cdc_connected()) { sleep_ms(100);}

    printf("con init done\n");
                              
    if (pico_mount(false) != LFS_ERR_OK) {
        printf("Error mounting FS AT MENU\n");
        pico_mount(true);
        //runDateMenu();
    }

    printf("pico mount done\n");
    
    if (!SUPRA_BYPASS_DATE_SET) {
        if(!supra_reflash_date_is_set())
        {
            runDateMenu();
            supra_reflash_date_mark_set();
        }
    }
    

    /*
    menuSelection=fs_readInt("prefs_lhHold");
    printf("menu selectL: %d\n",menuSelection);

    Module** currentHandModule = &leftModule;

    switch(menuSelection)
    {
        case 0:
            *currentHandModule=&seqoLeft;
        case 1:
            *currentHandModule=&squiglLeft;
        case 2:
            *currentHandModule=&morsedLeft;
        case 3:
            *currentHandModule=&meloLeft;
    }
    */

    /*
    menuSelection=fs_readInt("prefs_rhHold");
    printf("menu selectR: %d\n",menuSelection);
    applyModuleSelection();
    */

    //selectAnim();

    lookHand=0;
    
    printf("fs write read done\n");
    printf("prefs load done\n");
    

    // Initialize system components
    if(clock_init()==1)
    {
        runDateMenu();
        clock_init();
    }

    clock_read();

    printf("clocks done\n");

    //srand(03032025);
    srand(daySeed);
    IO_init();

    //IO_write_pwm(26, IO_pwmWrap);  // should go full bright
    //IO_write_pwm(5,  IO_pwmWrap); 

    printf("IO done\n");
    
    // Initialize all modules for both hands
    meloLeft.initialize();
    printf("got past melo init\n");
    squiglLeft.initialize();
    printf("got past squig init\n");
    seqoLeft.initialize();
    printf("got past seqo init\n");
    morsedLeft.initialize();
    printf("got past morsed init\n");
    
    meloRight.initialize();
    squiglRight.initialize();
    seqoRight.initialize();
    morsedRight.initialize();

    // Configure GPIOs
    
    gpio_init(lh);
    gpio_set_dir(lh, GPIO_OUT);
    gpio_init(rh);
    gpio_set_dir(rh, GPIO_OUT);

    printf("got to post gpios");

    gpio_put(lh, 1);
    
    
    // Launch core 1 with our entry point
    printf("Starting core 1 for high-frequency DAC updates...\n");
    multicore_launch_core1(core1_entry);
    
    // Main loop on core 0 - handles UI, control, and display
    while (true) {
        if(shouldReadPrefs)
        {
            multicore_reset_core1();  //needed
                                      //
            suppressPrefWrite = true;

            menuSelection=fs_readInt("prefs_lhHold");
            if (menuSelection < 0 || menuSelection >= menuItemCount) {
                menuSelection = 0;
            }
            printf("menu selectL: %d\n",menuSelection);
            lookHand=0;
            applyModuleSelection();

            menuSelection=fs_readInt("prefs_rhHold");
            if (menuSelection < 0 || menuSelection >= menuItemCount) {
                menuSelection = 0;
            }
            printf("menu selectR: %d\n",menuSelection);
            lookHand=1;
            applyModuleSelection();

            lookHand=0;

            multicore_launch_core1(core1_entry);
            shouldReadPrefs=false;
            shouldWritePrefs=false;
            suppressPrefWrite = false;
        }    

        if(shouldWritePrefs)
        {
            multicore_reset_core1();  //needed
                                      
            if(prefsHandToWrite==0)
            {
                fs_writeInt("prefs_lhHold",prefsValueToWrite);

            } else {

                fs_writeInt("prefs_rhHold",prefsValueToWrite);
            }
            multicore_launch_core1(core1_entry);

            shouldWritePrefs=false;
  

            //sleep_ms(1);
        }

        // Current time for button press duration checking
        uint64_t currentTime = to_ms_since_boot(get_absolute_time());
        
        // Check button state - completely rewritten button handling logic
        bool button0IsPressed = button_contin(0);
        
        // State changes for button 0
        if (button0IsPressed && !button0WasPressed) {
            // Button just pressed
            button0PressStartTime = currentTime;
            longPressActivated = false;
        }
        
        // Handle long press detection and activation
        if (button0IsPressed && !longPressActivated && 
            (currentTime - button0PressStartTime >= LONG_PRESS_TIME)) {
            
            if (!menuActive) {
                // Only activate menu if it's not already active
                menuActive = true;
                
                // Set initial menu selection based on current module
                if (lookHand == 0) {
                    // Left hand
                    if (leftModule == &meloLeft) menuSelection = 3;
                    else if (leftModule == &seqoLeft) menuSelection = 0;
                    else if (leftModule == &squiglLeft) menuSelection = 1;
                    else if (leftModule == &morsedLeft) menuSelection = 2;
                    else menuSelection = 0; // Default
                } else {
                    // Right hand
                    if (rightModule == &squiglRight) menuSelection = 1;
                    else if (rightModule == &morsedRight) menuSelection = 2;
                    else if (rightModule == &meloRight) menuSelection = 3;
                    else if (rightModule == &seqoRight) menuSelection = 0;
                    else menuSelection = 0; // Default
                }
                
                displayMenu();
                printf("Menu activated for %s hand\n", lookHand == 0 ? "left" : "right");
            }
            
            longPressActivated = true; // Mark that we've processed this long press
        }
        
        // Handle short press detection
        if (!button0IsPressed && button0WasPressed) {
            // Button just released
            if (!longPressActivated && (currentTime - button0PressStartTime < LONG_PRESS_TIME)) {
                // This was a short press
                if (menuActive) {
                    // If menu is active, close it
                    menuActive = false;
                    printf("Menu deactivated\n");
                } else {
                    // Regular press behavior (switch lookHand)
                    if (lookHand == 1) {
                        lookHand = 0;
                        gpio_put(rh, 0);
                        gpio_put(lh, 1);
                        //swapAnim(0);
                        swapAnim(0);
                    } else if (lookHand == 0) {
                        lookHand = 1;
                        gpio_put(rh, 1);
                        gpio_put(lh, 0);
                        swapAnim(1);
                    }
                }
            }
        }
        
        // Save current button state for next loop
        button0WasPressed = button0IsPressed;

        IO_runPulse();
        
        if (menuActive) {
            // MENU MODE - handle joystick navigation
            int joyInput = con_joyMomentary();
            bool needRefresh = false;
            
            switch (joyInput) {
                case 1: // UP
                    if (menuSelection > 0) {
                        menuSelection--;
                        needRefresh = true;
                    }
                    break;
                    
                case 2: // DOWN
                    if (menuSelection < menuItemCount - 1) {
                        menuSelection++;
                        needRefresh = true;
                    }
                    break;
                    
                case 5: // CENTER (select)
                    // Select the current menu item, apply it and close menu
                    //
                    selectAnim();
                    applyModuleSelection();
                    break;
            }
            
            if (menuActive && (needRefresh || frameCounter % 5 == 0)) {
                displayMenu();
            }
        } else {
            // NORMAL MODE - run modules and update display
            
            // Run the slower logic portions of both active modules
            leftModule->run();
            rightModule->run();
            
            // Update display for the active module based on lookHand
            if (lookHand == 0) {
                leftModule->draw();
            } else {
                rightModule->draw();
            }
        }

        // Add a small delay to prevent core 0 from hogging resources
        // This doesn't affect core 1's timing for DAC updates
        sleep_ms(10);
        frameCounter++;
    }
    
    return 0;
}
