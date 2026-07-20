#include "Squigl.h"
#include "Clock.h"
#include "squiglShapes.c"

// Constructor
Squigl::Squigl(int hand) : Module(hand), 
    currentDACValue(0),     // Initialize current DAC value
    targetDACValue(0),      // Initialize target DAC value
    smoothingFactor(1.5),   // Set default smoothing factor
    currentShapeIndex(0),   // Initialize shape index as zero
    squig(nullptr),         // Initialize squig pointer to nullptr
    boundaryCount(0),       // Initialize boundary count to zero
    closestDist(0),         // Initialize distance tracking
    farthestDist(0),        // Initialize distance tracking
    manualRunning(false),   // Initialize manual running state
    resetTriggered(false)   // Initialize reset trigger state
{
    // Constructor implementation
    printf("Initializing Squigl module, hand: %d\n", hand);
}


// Override initialize
void Squigl::initialize() {
    // Make sure the shape index is valid
    // CHANGE TO REAL YEARDAY, JUST YEARDAY not -72
   
    currentShapeIndex = yearDay%365;

    if(is_leap_year && yearDay > 59)
    {
        currentShapeIndex-=1;
    }
    
    // Use the first shape by default
    squig = squigShapes[currentShapeIndex];

    // Get proper boundary count from shape length array
    boundaryCount = squigLens[currentShapeIndex];

    // Initialize loop mode to manual
    currentLoopMode = LOOP_BOUNCE;
    manualRunning = false; // Start with manual mode stopped
    
    spacing = boundaryCount/8;
  
    for (int i = 0; i < boundaryCount; i++) {

        float curDist = calculateDistance(squig[i][0], squig[i][1], 64, 32);

        if (curDist > farthestDist) {
            farthestDist = curDist;
        }

        if (curDist < closestDist || closestDist == 0) {
            closestDist = curDist;
        }
    }

    float distRange = farthestDist - closestDist;
    if (distRange <= 0.0f) {
        distRange = 1.0f;
    }

    for (int i = 0; i < boundaryCount; i++) {
        float curDist = calculateDistance(squig[i][0], squig[i][1], 64, 32);
        float norm = (curDist - closestDist) / distRange;
        if (norm < 0.0f) {
            norm = 0.0f;
        } else if (norm > 1.0f) {
            norm = 1.0f;
        }

        squigDistMap[i] = static_cast<uint8_t>((norm * 64.0f) + 0.5f);
        squigDistMapHi[i] = static_cast<uint16_t>((norm * 4095.0f) + 0.5f);
    }


    /*
    if(hand!=10)
    {
        struct repeating_timer timer;
        add_repeating_timer_us(-100, timerCallback, this, &timer);
    }
    */

}

// Method to change the current shape
void Squigl::setShape(int index) {
    // Guard against invalid indices
    if (index < 0 || index >= 24) {
        // Force to a safe value
        index = 0;
    }
    
    // Store the index as an integer
    currentShapeIndex = index;
    
    // Update shape pointer and boundary count - with safety checks
    squig = squigShapes[currentShapeIndex];
    boundaryCount = squigLens[currentShapeIndex];
    
    // Validate boundary count value
    if (boundaryCount <= 0 || boundaryCount > MAX_BOUNDARY_POINTS) {
        // Something is wrong, use a safe default
        printf("Invalid boundary count %d for shape %d, resetting\n", boundaryCount, currentShapeIndex);
        currentShapeIndex = 2;
        squig = squigShapes[2];
        boundaryCount = squigLens[0];
    }
    
    spacing = boundaryCount/8;
    
    // Reset distance measurements for the new shape
    closestDist = 0;
    farthestDist = 0;
    
    // Recalculate distances for new shape
    for (int i = 0; i < boundaryCount; i++) {
        float curDist = calculateDistance(squig[i][0], squig[i][1], 64, 32);

        if (curDist > farthestDist) {
            farthestDist = curDist;
        }

        if (curDist < closestDist || closestDist == 0) {
            closestDist = curDist;
        }
    }

    float distRange = farthestDist - closestDist;
    if (distRange <= 0.0f) {
        distRange = 1.0f;
    }

    for (int i = 0; i < boundaryCount; i++) {
        float curDist = calculateDistance(squig[i][0], squig[i][1], 64, 32);
        float norm = (curDist - closestDist) / distRange;
        if (norm < 0.0f) {
            norm = 0.0f;
        } else if (norm > 1.0f) {
            norm = 1.0f;
        }

        squigDistMap[i] = static_cast<uint8_t>((norm * 64.0f) + 0.5f);
        squigDistMapHi[i] = static_cast<uint16_t>((norm * 4095.0f) + 0.5f);
    }
    
    // Reset position indices to avoid out-of-bounds errors
    ptInd = 0;
    startP = 0;
    midP = boundaryCount / 4;
    endP = midP + (spacing * 2);
}

