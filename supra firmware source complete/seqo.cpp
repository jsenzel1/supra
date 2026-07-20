// seqo.cpp
#include "seqo.h"

// Constructor
Seqo::Seqo(int hand)
    : Module(hand),
      autoClock(false),
      followLeftClock(false),
      followLeftClockPrev(false),
      clkDelay(140),
      drawSeqo(true),
      //boxesLeftPool{5, 3, 4, 7, 5, 6},
      boxesLeftPool{99, 99, 99, 99, 99, 99},
      boxesLeftPoolLen(6),
      //seqPool{4, 5, 6, 7, 8},
      seqPool{5, 8, 12, 7, 6, 13,10,9},
	      seqPoolLen(8),
	      boxesLeft(4),
	      inTick(clkDelay, 50),
	      followTick(clkDelay, 50),
	      clockDelay(40),
	      ms(0),
	      inFlag(false),
	      followWaitingForEdge(false),
	      followMasterPrevHigh(false),
	      followMasterRiseCount(0),
	      followMasterBpmSnapshot(0),
	      followMasterMultSnapshot(0),
	      followLocalMultSnapshot(0),
	      boxSpacing(10),
	      cursorX(0),
	      cursorY(20),
      numBoxes1(9),
      numBoxes2(5),
      xTemp(0),
      playheadCount(0),
      seqLengths{5, 6, 7},
      seqSelect(0),
      seqSpacing(22),
      key(0),
      heldBoxType(plain),
      boxTypeSelectArr{skip2, skip3, skip4, skip5},
      boxTypeSelectLen(4),
      boxTypeSelectInd(0),
      boxTypeToday(0),
      // Menu system variables (added from Morsed)
      showMenu(false),
      menuIndex(0),
      bpm(120),
      mult(1)
{
    // Initialize boxAnimBank
    //boxAnimBank[0][0] = 9633; // none
    //boxAnimBank[1][0] = 9632; // plain
    boxAnimBank[0][0] = 9676; // none
    boxAnimBank[1][0] = 9679; // plain
    boxAnimBank[2][0] = 9675; // skip2
    boxAnimBank[2][1] = 9681;
    boxAnimBank[3][0] = 9675; // skip3
    boxAnimBank[3][1] = 9681;
    boxAnimBank[3][2] = 9685;
    boxAnimBank[3][3] = 9679;
    boxAnimBank[3][4] = 9678;
    boxAnimBank[3][5] = 9677;
    boxAnimBank[4][0] = 9675; // skip4
    boxAnimBank[4][1] = 9684;
    boxAnimBank[4][2] = 9681;
    boxAnimBank[4][3] = 9679;
    boxAnimBank[4][4] = 9678;
    boxAnimBank[4][5] = 9677;
    boxAnimBank[5][0] = 9675; // skip5
    boxAnimBank[5][1] = 9681;
    boxAnimBank[5][2] = 9685;
    boxAnimBank[5][3] = 9679;
    boxAnimBank[5][4] = 9678;
    boxAnimBank[5][5] = 9677;

    // Initialize boxIcons
    boxIcons[0] = 0;
    //boxIcons[1] = 9632;
    boxIcons[1] = 9679;
    boxIcons[2] = 9681;
    boxIcons[3] = 9685;
    boxIcons[4] = 9677;
    //boxIcons[4] = 9679;
    boxIcons[5] = 9678;
}

