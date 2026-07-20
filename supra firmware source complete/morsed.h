    #ifndef MORSED_H
#define MORSED_H

#include "Module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <cstring>
#include <u8g2.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/clocks.h"
#include "screen.h"
#include "Controls.h"
#include "IO.h"
#include "utils.h"
#include "jfunc.h"

#define NUM_POINTS 64

// Structure for playhead
struct playhead {
    int x;
    int y;
    int loc;
    int id;
    int restCount;
    int flag;
    int curParentInd;
    int loopLoc;
    int max;
    bool rest;
};

// Structure for point
struct point {
    int x;
    int y;
    int type;
    int parentInd;
    bool cur;
    bool flip;
    char pChar;
};

class Morsed : public Module {
private:
    bool letterSpaces;
    int resetOffsetCount;
    int totalValidPoints;  
    int totalMorseLength;  
    int keyCharIndices[32];  // Track character positions
    int currentLetterIndex;  // Track the current letter being displayed

    // Control flags
    bool drawMorsed;
    int clkDelay;
    bool autoClock;
    Interval inTick;

    long msTillNewWords=0;

    bool inUsedForReset=true;

    bool followLeftClock=false;
    bool followLeftClockPrev=false;

    bool placing;
    int endX;
    int endY;
    
    // Timing
    int clockDelay;
    bool pointSpaceFlip;
    int randGlyph;
    float moveSpeedDefault;
    float moveSpeed;
    uint64_t ms;
    uint64_t nextBlip;
    bool blip;
    bool blipHold;
    bool blipFlag;
    
    // Morse code variables
    int letInd;
    bool inFlag;
    int lArrowPos;
    int rArrowPos;
    int loopStart;
    int loopEnd;
    int paraX;
    int paraPos;
    char para[500];
    std::string strword;
    // Arrays for morse code
    char* morse_char[28] = {"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z","\.","\,"};
    char* morse_char_combo = (char*)"ABCDEFGHIJKLMNOPQRSTUVWXYZ\.\,";
    char* morse_key[28] = {".-","-...","-.-.","-..",".","..-.","--.","....","..",".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",".--","-..-","-.--","--..",".-.-.-","--..--"};
    char* ditRef[3] = {".", "-","x"};
    char myKeys[32][16];
    char glueString[128] = {'\0'};
    
    // Morse step variables
    char curKey[10] = "";
    char curBit = '.';
    int rest = 0;
    bool rest1 = false;
    bool rest3 = false;
    bool doneRest1 = false;
    int restCount = 0;
    int output = 1;
    int keyInd = 0;
    int dashCount = 0;
    
    // Point and playhead structures
    struct point points[NUM_POINTS];
    struct playhead ph;
    struct point curPoint;
    
    // Menu variables
    bool showMenu;
    int menuIndex;
    bool splitOutput;

    int bpm=100;
    int mult=2;
    
    char bpmText[8]={0};
    char multText[8]={0};

    // Helper methods
    char* charToKey(char inChar);
    void string_create();
    void string_sep();
    void ph_init();
    void make_points();
    void rotatePoints(int direction);
    void ph_step();
    void draw_points();
    void draw_ph();
    void draw_text();
    void draw_menu();
    void menu_controls();
    void conCheck();

public:
    // Constructor
    Morsed(int hand);
    
    // Override methods from Module
    void initialize() override;
    void run() override;
    void draw() override;
    void runFast() override;
};

#endif // MORSED_H