void Squigl::runFast() {
    IO_check(hand * 2);
    IO_check((hand * 2) + 1);

    if (IO_inStatesMomentary[hand * 2] || IO_inStatesMomentary[(hand * 2) + 1]) {
        resetTriggered = true;
        printf("Reset triggered for hand %d\n", hand);
    }

    // Handle reset trigger (latched above)
    if (resetTriggered) {
        ptInd = startP;
        manualRunning = true; // Start running in manual mode
        resetTriggered = false; // Clear the flag
    }

    // Calculate gravity factor based on gravAmt
    curDist = calculateDistance(squig[ptInd][0], squig[ptInd][1], 64, 32);
    gravityFactor = gravAmt * 0.25f;

    // Apply the gravity factor to curDist
    adjustedDist = pow(curDist / 64.0f, gravityFactor) * 64.0f;

    // Cap the ratio of how intense of a speed increase gravamt can provide
    if(adjustedDist < (curDist/8)) {
        adjustedDist = curDist/8;
    }

    float speedPercent = squigSpeedPercent;
    if (speedPercent < 0.0f) {
        speedPercent = 0.0f;
    } else if (speedPercent > 1.0f) {
        speedPercent = 1.0f;
    }

    const float kBaseDelayScale = 0.25f;       
    const float kMaxSpeedFactor = 256.0f;     
    const float kSlowBand = 0.10f;           
    const float kSlowBandShare = 0.08f;     

    float t = speedPercent;
    float t_mapped;
    if (t <= kSlowBand) {
        float u = (kSlowBand > 0.0f) ? (t / kSlowBand) : 0.0f;
        t_mapped = kSlowBandShare * (u * u);
    } else {
        float u = (t - kSlowBand) / (1.0f - kSlowBand);
        float eased = u * u * (3.0f - 2.0f * u); // smoothstep
        t_mapped = kSlowBandShare + (1.0f - kSlowBandShare) * eased;
    }

    float speedFactor = powf(kMaxSpeedFactor, t_mapped);
    float delayScale = kBaseDelayScale / speedFactor;
    float minDelay = static_cast<float>(sDelayMin) * delayScale;
    float maxDelay = static_cast<float>(sDelayMax) * delayScale;
    float delay = (adjustedDist / 64.0f) * (maxDelay - minDelay) + minDelay;

    usDelay = (delay > 0.0f) ? static_cast<int>(delay + 0.5f) : 0;

    // For manual mode, only process if currently running
    if (currentLoopMode == LOOP_MANUAL && !manualRunning) {
        // In manual mode and not running, don't update position
        // But still output the current DAC value
    } 
    else {
        // Update position based on delay
        usCur++;
        if (usCur > usDelay) {
            ptInd = (ptInd + dir + boundaryCount) % boundaryCount;
            usCur = 0;
        }
    }

    // Apply the selected loop mode behavior
    switch (currentLoopMode) {
        case LOOP_MANUAL:
            // Manual mode - stops at end point until triggered again
            if (ptInd == endP) {
                manualRunning = false; // Stop running when we reach the end
            }
            break;
            
                    case LOOP_RESET:
            // Reset mode - jump back to start
            if (ptInd == endP) {
                ptInd = startP;
                // Ensure we move forward from start point by doing one advance immediately
                // This helps prevent getting stuck at the first point
                usCur = usDelay + 1; // Force an immediate step on the next cycle
            }
            break;
        case LOOP_BOUNCE:
            // Bounce mode - change direction at endpoints
            if (ptInd == endP) {
                dir = -1;
            } else if (ptInd == startP) {
                dir = 1;
            }
            break;
    }

    // Calculate the target DAC value with interpolation to avoid stepped CV at high speed.
    float interpDist = static_cast<float>(squigDistMapHi[ptInd]);
    bool canMove = !(currentLoopMode == LOOP_MANUAL && !manualRunning);
    if (canMove && usDelay > 0) {
        int nextInd = (ptInd + dir + boundaryCount) % boundaryCount;
        float progress = (usDelay > 0 && usCur <= usDelay)
            ? (static_cast<float>(usCur) / static_cast<float>(usDelay))
            : 1.0f;
        float nextDist = static_cast<float>(squigDistMapHi[nextInd]);
        interpDist = interpDist + (nextDist - interpDist) * progress;
    }

    int voltScale = static_cast<int>(4095.0f * ((voltAmt + (voltFineAmt * 10.0f)) / 10.0f));
    targetDACValue = static_cast<int>(((4095.0f - interpDist) / 4095.0f) * voltScale);
    
    // Apply smoothing - move the current value toward the target value
    // The larger the smoothingFactor, the faster the transition
    if (currentDACValue != targetDACValue) {
       

        double diff = static_cast<double>(targetDACValue) - tempDACValue;

        //smoothingFactor=0.001f;
        //smoothinFactor=
        //smoothingFactor=0.00005f;
        //smoothingFactor=0.005f;
        //smoothingFactor=(voltFineAmt);
        double step = diff * smoothingFactor;
        
        
        // Update the current value
        tempDACValue += step;
        //currentDACValue += step;
      
    }
    
    // Write the smoothed value to the DAC
    {
        double shaped = tempDACValue + dacDitherError;
        if (shaped < 0.0) {
            shaped = 0.0;
            dacDitherError = 0.0;
        } else if (shaped > static_cast<double>(voltScale)) {
            shaped = static_cast<double>(voltScale);
            dacDitherError = 0.0;
        }

        int quantized = static_cast<int>(shaped);
        dacDitherError = shaped - static_cast<double>(quantized);
        currentDACValue = quantized;
    }
    
    IO_writeDAC(hand*2, currentDACValue);
    //IO_writeDAC(hand*2+1,(int)(4095-currentDACValue));
   
    int invertDACValue = voltScale - currentDACValue;

    IO_writeDAC(hand*2+1,invertDACValue);
}