// Shuffle Box Types
void Seqo::shuffleBoxTypes(boxType *array, int size) {
    for (int i = size - 1; i > 1; i--) {
        int j = 1 + rand() % (i - 1);
        boxType temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

// Initialize Playhead
void Seqo::playhead_init(Playhead &ph) {
    ph.pos = 0;
    ph.on = false;
    ph.flag = false;
    ph.id = playheadCount;
    playheadCount++;
}

// Tick Skip - FIXED
void Seqo::playhead_tick_skip(Playhead* ph, Box* curBox) {
    if(curBox->skipInd >= curBox->skipMax - 1) {
        // Time to trigger - reset counter and output high
        curBox->skipInd = 0;
        curBox->animPos = 0;
        curBox->glyph = boxAnimBank[curBox->type][0]; // Show the base glyph when triggering
        IO_outStates[ph->id+(hand*2)] = true;
        curBox->on = true;
    } else {
        // Still skipping - increment counter and show skip glyph
        curBox->on = false;
        curBox->skipInd++;
        // For skip blocks, show the skip glyph (index 1) when skipping
        curBox->animPos = 1; // Always show the semi-circle glyph when skipping
        curBox->glyph = boxAnimBank[curBox->type][curBox->skipInd]; // Semi-circle glyph
    }
}

void Seqo::playhead_tick_plain(Playhead* ph, Box* curBox) {
    // Calculate the output index accounting for hand offset
    int outputIndex = ph->id + (hand * 2);
    
    // Set output state based on box state
    if(curBox->on) {
        IO_outStates[outputIndex] = true;
    } else {
        IO_outStates[outputIndex] = false;
    }
}

// Tick Playhead - advancing on clock high
void Seqo::playhead_tick(Playhead* ph) {
    // When clock is high
    if(IO_inStates[hand*2] == 1) {
        // Only trigger once per clock pulse
        if(!ph->flag) {
            // Advance playhead position FIRST
            ph->pos += 1;
            if(ph->pos > seqLengths[ph->id] - 1) {
                ph->pos = 0;
            }
            
            // Now process the box at the NEW position
            Box *curBox = &metaBox.all[ph->id][ph->pos];
            
            // Process current box (this activates outputs if needed)
            switch(curBox->type) {
                case plain:
                    // For plain boxes, directly set output based on box state
                    if(curBox->on) {
                        IO_outStates[ph->id+(hand*2)] = true;
                    }
                    break;
                case skip2:
                case skip3:
                case skip4:
                case skip5:
                    // For skip boxes, follow the same skip logic
                    playhead_tick_skip(ph, curBox);
                    break;
                default:
                    // No output for empty boxes
                    break;
            }
            
            // Mark that this clock pulse has been processed
            ph->flag = true;
        }
    }
    // When clock is low
    else {
        // Reset outputs for non-skip boxes (skip boxes manage their own timing)
        Box *curBox = &metaBox.all[ph->id][ph->pos];
        
        IO_outStates[ph->id+(hand*2)] = false;

        // Just reset the flag, no position change
        ph->flag = false;
    }
}

// FIXED playhead_reset
void Seqo::playhead_reset(Playhead* ph) {
    ph->flag = true;
    ph->pos = 0;
    
    // Immediately process the box at position 0
    Box *curBox = &metaBox.all[ph->id][ph->pos];
    
    // Clear the current output state
    IO_outStates[ph->id+(hand*2)] = false;
    
    // Process the box at position 0 based on its type
    switch(curBox->type) {
        case plain:
            if(curBox->on) {
                IO_outStates[ph->id+(hand*2)] = true;
            }
            break;
        case skip2:
        case skip3:
        case skip4:
        case skip5:
            if(curBox->on) {
                // Reset skip sequence for skip boxes
                curBox->skipInd = 0;
                curBox->animPos = 0;
                curBox->glyph = boxAnimBank[curBox->type][0]; // Show base glyph
                // On reset, skip boxes should trigger immediately
                IO_outStates[ph->id+(hand*2)] = true;
            }
            break;
        default:
            // Empty box, ensure output is off
            break;
    }
    
    // Make sure to update the outputs
    IO_write(hand);
}

// Draw Playhead
void Seqo::playhead_draw(Playhead* ph) {
    int curGlyph = 176;
    Box *curBox = &metaBox.all[ph->id][ph->pos];

    if(curBox->on) {
        curGlyph=9733;
    } else {
        curGlyph = 176;
    }
    if(curGlyph==9733)
    {
        u8g2_SetDrawColor(&u8g2,2);
        u8g2_DrawBox(&u8g2, curBox->x, curBox->y-5, 8,6);
        //9674
        //u8g2_SetDrawColor(&u8g2,1);
    }

    if(curGlyph == 9733 &&
       (curBox->type == skip2 || curBox->type == skip3 || curBox->type == skip4 || curBox->type == skip5))
    {
        u8g2_DrawGlyph(&u8g2, curBox->x, curBox->y, 9679);

        //from old system where you would have a limited amount of boxes
        //char buffer[10];
        //snprintf(buffer, 10, "%d", boxesLeft);
        //u8g2_DrawStr(&u8g2, 75, 15, buffer);
        
        u8g2_DrawGlyph(&u8g2, curBox->x, curBox->y - 7, curGlyph);
    } else {
        u8g2_DrawGlyph(&u8g2, curBox->x, curBox->y - 5, curGlyph);
    }
}

// Initialize Boxes - FIXED
void Seqo::boxes_init() {
    int xoff=-1;
    for(int i = 0; i < seqLengths[0]; i++) {
        metaBox.boxes1[i].y = 33;
        metaBox.boxes1[i].x = i * boxSpacing+xoff;
        metaBox.boxes1[i].on = false;
        metaBox.boxes1[i].type = none;
        metaBox.boxes1[i].animPos = 0;
        metaBox.boxes1[i].glyph = boxAnimBank[metaBox.boxes1[i].type][0];
        metaBox.boxes1[i].skipMax = 2; // Default, will be set properly when box type is assigned
        metaBox.boxes1[i].skipInd = 0; // Initialize skipInd
    }

    for(int i = 0; i < seqLengths[1]; i++) {
        metaBox.boxes2[i].y = seqSpacing + 33;
        metaBox.boxes2[i].x = i * boxSpacing+xoff;
        metaBox.boxes2[i].on = false;
        metaBox.boxes2[i].type = none;
        metaBox.boxes2[i].animPos = 0;
        metaBox.boxes2[i].glyph = boxAnimBank[metaBox.boxes2[i].type][0];
        metaBox.boxes2[i].skipMax = 2; // Default, will be set properly when box type is assigned
        metaBox.boxes2[i].skipInd = 0; // Initialize skipInd
    }
}

// Draw Boxes
void Seqo::drawBoxes(Box *boxes, int length) {
    for(int i = 0; i < length; i++) {
        Box *curBox = &boxes[i];
        u8g2_DrawGlyph(&u8g2, curBox->x, curBox->y, curBox->glyph);
    }
}

// Draw All Boxes
void Seqo::boxes_draw() {
    u8g2_SetFont(&u8g2, u8g2_font_unifont_t_symbols);
    drawBoxes(metaBox.boxes1, seqLengths[0]);
    drawBoxes(metaBox.boxes2, seqLengths[1]);
}

// Draw Cursor
void Seqo::cursor_draw() {
    cursorY = 20 + (seqSpacing * seqSelect);
    //underline
    u8g2_DrawGlyph(&u8g2, (boxSpacing * cursorX)-1, cursorY + 14, 95);
}

// Menu Controls - Added from Morsed
void Seqo::menu_controls() {
    int joyValue = con_joyAccelerate();
    bool followOptionVisible = (hand == 1) && IO_selfClockEnabledByHand[0];
    int maxMenuIndex = followOptionVisible ? 3 : 2;
    if (menuIndex < 0) menuIndex = 0;
    if (menuIndex > maxMenuIndex) menuIndex = maxMenuIndex;
    
    // Menu navigation
    if (joyValue == 1 && menuIndex > 0) {
        // Up - move to previous option
        menuIndex--;
    } else if (joyValue == 2 && menuIndex < maxMenuIndex) {
        // Down - move to next option
        menuIndex++;
    }
    
    // Option value change
    if (joyValue == 3 || joyValue == 4 || joyValue == 5) {
        if (followOptionVisible && menuIndex == 0) {
            followLeftClock = !followLeftClock;
	        } else {
	            int effectiveIndex = followOptionVisible ? (menuIndex - 1) : menuIndex;
	            if (!followLeftClock || effectiveIndex == 2) {
	                // Left or Right - toggle option value
	                switch (effectiveIndex) {
	                    case 0: // Self clock
	                        autoClock = !autoClock;
	                        break;
                    case 1: // BPM
                        if (joyValue == 4 && bpm < 300) {
                            bpm++;
                            inTick = Interval(60000/(bpm*mult), 50);
                        } else if (joyValue == 3 && bpm > 60) {
                            bpm--;
                            inTick = Interval(60000/(bpm*mult), 50);
                        } 
                        break;
                    case 2: // Mult
                        if (joyValue == 3 && mult > 1) {
                            mult = mult/2;
                            inTick = Interval(60000/(bpm*mult), 50);
                        } else if (joyValue == 4 && mult < 16) {
                            mult *= 2;
                            inTick = Interval(60000/(bpm*mult), 50);
                        } 
                        break;
                }
            }
        }
    }
    
    // Exit menu if center button pressed
    if (button_momentary(1) == true) {
        showMenu = false;
    }
}

// Draw Menu - Added from Morsed
void Seqo::draw_menu() {
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_DrawBox(&u8g2, 20, 5, 88, 54);
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_DrawFrame(&u8g2, 20, 5, 88, 54);
    
    // Menu options
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    bool followOptionVisible = (hand == 1) && IO_selfClockEnabledByHand[0];
    int yPos = followOptionVisible ? 16 : 18;
    int optionSpacing = followOptionVisible ? 10 : 12;
    int xPos = 26;
    
    int line = 0;

    if (followOptionVisible) {
        u8g2_DrawStr(&u8g2, xPos, yPos + optionSpacing * line, "follow LH:");
        u8g2_DrawStr(&u8g2, xPos+65, yPos + optionSpacing * line, followLeftClock ? "ON" : "OFF");
        line++;
    }

    int selfLineY = yPos + optionSpacing * line;
    u8g2_DrawStr(&u8g2, xPos, selfLineY, "self clk:");
    u8g2_DrawStr(&u8g2, xPos+65, selfLineY, autoClock ? "ON" : "OFF");
    line++;

    int bpmLineY = yPos + optionSpacing * line;
    u8g2_DrawStr(&u8g2, xPos, bpmLineY, "BPM:");
    sprintf(bpmText, "%3d", bpm);
    u8g2_DrawStr(&u8g2, xPos+30, bpmLineY, bpmText);
    line++;

    int multLineY = yPos + optionSpacing * line;
    u8g2_DrawStr(&u8g2, xPos, multLineY, "mult:    X");
    sprintf(multText, "%3d", mult);
    u8g2_DrawStr(&u8g2, xPos+35, multLineY, multText);

	    if (followOptionVisible && followLeftClock) {
	        int strikeYOff = 4;
	        u8g2_DrawHLine(&u8g2, xPos, selfLineY - strikeYOff, 78);
	        u8g2_DrawHLine(&u8g2, xPos, bpmLineY - strikeYOff, 78);
	    }
    
    // Highlight the selected option
    u8g2_SetDrawColor(&u8g2, 2);
    u8g2_DrawBox(&u8g2, 25, yPos - 8 + (menuIndex * optionSpacing), 80, 10);
}

// Handle Controls - FIXED
void Seqo::controls_tick() {
    // Check if menu is active, and if so, use menu controls instead
    if (showMenu) {
        menu_controls();
        return;
    }

    if(button_momentary(1)) {
        // Toggle menu if button 1 is pressed
        showMenu = !showMenu;
        menuIndex = 0; // Reset menu selection when opening
        return;
    }

    if (button_momentary(2)==true) {
        printf("box change\n");        
        if(heldBoxType==plain) {
            heldBoxType=boxTypeSelectArr[boxTypeToday];
        } else {
            heldBoxType=plain;
        }
    }

    // Hold-to-scroll (accelerated repeats) like Morsed; keep center press momentary for box place/remove.
    if (con_joyMomentary() == 5) {
        // Box placement logic
        Box *curBox = &metaBox.all[seqSelect][cursorX];

        if(!curBox->on && curBox->skipInd==0 ) {
            if(boxesLeft > 0) {
                curBox->type = heldBoxType;
                // Set skipMax based on the box type
                switch(heldBoxType) {
                    case skip2: curBox->skipMax = 2; break;
                    case skip3: curBox->skipMax = 3; break;
                    case skip4: curBox->skipMax = 4; break;
                    case skip5: curBox->skipMax = 5; break;
                    default: curBox->skipMax = 1; break;
                }
                curBox->skipInd = 0; // Reset skip index
                curBox->animPos = 0; // Reset animation position
                curBox->glyph = boxAnimBank[curBox->type][0];
                curBox->on = true;
                boxesLeft--;

                // Adjust y position for skip boxes
                boxType t = curBox->type;
                if(t == skip2 || t == skip3 || t == skip4 || t == skip5) {
                    curBox->y = (seqSelect * seqSpacing) + 33 + 2;
                } else {
                    curBox->y = (seqSelect * seqSpacing) + 33;
                }
            }
        } else {
            // Remove box
            boxType t = curBox->type;
            if(t == skip2 || t == skip3 || t == skip4 || t == skip5) {
                curBox->y = (seqSelect * seqSpacing) + 33; 
            }

            curBox->type = none;
            curBox->glyph = boxAnimBank[curBox->type][0];
            curBox->animPos = 0;
            curBox->skipInd = 0; // Reset skip index
            curBox->on = false;
            boxesLeft++;
        }
        return;
    }

    key = con_joyAccelerate();

    if(key == 2) {
        seqSelect++;
        if(seqSelect > 1) {
            seqSelect = 1;
        }
        if(cursorX > seqLengths[seqSelect] - 1) {
            cursorX = seqLengths[seqSelect] - 1;
        }
    }

    if(key == 1) {
        seqSelect--;
        if(seqSelect < 0) {
            seqSelect = 0;
        }
        if(cursorX > seqLengths[seqSelect] - 1) {
            cursorX = seqLengths[seqSelect] - 1;
        }
    }

    if(key == 3) {
        if(cursorX > 0) {
            cursorX--;
        }
    }

    if(key == 4) {
        if(cursorX < seqLengths[seqSelect] - 1) {
            cursorX++;
        }
    }
}

// Draw Method
void Seqo::draw() {

    u8g2_SetDrawColor(&u8g2, 1);
    controls_tick();

    u8g2_ClearBuffer(&u8g2);

    u8g2_SetFontPosBaseline(&u8g2);
    u8g2_SetFontMode(&u8g2, 1);
    u8g2_SetFont(&u8g2, u8g2_font_t0_15b_mr);
    u8g2_SetFont(&u8g2, u8g2_font_unifont_t_symbols);

    playhead_draw(&head1);
    playhead_draw(&head2);

    boxes_draw();
    cursor_draw();

    if(head1.pos == 0 && head2.pos == 0) {
        u8g2_SetDrawColor(&u8g2,2);
        u8g2_DrawRBox(&u8g2,-2,14,11,47,4);
        u8g2_SetDrawColor(&u8g2,1);
        //u8g2_DrawGlyphX2(&u8g2, 100, 20, 9672);
    }

    if(IO_inStates[hand*2]) {
        u8g2_DrawGlyph(&u8g2, 3, 10, 9655);
    }

    /*
    //boxes left
    char buffer[10];
    snprintf(buffer, 10, "%d", boxesLeft);
    u8g2_DrawStr(&u8g2, 103, 13, buffer);

    u8g2_SetDrawColor(&u8g2,2);
    u8g2_DrawFrame(&u8g2,100,1,14,14);
    u8g2_SetDrawColor(&u8g2,1);
    */

    //draw box icon
    u8g2_DrawGlyph(&u8g2, 117, 12, boxIcons[heldBoxType]);


    //text
    char buffer2[30];
    u8g2_SetFont(&u8g2,u8g2_font_luBIS08_tr);
    snprintf(buffer2, 30, "\'%d vs. %d\' ", seqLengths[0],seqLengths[1]);

    u8g2_DrawStr(&u8g2, 24, 11, buffer2);

    // Draw menu if toggled on
    if (showMenu) {
        draw_menu();
    }

    u8g2_UpdateDisplay(&u8g2);
}

// Initialize Seqo
void Seqo::initialize() {

    inTick = Interval(60000/(bpm*mult), 50);
    followTick = Interval(60000/(bpm*mult), 50);

    playheadCount = 0;

    //this line gives 2 different setups 1 per hand 
    //depending on which hand its in
    //srand(daySeed+(hand*1000));
    srand(daySeed);

    // Shuffle box types
    boxTypeToday = rand() % 4;

    // Initialize boxesLeft from pool
    int rInd = rand() % boxesLeftPoolLen;
    boxesLeft = boxesLeftPool[rInd];

    // Initialize seqLengths from pool
    for(int i = 0; i < MAX_SEQ_LENGTHS; i++) {
        rInd = rand() % seqPoolLen;
        seqLengths[i] = seqPool[rInd];
    }

    boxes_init();

    playhead_init(head1);
    playhead_init(head2);
    head2.id = 1;

    // Initialize menu-related variables
    showMenu = false;
    menuIndex = 0;
    bpm = 120;
    mult = 1;
    followLeftClock = false;
    followLeftClockPrev = false;
    followWaitingForEdge = false;
    followMasterPrevHigh = false;
    followMasterRiseCount = 0;
    followMasterBpmSnapshot = 0;
    followMasterMultSnapshot = 0;
    followLocalMultSnapshot = 0;

    //display_sequence();
    con_init();
}

// Resume Seqo
void Seqo::resume() {
    //from an old system that included a resume method when hand is refocused
    /*
    if(autoClock) {
        IO_inStates[hand*2] = inTick.run(true);
    }
    printf("seqo resumed\n");
    */
}

void Seqo::update_vals() {
    //UNUSED
}

// Run Method
void Seqo::runFast() {

    IO_check(hand*2);
    IO_check((hand*2)+1);

    
    if(IO_inStatesMomentary[(hand*2)+1]==true)
    {
        printf("RESET\n");
        playhead_reset(&head1);
        playhead_reset(&head2);
    }

    if ((hand == 1) && followLeftClock && !IO_selfClockEnabledByHand[0]) {
        followLeftClock = false;
    }

    bool followActive = (hand == 1) && followLeftClock && IO_selfClockEnabledByHand[0];
    if (followActive) {
        int masterBpm = IO_selfClockBpmByHand[0];
        int masterMult = IO_selfClockMultByHand[0];
        if (masterBpm <= 0) masterBpm = 120;
        if (masterMult <= 0) masterMult = 1;

        bool leftClkHigh = IO_inStates[0];
        bool leftRising = leftClkHigh && !followMasterPrevHigh;
        followMasterPrevHigh = leftClkHigh;

        if (!followLeftClockPrev) {
            followWaitingForEdge = true;
            followMasterRiseCount = 0;
            followMasterBpmSnapshot = 0;
            followMasterMultSnapshot = 0;
            followLocalMultSnapshot = 0;
        }

        if (followMasterMultSnapshot != 0 && masterMult != followMasterMultSnapshot) {
            followWaitingForEdge = true;
        }

        if (followWaitingForEdge) {
            if (!leftRising) {
                IO_inStates[hand * 2] = false;
            } else {
                followWaitingForEdge = false;
                followMasterRiseCount = 0;
                followMasterBpmSnapshot = masterBpm;
                followMasterMultSnapshot = masterMult;
                followLocalMultSnapshot = mult;
                followTick = Interval(60000 / (masterBpm * mult), 50);
            }
        }

        if (!followWaitingForEdge) {
            bool resetFollowTick = false;

            if (leftRising) {
                followMasterRiseCount++;
                if (followMasterRiseCount >= masterMult) {
                    followMasterRiseCount = 0;
                    resetFollowTick = true;

                    // Only retune on beat boundaries to keep phase locked.
                    if (masterBpm != followMasterBpmSnapshot ||
                        masterMult != followMasterMultSnapshot ||
                        mult != followLocalMultSnapshot) {
                        followMasterBpmSnapshot = masterBpm;
                        followMasterMultSnapshot = masterMult;
                        followLocalMultSnapshot = mult;
                        followTick = Interval(60000 / (masterBpm * mult), 50);
                    }
                }
            }

            IO_inStates[hand * 2] = followTick.run(resetFollowTick);
        }
    } else if(autoClock) {
        IO_selfClockEnabledByHand[hand] = true;
        IO_selfClockBpmByHand[hand] = bpm;
        IO_selfClockMultByHand[hand] = mult;
        IO_inStates[hand*2] = inTick.run();
        followWaitingForEdge = false;
        followMasterPrevHigh = false;
        followMasterRiseCount = 0;
        followMasterBpmSnapshot = 0;
        followMasterMultSnapshot = 0;
        followLocalMultSnapshot = 0;
    } else {
        followWaitingForEdge = false;
        followMasterPrevHigh = false;
    }
    followLeftClockPrev = followLeftClock;

    playhead_tick(&head1);
    playhead_tick(&head2);

    IO_write(hand);
}

void Seqo::run(){
    //printf("print4: %d \n",sharedPulse);
}
