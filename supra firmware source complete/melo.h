#ifndef MELO_H
#define MELO_H

#include "Module.h"
#include <u8g2.h>
#include <algorithm>
#include "screen.h"
#include "Controls.h"
#include "Clock.h"
#include "IO.h"
#include "utils.h"
#include "jfunc.h"
#include "pico/stdlib.h"

extern const char* NOTE_NAMES[12];
int  decimal_to_pitch_class_array(int decimal, int* pitchClasses);

struct note {
    int   row;
    int   glyph;
    int   val;        // semitone number (C-0 == 0)
    int   x, y;
    bool  on;
};

class Melo : public Module {
private:

    int selectedScale=0;

    int summerScales[7][2]={
        //church modes and variations
        {2741,2731},
        {1709,1967},
        {1451,1459},
        {2773,1493},
        {1717,1845},
        {1453,2475},
        {1387,1395}
    };

    int fallScales[7][2]={
        {1009,505},
        {351,501},
        {637,3877},
        {1119,3351},
        {3271,3683},
        {1777,1931},
        {3467,3725}
    };

    int winterScales[7][2]={
        {1507,1423},
        {1509,981},
        {701,3403},
        {1815,1863},
        {2619,2955},
        {2845,1595},
        {695,2899}
    };
    
    int springScales[7][2]={
        {427,2261},
        {1333,1429},
        {373,2603},
        {2265,2253},
        {469,3157},
        {1365,2457},
        {723,813}
    };
        

    bool isDaytime=false;

    int   libRow = 0;
    bool  autoClock;
    int   clkDelay;
    int   cursorx, cursory;
    long  ms;
    int   ySpacing = 10,  yCollumns = 0;
    //int   xSpacing = 17, xSpacingSeq = 12;
    int   xSpacing = 17, xSpacingSeq = 14;
    int   libSize  = 8,  currentLibSize = 0;
    int   libCursor = 0, seqCursor = 0;
    bool  editingLib = true, triggerFlag = false;
    int   playheadPos = 0;
    note  sequence[32];
    note  library[17];
    int   pitchGlyphs[24]  = {49,50,51,52,53,54,55,56,57,58,59,60,
                              61,62,63,64,65,66,67,68,69,70,71,72};
    int   possiblePitches[12] = {0};
    int   scales[3] = {623, 1809, 2187};
    int   currentScaleIndex = 0;
    int   seqLen = 0;
    float dacVoltages[25];

    bool showMenu = false;
    int  menuIndex = 0;           // 0 = semis, 1 = oct
    int  transposeSemis = 0;      // –11 … +11
    int  transposeOct   = 0;      // –5 … +5
    static constexpr int kOctMin  = -5;
    static constexpr int kOctMax  =  5;
    static constexpr int kSemiMin = 0;
    static constexpr int kSemiMax = 120;  // 10 V

    bool flashingNote = false;
    int flashingNoteOrigIndex = -1;  // Original index for reference
    note flashingNoteData{};
    long flashTimer = 0;
    bool flashState = true;
    static constexpr long kFlashPeriodMs = 150; // Flash toggle every 250ms
    note originalSequence[32];   // Store the original sequence during edit mode
    int originalSeqLen = 0;      // Original sequence length

    void controls_tick();
    void drawSequence();
    void drawLibrary();
    void drawCursors();
    void drawPlayhead();
    void processTrigger();
    void outputNote(int seqPosition);
    void loadScale(int scaleDecimal);
    void recalcSeqPositions();
    void draw_menu();
    void menu_controls();
    void shiftSequence(int semitoneAmount);
    void shiftLibrary(int semitoneAmount);
    
    // New edit mode helpers
    void startNoteEdit(int index);
    void finishNoteEdit();
    void updateFlashingState();
    void rearrangeSequenceForCursor();  // Improved version
    void initScaleTime();




public:
    Melo(int hand);
    void initialize() override;
    void run()       override;
    void draw()      override;
    void runFast()   override;
    void resume();
};

#endif
