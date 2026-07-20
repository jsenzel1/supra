// menus.cpp
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "Controls.h"
#include "Screen.h"
#include "Clock.h"

// Date and time values
int selectedDay = 1;
int selectedMonth = 1;
int selectedYear = 2025;
int selectedHour = 0;
int selectedMinute = 0;
int selectedDayOfWeek = 0; // 0 = Sunday, 1 = Monday, etc.

// Day of week names
const char* dayOfWeekNames[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

// Menu state
int dateMenuSelection = 0; // 0 = day, 1 = month, 2 = year, 3 = day of week, 4 = hour, 5 = minute, 6 = OK button
bool dateMenuActive = true;

// Constants for display
const int dateFieldX = 1;    // X position of the first date field (moved to almost the left edge)
const int dateFieldY = 32;   // Y position of the date fields
const int dateFieldSpacing = 18; // Reduced spacing between fields
const int dateMenuDelay = 5;    // Delay in ms for menu responsiveness

// Function prototypes
void displayDateMenu();
void handleDateMenuInput();
void applyDateSelection();

// Main date menu function
void runDateMenu() {
    dateMenuSelection = 0;
    
    // Display initial menu
    displayDateMenu();
    
    // Menu loop
    while (dateMenuActive) {
        // Handle user input
        handleDateMenuInput();
        
        // Update display if needed
        displayDateMenu();
        
        // Add a small delay to prevent CPU hogging
        sleep_ms(dateMenuDelay);
    }
}

// Function to display the date menu
void displayDateMenu() {
    u8g2_ClearBuffer(&u8g2);
    
    // Draw menu title
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    u8g2_SetFontPosTop(&u8g2);
    u8g2_DrawStr(&u8g2, 10, 5, "Date & Time Select");
    
    // Draw date and time fields and labels
    char dayStr[5], monthStr[5], yearStr[8], hourStr[5], minStr[5], okStr[5];
    sprintf(dayStr, "%02d", selectedDay);
    sprintf(monthStr, "%02d", selectedMonth);
    sprintf(yearStr, "%02d", selectedYear % 100); // Show only last 2 digits of year
    sprintf(hourStr, "%02d", selectedHour);
    sprintf(minStr, "%02d", selectedMinute);
    sprintf(okStr, "OK");
    
    // Draw field labels
    u8g2_SetFont(&u8g2, u8g2_font_4x6_tf);
    u8g2_DrawStr(&u8g2, dateFieldX, dateFieldY - 12, "D");
    u8g2_DrawStr(&u8g2, dateFieldX + dateFieldSpacing, dateFieldY - 12, "M");
    u8g2_DrawStr(&u8g2, dateFieldX + dateFieldSpacing * 2, dateFieldY - 12, "Y");
    u8g2_DrawStr(&u8g2, dateFieldX + dateFieldSpacing * 3, dateFieldY - 12, "WD");
    u8g2_DrawStr(&u8g2, dateFieldX + dateFieldSpacing * 4, dateFieldY - 12, "HR");
    u8g2_DrawStr(&u8g2, dateFieldX + dateFieldSpacing * 5, dateFieldY - 12, "MIN");
    
    // Draw field values
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    
    // Day field
    if (dateMenuSelection == 0) {
        // Highlight selected field
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_DrawBox(&u8g2, dateFieldX - 2, dateFieldY - 2, 16, 14);
        u8g2_SetDrawColor(&u8g2, 0);
    } else {
        u8g2_SetDrawColor(&u8g2, 1);
    }
    u8g2_DrawStr(&u8g2, dateFieldX, dateFieldY, dayStr);
    
    // Month field
    if (dateMenuSelection == 1) {
        // Highlight selected field
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_DrawBox(&u8g2, dateFieldX + dateFieldSpacing - 2, dateFieldY - 2, 16, 14);
        u8g2_SetDrawColor(&u8g2, 0);
    } else {
        u8g2_SetDrawColor(&u8g2, 1);
    }
    u8g2_DrawStr(&u8g2, dateFieldX + dateFieldSpacing, dateFieldY, monthStr);
    
    // Year field
    if (dateMenuSelection == 2) {
        // Highlight selected field
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_DrawBox(&u8g2, dateFieldX + dateFieldSpacing * 2 - 2, dateFieldY - 2, 16, 14);
        u8g2_SetDrawColor(&u8g2, 0);
    } else {
        u8g2_SetDrawColor(&u8g2, 1);
    }
    u8g2_DrawStr(&u8g2, dateFieldX + dateFieldSpacing * 2, dateFieldY, yearStr);
    
    // Day of week field
    if (dateMenuSelection == 3) {
        // Highlight selected field
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_DrawBox(&u8g2, dateFieldX + dateFieldSpacing * 3 - 2, dateFieldY - 2, 16, 14);
        u8g2_SetDrawColor(&u8g2, 0);
    } else {
        u8g2_SetDrawColor(&u8g2, 1);
    }
    u8g2_DrawStr(&u8g2, dateFieldX + dateFieldSpacing * 3, dateFieldY, dayOfWeekNames[selectedDayOfWeek]);
    
    // Hour field
    if (dateMenuSelection == 4) {
        // Highlight selected field
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_DrawBox(&u8g2, dateFieldX + dateFieldSpacing * 4 - 2, dateFieldY - 2, 16, 14);
        u8g2_SetDrawColor(&u8g2, 0);
    } else {
        u8g2_SetDrawColor(&u8g2, 1);
    }
    u8g2_DrawStr(&u8g2, dateFieldX + dateFieldSpacing * 4, dateFieldY, hourStr);
    
    // Minute field
    if (dateMenuSelection == 5) {
        // Highlight selected field
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_DrawBox(&u8g2, dateFieldX + dateFieldSpacing * 5 - 2, dateFieldY - 2, 16, 14);
        u8g2_SetDrawColor(&u8g2, 0);
    } else {
        u8g2_SetDrawColor(&u8g2, 1);
    }
    u8g2_DrawStr(&u8g2, dateFieldX + dateFieldSpacing * 5, dateFieldY, minStr);
    
    // OK button
    if (dateMenuSelection == 6) {
        // Highlight selected button
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_DrawBox(&u8g2, dateFieldX + dateFieldSpacing * 6 - 2, dateFieldY - 2, 16, 14);
        u8g2_SetDrawColor(&u8g2, 0);
    } else {
        u8g2_SetDrawColor(&u8g2, 1);
    }
    u8g2_DrawStr(&u8g2, dateFieldX + dateFieldSpacing * 6, dateFieldY, okStr);
    
    // Reset draw color
    u8g2_SetDrawColor(&u8g2, 1);
    
    // Draw navigation hint at the bottom
    u8g2_SetFont(&u8g2, u8g2_font_4x6_tf);
    u8g2_DrawStr(&u8g2, 5, 55, "Left/Right: Navigate | Up/Down: Change");
    
    // Update the display
    u8g2_UpdateDisplay(&u8g2);
}

// Function to handle user input for the date menu
void handleDateMenuInput() {
    // Check for left/right navigation
    int joyInput = con_joyMomentary();
    
    if (joyInput == 3) { // Left
        if (dateMenuSelection > 0) {
            dateMenuSelection--;
        }
    } else if (joyInput == 4) { // Right
        if (dateMenuSelection < 6) {
            dateMenuSelection++;
        }
    }
    
    // Check for up/down value changes or button selection
    int joyValueChange = con_joyAccelerate();
    
    if (dateMenuSelection == 0) { // Day field
        if (joyValueChange == 1) { // Up
            selectedDay++;
            // Validate day range based on month
            int daysInMonth = 31; // Default for most months
            
            if (selectedMonth == 4 || selectedMonth == 6 || 
                selectedMonth == 9 || selectedMonth == 11) {
                daysInMonth = 30;
            } else if (selectedMonth == 2) {
                // Simple leap year check
                bool isLeapYear = (selectedYear % 4 == 0 && 
                                  (selectedYear % 100 != 0 || selectedYear % 400 == 0));
                daysInMonth = isLeapYear ? 29 : 28;
            }
            
            if (selectedDay > daysInMonth) {
                selectedDay = 1;
            }
        } else if (joyValueChange == 2) { // Down
            selectedDay--;
            // Validate day range
            int daysInMonth = 31; // Default for most months
            
            if (selectedMonth == 4 || selectedMonth == 6 || 
                selectedMonth == 9 || selectedMonth == 11) {
                daysInMonth = 30;
            } else if (selectedMonth == 2) {
                // Simple leap year check
                bool isLeapYear = (selectedYear % 4 == 0 && 
                                  (selectedYear % 100 != 0 || selectedYear % 400 == 0));
                daysInMonth = isLeapYear ? 29 : 28;
            }
            
            if (selectedDay < 1) {
                selectedDay = daysInMonth;
            }
        }
    } else if (dateMenuSelection == 1) { // Month field
        if (joyValueChange == 1) { // Up
            selectedMonth++;
            //PURPOSEFUL BUG 
            if (selectedMonth > 12) {
                selectedMonth = 1;
            }
            // Validate day when month changes
            if ((selectedMonth == 4 || selectedMonth == 6 || 
                 selectedMonth == 9 || selectedMonth == 11) && selectedDay > 30) {
                selectedDay = 30;
            } else if (selectedMonth == 2) {
                bool isLeapYear = (selectedYear % 4 == 0 && 
                                  (selectedYear % 100 != 0 || selectedYear % 400 == 0));
                int maxDay = isLeapYear ? 29 : 28;
                if (selectedDay > maxDay) {
                    selectedDay = maxDay;
                }
            }
        } else if (joyValueChange == 2) { // Down
            selectedMonth--;
            if (selectedMonth < 1) {
                selectedMonth = 12;
            }
            // Validate day when month changes
            if ((selectedMonth == 4 || selectedMonth == 6 || 
                 selectedMonth == 9 || selectedMonth == 11) && selectedDay > 30) {
                selectedDay = 30;
            } else if (selectedMonth == 2) {
                bool isLeapYear = (selectedYear % 4 == 0 && 
                                  (selectedYear % 100 != 0 || selectedYear % 400 == 0));
                int maxDay = isLeapYear ? 29 : 28;
                if (selectedDay > maxDay) {
                    selectedDay = maxDay;
                }
            }
        }
    } else if (dateMenuSelection == 2) { // Year field
        if (joyValueChange == 1) { // Up
            selectedYear++;
            if (selectedYear > 2100) { // Arbitrary upper limit
                selectedYear = 2100;
            }
            // Validate February 29th on leap year changes
            if (selectedMonth == 2 && selectedDay == 29) {
                bool isLeapYear = (selectedYear % 4 == 0 && 
                                  (selectedYear % 100 != 0 || selectedYear % 400 == 0));
                if (!isLeapYear) {
                    selectedDay = 28;
                }
            }
        } else if (joyValueChange == 2) { // Down
            selectedYear--;
            if (selectedYear < 2000) { // Arbitrary lower limit
                selectedYear = 2000;
            }
            // Validate February 29th on leap year changes
            if (selectedMonth == 2 && selectedDay == 29) {
                bool isLeapYear = (selectedYear % 4 == 0 && 
                                  (selectedYear % 100 != 0 || selectedYear % 400 == 0));
                if (!isLeapYear) {
                    selectedDay = 28;
                }
            }
        }
    } else if (dateMenuSelection == 3) { // Day of Week field
        if (joyValueChange == 1) { // Up
            selectedDayOfWeek++;
            if (selectedDayOfWeek > 6) {
                selectedDayOfWeek = 0;
            }
        } else if (joyValueChange == 2) { // Down
            selectedDayOfWeek--;
            if (selectedDayOfWeek < 0) {
                selectedDayOfWeek = 6;
            }
        }
    } else if (dateMenuSelection == 4) { // Hour field
        if (joyValueChange == 1) { // Up
            selectedHour++;
            if (selectedHour > 23) {
                selectedHour = 0;
            }
        } else if (joyValueChange == 2) { // Down
            selectedHour--;
            if (selectedHour < 0) {
                selectedHour = 23;
            }
        }
    } else if (dateMenuSelection == 5) { // Minute field
        if (joyValueChange == 1) { // Up
            selectedMinute++;
            if (selectedMinute > 590) {
                selectedMinute = 0;
            }
        } else if (joyValueChange == 2) { // Down
            selectedMinute--;
            if (selectedMinute < 0) {
                selectedMinute = 59;
            }
        }
    } else if (dateMenuSelection == 6) { // OK button
        if (joyValueChange == 5 || joyInput == 5) { // Center press
            applyDateSelection();
            dateMenuActive = false;
        }
    }
    
    // Check for button press to exit
    if (button_momentary(1)) {
        applyDateSelection();
        dateMenuActive = false;
    }
}

// Function to apply date selection and print the result
void applyDateSelection() {
    clock_init();
    // Note: The clock_write function takes parameters in this order:
    // (set_time, wkday, date, month, year-2000, hour, minute, second)
    clock_write(true, selectedDayOfWeek, selectedDay, selectedMonth, selectedYear-2000, selectedHour, selectedMinute, 0);
    
    printf("Date & Time selected: %s %02d/%02d/%04d %02d:%02d\n", 
           dayOfWeekNames[selectedDayOfWeek], 
           selectedDay, selectedMonth, selectedYear, 
           selectedHour, selectedMinute);
    
    clock_read();
}