// Override draw
void Squigl::draw() {
    squigl_con_check();

    // Existing draw code
    u8g2_ClearBuffer(&u8g2);
    //should be i+=4 for dashed outline
    for (int i = 0; i < boundaryCount; i += 4) {
        u8g2_DrawPixel(&u8g2, squig[i][0], squig[i][1]); 
    }

    u8g2_SetFont(&u8g2, u8g2_font_unifont_t_symbols);
    u8g2_SetFontPosCenter(&u8g2);

    curDist=(float)squigDistMap[ptInd];

    int circSize = map(curDist, 0, 64, 7, 1);

    u8g2_SetDrawColor(&u8g2, 2);
    u8g2_DrawFilledEllipse(&u8g2, squig[ptInd][0], squig[ptInd][1], circSize, circSize, U8G2_DRAW_ALL);
    u8g2_DrawFilledEllipse(&u8g2, 64, 32, 2, 2, U8G2_DRAW_ALL);
    u8g2_SetDrawColor(&u8g2, 1);

    u8g2_DrawFilledEllipse(&u8g2,
            (squig[ptInd][0] + 64) / 2,
            (squig[ptInd][1] + 32) / 2,
            1,
            1,
            U8G2_DRAW_ALL);

    u8g2_DrawFilledEllipse(&u8g2,
            (squig[ptInd][0] * 3 + 64) / 4,
            (squig[ptInd][1] * 3 + 32) / 4,
            1,
            1,
            U8G2_DRAW_ALL);

    for (int i = startP; i < (startP + (spacing * 2)); i++) {
        int ind = i % boundaryCount;
        u8g2_DrawEllipse(&u8g2, squig[ind][0], squig[ind][1], 1, 1, U8G2_DRAW_ALL);
    }

    u8g2_DrawGlyph(&u8g2, squig[startP][0],squig[startP][1],9673);

    u8g2_SetDrawColor(&u8g2, 1);

    //old location of this 
    //updatePtIndWithinBounds();

    /*
    //Thresh draw
    //KEEEP JUST COMMENTED
    if(threshOn)
    {
        u8g2_SetDrawColor(&u8g2, 2);
        u8g2_DrawFilledEllipse(&u8g2,64,32,threshold,threshold,U8G2_DRAW_ALL);
        u8g2_SetDrawColor(&u8g2, 1);
    }

    u8g2_DrawEllipse(&u8g2,64,32,threshold,threshold,U8G2_DRAW_ALL);
    */

    //FINAL STEPS
    if (squigOptions) {
        squig_draw_options();
    }
    u8g2_UpdateDisplay(&u8g2);
}



