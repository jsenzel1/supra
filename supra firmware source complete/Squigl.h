// Squigl.h
#ifndef SQUIGL_H
#define SQUIGL_H

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <u8g2.h>
#include "pico/stdlib.h"
#include "Module.h"
#include "screen.h"
#include "Controls.h"
#include "IO.h"
#include "Clock.h"
#include "utils.h"

class Squigl : public Module {
private:
    static constexpr int MAX_BOUNDARY_POINTS = 1100;

    enum LoopMode {
        LOOP_MANUAL = 0,
        LOOP_RESET = 1,
        LOOP_BOUNCE = 2
    };

    int currentDACValue = 0;
    int targetDACValue = 0;
    double tempDACValue = 0.0;
    float smoothingFactor = 0.0f;
    double dacDitherError = 0.0;

    int currentShapeIndex = 0;
    const uint8_t (*squig)[2] = nullptr;
    int boundaryCount = 0;
    uint8_t squigDistMap[MAX_BOUNDARY_POINTS]{};
    uint16_t squigDistMapHi[MAX_BOUNDARY_POINTS]{};
    int spacing = 0;
    int ptInd = 0;
    int startP = 0;
    int midP = 0;
    int endP = 0;
    int dir = 1;

    float closestDist = 0.0f;
    float farthestDist = 0.0f;
    float curDist = 0.0f;
    float gravityFactor = 0.0f;
    float adjustedDist = 0.0f;

    int usDelay = 1;
    int usCur = 0;
    int sDelayMin = 60;
    int sDelayMax = 3000;
    float squigSpeedPercent = 0.5f;
    int gravAmt = 4;

    int voltAmt = 5;
    float voltFineAmt = 0.0f;

    LoopMode currentLoopMode = LOOP_BOUNCE;
    const char* loopModeNames[3] = {"manual", "reset", "bounce"};
    bool squigOptions = false;
    int optionsInd = 0;
    int optionsSpacing = 10;
    int optionsYoff = 4;
    char speedText[10]{};
    char voltText[10]{};
    char gravText[10]{};
    char voltFineText[10]{};

    bool manualRunning = false;
    bool resetTriggered = false;
    bool dirHoldFlag = false;

    float squigms = 0.0f;
    float squigMoveTick = 0.0f;
    float nudgeTick = 0.0f;
    int scrollDelay = 40;
    int frameCounter = 0;

    bool threshOn = false;
    float threshold = 0.0f;

    void setShape(int index);
    float calculateDistance(int x1, int y1, int x2, int y2);
    void squig_draw_options();
    void squigl_con_options();
    void squigl_con_main();
    void squigl_con_check();
    void updatePtIndWithinBounds();
    long map(long x, long in_min, long in_max, long out_min, long out_max);
    void nudgeCheck();

public:
    Squigl(int hand);
    void initialize() override;
    void run() override;
    void runFast() override;
    void draw() override;
};

#endif // SQUIGL_H
