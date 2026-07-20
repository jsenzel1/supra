// seqo.h
#ifndef SEQO_H
#define SEQO_H

#include "Module.h"
#include <stdio.h>
#include <stdlib.h>
#include <u8g2.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/clocks.h"
#include "screen.h"
#include "Controls.h"
#include "IO.h"
#include "Clock.h"
#include "utils.h"
#include "jfunc.h"

// Define constants
#define MAX_BOXES 64
#define MAX_SEQ_LENGTHS 3
#define MAX_BOX_TYPES 6
#define MAX_BOX_ANIM_BANK 10
#define MAX_BOX_ANIM_POS 8

// Enum for box types
typedef enum {
    none,
    plain,
    skip2,
    skip3,
    skip4,
    skip5,
    glitch 
} boxType;

// Structure for playhead
struct Playhead {
    int pos;
    int id;
    bool on;
    bool flag;
};

// Structure for box
struct Box {
    boxType type;
    int x;
    int y;
    bool on;
    int id;
    int animPos;
    int glyph;
    int skipMax;
    int skipInd;
};

// Union for MetaBox
typedef Box boxArray[64];
union MetaBox {
    struct {
        boxArray boxes1;
        boxArray boxes2;
        boxArray boxes3;
    };
    boxArray all[3];
};

class Seqo : public Module {  // Inherit from Module
private:
    bool boxTypeFlip;
    // TESTING VARS
    bool autoClock;
    bool followLeftClock;
    bool followLeftClockPrev;
    int clkDelay;

    // Control flags
    bool drawSeqo;

    // Pools
    int boxesLeftPool[32];
    int boxesLeftPoolLen;
    int seqPool[32];
    int seqPoolLen;
    int boxesLeft;

    // Timing
    Interval inTick;
    Interval followTick;
    int clockDelay;
    long long ms;
    bool inFlag;
    bool followWaitingForEdge;
    bool followMasterPrevHigh;
    int followMasterRiseCount;
    int followMasterBpmSnapshot;
    int followMasterMultSnapshot;
    int followLocalMultSnapshot;

    // Visual
    int boxSpacing;
    int cursorX;
    int cursorY;
    int numBoxes1;
    int numBoxes2;
    int xTemp;
    int playheadCount;
    int seqLengths[MAX_SEQ_LENGTHS];
    int seqSelect;
    int seqSpacing;
    int key;

    // Box Animation
    int boxAnimBank[MAX_BOX_ANIM_BANK][MAX_BOX_ANIM_POS];
    int boxIcons[MAX_BOX_TYPES];

    // Box Type Selection
    boxType heldBoxType;
    boxType boxTypeSelectArr[5];
    int boxTypeToday;
    int boxTypeSelectLen;
    int boxTypeSelectInd;

    // Boxes and Playheads
    MetaBox metaBox;
    Playhead head1;
    Playhead head2;
    Playhead head3;

    // Menu system (added from Morsed)
    bool showMenu;
    int menuIndex;
    int bpm;
    int mult;
    char bpmText[10];
    char multText[10];

    // Helper Methods
    void shuffleBoxTypes(boxType *array, int size);
    void playhead_init(Playhead &ph);
    void playhead_tick_skip(Playhead* ph, Box* curBox);
    void playhead_tick_plain(Playhead* ph, Box* curBox);
    void playhead_tick(Playhead* ph);
    void playhead_draw(Playhead* ph);
    void playhead_reset(Playhead* ph);
    void boxes_init();
    void drawBoxes(Box *boxes, int length);
    void boxes_draw();
    void cursor_draw();
    void controls_tick();
    void draw_menu();
    void menu_controls();

    //void draw();
    void update_vals();

public:
    // Constructor
    Seqo(int hand);

    // Override methods from Module
    void initialize() override;
    void run() override;
    void draw() override;
    void runFast() override;
    void resume();
};

#endif // SEQO_H