// Implementation of other Squigl methods

float Squigl::calculateDistance(int x1, int y1, int x2, int y2) {
    //return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
    return sqrt(((x2-x1)*(x2-x1))+((y2-y1)*(y2-y1)));
}

void Squigl::squig_draw_options() {
    int cx = 64;
    int cy = 16 + 16;

    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_SetFontPosTop(&u8g2);

    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_DrawBox(&u8g2, 18, (1 + optionsInd) * optionsSpacing + optionsYoff, 85, 10);
    u8g2_SetDrawColor(&u8g2, 1);

    u8g2_SetFont(&u8g2, u8g2_font_unifont_t_symbols);
    u8g2_DrawGlyph(&u8g2, cx - 11, (1 + optionsInd) * optionsSpacing + optionsYoff - 1, 9668);
    u8g2_DrawGlyph(&u8g2, cx + 32, (1 + optionsInd) * optionsSpacing + optionsYoff - 1, 9658);

    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    u8g2_DrawStr(&u8g2, 19, 1 * optionsSpacing + optionsYoff, "speed");
    u8g2_DrawStr(&u8g2, 19, 2 * optionsSpacing + optionsYoff, " grav");
    u8g2_DrawStr(&u8g2, 19, 3 * optionsSpacing + optionsYoff, " loop");
    u8g2_DrawStr(&u8g2, 19, 4 * optionsSpacing + optionsYoff, " volt");
    u8g2_DrawStr(&u8g2, 19, 5 * optionsSpacing + optionsYoff, "vfine");

    sprintf(speedText, "%3d%%", (int)(squigSpeedPercent * 100));
    sprintf(voltText, "%3dV", voltAmt);
    if (gravAmt == 0) {
        sprintf(gravText, " 0g");
    } else {
        sprintf(gravText, "%2dg", gravAmt);
    }
    sprintf(voltFineText, "0.%.0gV", voltFineAmt * 100);

    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    u8g2_DrawStr(&u8g2, cx - 2, 1 * optionsSpacing + optionsYoff, speedText);
    u8g2_DrawStr(&u8g2, cx - 2, 2 * optionsSpacing + optionsYoff, gravText);
    u8g2_DrawStr(&u8g2, cx - 2, 3 * optionsSpacing + optionsYoff, loopModeNames[currentLoopMode]);
    u8g2_DrawStr(&u8g2, cx - 2, 4 * optionsSpacing + optionsYoff, voltText);
    u8g2_DrawStr(&u8g2, cx - 2, 5 * optionsSpacing + optionsYoff, voltFineText);

    u8g2_SetDrawColor(&u8g2, 2);
    u8g2_DrawBox(&u8g2, 18, (1 + optionsInd) * optionsSpacing + optionsYoff, 85, 10);
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_SetDrawColor(&u8g2, 1);
}

void Squigl::squigl_con_options() {
    int val = con_joyMomentary();

    if (val == 2 && optionsInd < 5) {
        optionsInd++;
    }

    if (val == 1 && optionsInd > 0) {
        optionsInd--;
    }

    int valac = con_joyAccelerate();

    switch (optionsInd) {
        case 0:
            if (valac == 3 && squigSpeedPercent > 0.0f) {
                squigSpeedPercent -= 0.01f;
                if (squigSpeedPercent < 0.0f) {
                    squigSpeedPercent = 0.0f;
                }
                //smoothingFactor=pow(usDelay, 0.866442006 - 0.2455553161 * log(usDelay));
                printf("delay %d",usDelay);
                printf("smoothing %f \n",smoothingFactor);
            }
            if (valac == 4 && squigSpeedPercent < 1.0f) {
                //smoothingFactor=pow(usDelay, 0.866442006 - 0.2455553161 * log(usDelay));
                squigSpeedPercent += 0.01f;
                if (squigSpeedPercent > 1.0f) {
                    squigSpeedPercent = 1.0f;
                }
                printf("delay %d",usDelay);
                printf("smoothing %f \n",smoothingFactor);
            }
            break;
        case 1:
            if (valac == 3 && gravAmt > 0) {
                gravAmt--;
            }
            if (valac == 4 && gravAmt < 10) {
                gravAmt++;
            }
            break;
        case 2: // Loop mode selection
            if (valac == 3) {
                // Cycle backward through modes
                currentLoopMode = static_cast<LoopMode>((currentLoopMode + 2) % 3);
            }
            if (valac == 4) {
                // Cycle forward through modes
                currentLoopMode = static_cast<LoopMode>((currentLoopMode + 1) % 3);
            }
            break;
        case 3:
            if (valac == 3 && voltAmt > 0) {
                voltAmt -= 1;
            }
            if (valac == 4 && voltAmt < 10) {
                voltAmt += 1;
            }
            break;
        case 4:
            if (valac == 3 && voltFineAmt >= 0.-.08f) {
                voltFineAmt -= 0.01f;
            }
            if (valac == 4 && voltFineAmt <= 0.08f) {
                voltFineAmt += 0.01f;
                printf("vfamt %f\n",voltFineAmt);
            }
            break;
    }
}

void Squigl::squigl_con_main() {
    const int gap = 1;

    if (con_joyContin() != 0) {
        if (!dirHoldFlag) {
            squigMoveTick = squigms + scrollDelay;
        }
        dirHoldFlag = true;

        if (squigms > squigMoveTick) {
            squigMoveTick = squigms + scrollDelay;

            if (con_joyContin() == 2) {
                midP += gap;
            }

            if (con_joyContin() == 1) {
                midP -= gap;
            }

            if (con_joyContin() == 4) {
                    if ((spacing*2) < (boundaryCount - 5)) {
                    spacing += gap;
                }
            }

            if (con_joyContin() == 3) {
                if (spacing > 3) {
                    spacing -= gap;
                }
            }
        }
    } else {
        dirHoldFlag = false;
    }
}
void Squigl::squigl_con_check() {
    if (button_momentary(1) == true) {
        squigOptions = !squigOptions;
        printf("menu\n");
    }

    if (squigOptions) {
        squigl_con_options();
    } else {
        squigl_con_main();
    }
}

void Squigl::updatePtIndWithinBounds() {
    if (startP <= endP) {
        if (ptInd < startP || ptInd > endP) {
            ptInd = (dir == 1) ? startP : endP;
        }
    } else {
        if (ptInd > endP && ptInd < startP) {
            ptInd = (dir == 1) ? startP : endP;
        }
    }
}

/*
bool Squigl::timerCallback(struct repeating_timer *t) {
    Squigl* squigl = static_cast<Squigl*>(t->user_data);
    return squigl->updateSquigl();
}
*/

long Squigl::map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void Squigl:: nudgeCheck()
{

    int pinIndex=(hand*2)+1;
    IO_check(pinIndex);

    //gpio_pull_down(pinIndex);

    if(IO_inStates[pinIndex]==false)
    {
        printf("IN\n");
        if(squigms>nudgeTick)
        {
            nudgeTick = squigms + scrollDelay;
            midP--;
        }
    }
}

void Squigl::run() {

    {
        const float kSettle = 0.02f;
        const float kMinSmoothing = 0.00005f;
        const float kMaxSmoothing = 0.5f;
        float delay = (usDelay > 0) ? static_cast<float>(usDelay) : 1.0f;
        float alpha = 1.0f - static_cast<float>(pow(kSettle, 1.0f / delay));

        if (alpha < kMinSmoothing) {
            alpha = kMinSmoothing;
        } else if (alpha > kMaxSmoothing) {
            alpha = kMaxSmoothing;
        }

        smoothingFactor = alpha;
    }
            
    uint64_t us_since_boot = to_us_since_boot(get_absolute_time());
    squigms = us_since_boot / 1000.0f;

    midP = (midP % boundaryCount + boundaryCount) % boundaryCount;
    startP = ((midP) % boundaryCount + boundaryCount) % boundaryCount;
    endP = ((midP + (spacing * 2)) % boundaryCount + boundaryCount) % boundaryCount;

    updatePtIndWithinBounds();

    frameCounter++;

    if(curDist < threshold)
    {
        threshOn=true;
    } else {
        threshOn=false;
    }

    /*
    //old testing stuff
    if(frameCounter%30==0)
    {
      
        //IO_writeDAC(15000);

        //printf("gravfactor %f \n",gravityFactor);
        //printf("threshon %b \n",threshOn);
    }
    */
}
