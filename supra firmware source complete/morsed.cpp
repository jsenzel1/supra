#include "Morsed.h"
#include "Clock.h"
#include "morsedwords.c"

//YEAR FORMAT IS 25 not 2025

// Constructor
Morsed::Morsed(int hand) : Module(hand),
    drawMorsed(true),
    placing(false),
    clkDelay(600),
    autoClock(false),
    inTick(clkDelay, 50),
    clockDelay(400),
    pointSpaceFlip(false),
    moveSpeedDefault(2),
    moveSpeed(moveSpeedDefault),
    ms(0),
    nextBlip(0),
    blip(false),
    blipHold(false),
    blipFlag(false),
    letInd(0),
    inFlag(false),
    lArrowPos(1),
    rArrowPos(7),
    loopStart(2),
    loopEnd(5),
    paraX(50),
    paraPos(0),
    showMenu(false),
    menuIndex(0),
    splitOutput(true),
    totalValidPoints(0),
    letterSpaces(true),
    resetOffsetCount(0),
    currentLetterIndex(0)
{
    // Initialize morse_char and morse_key arrays
    // These are already defined in the original file
}

void Morsed::initialize() {

    msTillNewWords = ((2-(hour%3))*3600000)+
                     ((60-minute)*60000)+
                     ((60-second)*1000);

    followLeftClock = false;
    followLeftClockPrev = false;


    con_init();
    ph_init();

    string_create();
    string_sep();
    make_points();
    
    // Set initial max value to match the actual content length
    if (totalValidPoints > 0) {
        ph.max = totalValidPoints;
    }
}

void Morsed::ph_init() {
    ph.x = 0;
    ph.y = 0;
    ph.id = 1;
    ph.restCount = 0;
    ph.flag = false;
    ph.loc = 0;
    ph.curParentInd = 0;
    ph.max = 14;
    ph.rest = false;
    curPoint = points[0];
    currentLetterIndex = 0; // Initialize the current letter index
}

// Modified to load only 2 words
void Morsed::string_create()
{
    // REPLACE MODULO WITH LENGTH OF POEM
    // mess with this so poem starts on or around launch date 
    
    //int dayOff = (((year-20)*365)+yearDay) % 551;
   

    int section=hour/3;    

    int dayOff  = ((year-25)*(365*24)) + (yearDay*24) + (section*3);

    dayOff += resetOffsetCount;

    dayOff=dayOff%551;
    // Clear the para array first
    memset(para, 0, sizeof(para));
    
    // Only load 2 words at a time
    int wordcount=3;
    for(int i = dayOff; i < dayOff + wordcount && i < 551; i++)
    {
        if (i == dayOff) {
            strncpy(para, words[i], sizeof(para) - 2);
        } else {
            strncat(para, " ", sizeof(para) - strlen(para) - 1);  // Add space between words
            strncat(para, words[i], sizeof(para) - strlen(para) - 2);
        }
    }

    para[sizeof(para) - 1] = '\0';
}

// Modified to add blank spaces between letters
void Morsed::string_sep() {
    // Clear the myKeys array first
    memset(myKeys, 0, sizeof(myKeys));
    
    int keyIndex = 0;
    int charIndex = 0; // Track actual character position in para
    
    // Process each character in the para string
    for(int i = 0; i < strlen(para) && keyIndex < 32; i++) {
        // Get the Morse code for the current character
        const char* tempKey = charToKey(para[i]);
        
        if (tempKey != nullptr) {
            // Only add actual characters to Morse codes (excluding spaces)
            if (!isspace(para[i])) {
                // Copy the Morse code for this character
                strncpy(myKeys[keyIndex], tempKey, 15);
                myKeys[keyIndex][15] = '\0';
                
                // Store the original character index for this morse code
                keyCharIndices[keyIndex] = charIndex;
                
                keyIndex++;
                
                // Add a letter space after each letter (but not after spaces)
                
                if (letterSpaces && keyIndex < 32 && i < strlen(para) - 1) {
                    strncpy(myKeys[keyIndex], "s", 15); // 's' for letter space
                    myKeys[keyIndex][15] = '\0';
                    
                    // Letter spaces have the same character index as the letter they follow
                    keyCharIndices[keyIndex] = charIndex;
                    
                    keyIndex++;
                }
            } else {
                // For word spaces, add a special marker but don't count as a morse code key
                strncpy(myKeys[keyIndex], "w", 15); // 'w' for word space
                myKeys[keyIndex][15] = '\0';
                
                // Word spaces are treated specially
                keyCharIndices[keyIndex] = charIndex;
                
                keyIndex++;
            }
            
            // Increment character index for all characters including spaces
            charIndex++;
        }
    }
    
    // Calculate total Morse code length that will be represented
    totalMorseLength = 0;
    for (int i = 0; i < keyIndex; i++) {
        totalMorseLength += strlen(myKeys[i]);
    }
}

char* Morsed::charToKey(char inChar) {
    // If it's a space, we'll handle it specially in string_sep
    if(isspace(inChar)) {
        return (char*)"x"; // This will be replaced with 'w' in string_sep
    }
    
    // Convert to uppercase for consistency
    inChar = toupper(inChar);
    
    // Look up the character in morse_char array
    for(int i = 0; i < 28; i++) {
        if(inChar == morse_char[i][0]) {
            return morse_key[i];
        }
    }
    
    return nullptr;
}

void Morsed::make_points() {
    int keyCharInd = 0;
    int keyInd = 0;
    char* tempMyKey = myKeys[keyInd];
    
    char curChar;
    int tempX = 0;
    int tempY = 0;
    
    // Clear the points array first
    memset(points, 0, sizeof(points));
    
    int pointIndex = 0;
    
    // First, determine how many valid keys we have
    int validKeyCount = 0;
    while (validKeyCount < 32 && myKeys[validKeyCount][0] != '\0') {
        validKeyCount++;
    }
    
    // Now populate the points array only for valid content
    for(int i = 0; i < 64 && pointIndex < 64; i++) {
        tempX = i % 16;
        tempY = (i / 16);
        
        points[pointIndex].x = tempX;
        points[pointIndex].y = tempY;
        
        // If we've gone through all keys, stop adding new points
        if (keyInd >= validKeyCount) {
            break;
        }
        
        curChar = tempMyKey[keyCharInd];
        
        if(curChar == '\0') {
            keyCharInd = 0;
            keyInd++;
            
            // If we've gone through all keys, stop adding new points
            if (keyInd >= validKeyCount) {
                break;
            }
            
            tempMyKey = myKeys[keyInd];
            
            // Only flip the color for actual characters, not for spaces
            if (tempMyKey[0] != 'w' && tempMyKey[0] != 's') {
                pointSpaceFlip = !pointSpaceFlip;
            }
        }
        
        points[pointIndex].flip = pointSpaceFlip;
        
        curChar = tempMyKey[keyCharInd];
        keyCharInd++;
        
        // Store the character index for correct text highlighting
        points[pointIndex].parentInd = keyCharIndices[keyInd];
        
        if(curChar == '.') {
            points[pointIndex].type = 0;
            points[pointIndex].pChar = '.';
            pointIndex++; // Only increment after valid point is added
        } else if(curChar == '-') {
            points[pointIndex].type = 1;
            points[pointIndex].pChar = '-';
            pointIndex++; // Only increment after valid point is added
        } else if(curChar == 's') {
            // This is a letter space
            points[pointIndex].type = 3; // Type 3 for spaces
            points[pointIndex].pChar = 's';
            points[pointIndex].flip = false; // Make spaces unhighlighted
            pointIndex++; // Only increment after valid point is added
        } else if(curChar == 'w') {
            // This is a word space
            points[pointIndex].type = 4; // Type 4 for word spaces
            points[pointIndex].pChar = 'w';
            points[pointIndex].flip = false; // Make spaces unhighlighted
            pointIndex++; // Only increment after valid point is added
        } else {
            // Skip any invalid characters
            continue;
        }
    }
    
    // Store the total number of valid points
    totalValidPoints = pointIndex;
    
    // Update ph.max to match the actual content length
    if (totalValidPoints > 0 && ph.max > totalValidPoints) {
        ph.max = totalValidPoints;
    }
}

void Morsed::rotatePoints(int direction) {
    if (totalValidPoints <= 1) {
        return; // Nothing to rotate
    }
    
    // Create a temporary array to store only the content properties
    struct {
        int type;
        int parentInd;
        bool flip;
        char pChar;
    } tempContent[NUM_POINTS];
    
    // Copy the content properties to the temporary array
    for (int i = 0; i < totalValidPoints; i++) {
        tempContent[i].type = points[i].type;
        tempContent[i].parentInd = points[i].parentInd;
        tempContent[i].flip = points[i].flip;
        tempContent[i].pChar = points[i].pChar;
    }
    
    if (direction == 1) { // Rotate forward
        // For each point, take content from the previous point
        for (int i = 0; i < totalValidPoints; i++) {
            int prevIndex = (i - 1 + totalValidPoints) % totalValidPoints;
            points[i].type = tempContent[prevIndex].type;
            points[i].parentInd = tempContent[prevIndex].parentInd;
            points[i].flip = tempContent[prevIndex].flip;
            points[i].pChar = tempContent[prevIndex].pChar;
        }
    } else { // Rotate backward
        // For each point, take content from the next point
        for (int i = 0; i < totalValidPoints; i++) {
            int nextIndex = (i + 1) % totalValidPoints;
            points[i].type = tempContent[nextIndex].type;
            points[i].parentInd = tempContent[nextIndex].parentInd;
            points[i].flip = tempContent[nextIndex].flip;
            points[i].pChar = tempContent[nextIndex].pChar;
        }
    }
    
    // Make sure we update the current point for drawing
    if (ph.loc < totalValidPoints) {
        curPoint = points[ph.loc];
    }
}

void Morsed::ph_step() {
    if(ph.rest) {
        ph.rest = false;
        ph.loc++;
        ph.x++;
    }
    
    // Make sure we don't exceed the total valid points
    if (ph.loc >= totalValidPoints) {
        ph.loc = 0;
        ph.x = 0;
        ph.y = 0;
    }
    
    //if (ph.x+(ph.y*16) >= ph.max-1) {
    if (ph.x+(ph.y*16) >= ph.max-1) {
        ph.x = 0;
        //ph.y = (ph.y + 1) % 4;
        ph.y=0;
        ph.loc = 0;
    }
    
    // Make sure we don't access invalid points
    if (ph.loc < totalValidPoints) {
        curPoint = points[ph.loc];
    } else {
        // Reset to a safe position
        ph.loc = 0;
        curPoint = points[0];
    }
    
    if(ph.x > ph.max) {
        ph.x = ph.max - 1;
        ph.loc = ph.max - 1;
    }
    
    if(ph.x == 16) {
        ph.x = 0;
        ph.y++;
    }
    
    // Update the currentLetterIndex based on the current point's parentInd
    // Only update for actual characters (types 0 and 1), not spaces
    if (curPoint.type < 2) {
        currentLetterIndex = curPoint.parentInd;
    }
    
    ph.curParentInd = curPoint.parentInd;
    
    if(curPoint.type == 0) { // Dot
        if (splitOutput) {
            IO_outStates[hand * 2] = true;     // Output dot on first output
            IO_outStates[(hand * 2) + 1] = false; // Ensure dash output is off
        } else {
            IO_outStates[hand * 2] = true;     // Standard output for combo mode
            IO_outStates[(hand * 2) + 1] = true;
        }
    }
    
    if(curPoint.type == 1) { // Dash
        if(ph.restCount == 3) {
            ph.restCount = 0;
        }
            
        ph.restCount++;
        if(splitOutput)
        {
            IO_outStates[(hand * 2) + 1] = true; // Output dash on second output
            IO_outStates[hand * 2] = false;      // Ensure dot output is off
        } else {
            IO_outStates[hand * 2] = true;     // Standard output for combo mode
            IO_outStates[(hand * 2) + 1] = true;
        }                
    }
    
    // For spaces (types 3 and 4), no output
    if(curPoint.type >= 3) {
        IO_outStates[hand * 2] = false;
        IO_outStates[(hand * 2) + 1] = false;
    }
}

void Morsed::draw_points() {
    u8g2_SetFont(&u8g2, u8g2_font_t0_11_te);
    u8g2_SetFontPosBaseline(&u8g2);
    
    // Only draw valid points
    for(int i = 0; i < totalValidPoints; i++) {
        // Skip drawing spaces (types 3 and 4)
        if (points[i].type >= 3) {
            continue;
        }
        
        // Always reset draw color before drawing each point
        u8g2_SetDrawColor(&u8g2, points[i].flip ? 1 : 0);
        u8g2_DrawBox(&u8g2, ((points[i].x) * 8), 2 + ((points[i].y + 4) * 8), 8, 5);
        
        // Ensure proper contrast for text
        u8g2_SetDrawColor(&u8g2, points[i].flip ? 0 : 1);
        u8g2_DrawStr(&u8g2, (points[i].x) * 8, ((points[i].y + 5) * 8 + (points[i].type * 3) - 2), ditRef[points[i].type]);
    }
}

void Morsed::draw_ph() {
    // Ensure the playhead position is valid
    if (ph.max > totalValidPoints) {
        ph.max = totalValidPoints;
    }
    
    if (ph.loc >= totalValidPoints) {
        ph.x = 0;
        ph.y = 0;
        ph.loc = 0;
    } 
    
    if (ph.x > ph.max) {
        ph.x = ph.max - 1;
        ph.loc = ph.max - 1;
    }
    
    // Update playhead state based on input
    if (IO_inStates[hand*2] == 1) {
        if (!ph.flag) {
            ph_step();
            ph.flag = true;
        }
    } else {
        // FIXED VERSION - Handle dash length consistently in both split and combo modes
        if (ph.loc < totalValidPoints && points[ph.loc].type == 1) {
            // For dashes, use the same timing logic regardless of mode
            if (ph.restCount == 2) {
                if (splitOutput) {
                    IO_outStates[(hand * 2) + 1] = false; // Turn off dash output
                } else {
                    IO_outStates[hand * 2] = false; // Turn off combo output
                    IO_outStates[(hand * 2) + 1] = false;
                }
                ph.rest = true;
            }
        } else {
            // For dots and spaces
            IO_outStates[hand * 2] = false;
            IO_outStates[(hand * 2) + 1] = false;
            ph.rest = true;
        }
        ph.flag = false;
    }
    
    // Make sure we don't try to go past the end of valid content
    int ind = (ph.max > 0) ? ph.max - 1 : 0;
    if (ind >= totalValidPoints) {
        ind = (totalValidPoints > 0) ? totalValidPoints - 1 : 0;
    }
    
    // Calculate the coordinates
    endX = (ind % 16) * 8;
    endY = 42 + ((ind / 16) * 8);
    
    // Make sure curPoint is valid
    if (ph.loc < totalValidPoints) {
        curPoint = points[ph.loc];
    } else if (totalValidPoints > 0) {
        ph.loc = 0;
        curPoint = points[0];
    }
    
    u8g2_SetFontMode(&u8g2, 1);  // Set transparent mode
    u8g2_SetDrawColor(&u8g2, 2); // Use XOR mode (2) consistently

    // Only draw highlight for non-space points
    if (curPoint.type < 3) {
        // Draw the current point highlight
        u8g2_DrawBox(&u8g2, (curPoint.x * 8), 1 + ((curPoint.y + 4) * 8), 6, 7);
    }

    if (!placing) {
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_DrawBox(&u8g2, endX, endY-10, 8, 8);
        u8g2_SetDrawColor(&u8g2, 0);
        u8g2_DrawGlyph(&u8g2, endX, endY, 9664);
    }
    if (placing) {
        // In placing mode, fix the offset issue
        int spacing = 8;
        int toff = 10;
        
        u8g2_SetDrawColor(&u8g2, 0);

        // Draw the arrow control boxes
        u8g2_DrawBox(&u8g2, endX-spacing, endY-toff, 8, 8); // left arrow
        u8g2_DrawBox(&u8g2, endX+spacing, endY-toff, 8, 8); // right arrow
        u8g2_DrawBox(&u8g2, endX, endY-spacing-toff, 8, 8); // up arrow
        u8g2_DrawBox(&u8g2, endX, endY+spacing-toff, 8, 8); // down arrow
        
        u8g2_SetDrawColor(&u8g2, 1);
        // Draw the arrow glyphs
        u8g2_DrawGlyph(&u8g2, endX-spacing, endY, 9664); // left arrow
        u8g2_DrawGlyph(&u8g2, endX+spacing, endY, 9654); // right arrow
        u8g2_DrawGlyph(&u8g2, endX, endY-spacing, 9650); // up arrow
        u8g2_DrawGlyph(&u8g2, endX, endY+spacing, 9660); // down arrow
    }
}

void Morsed::draw_text() {
    u8g2_SetFontMode(&u8g2, 1);
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_DrawBox(&u8g2, 0, 0, 128, 32);
    u8g2_SetDrawColor(&u8g2, 1);
    
    // Calculate display start position - this centers the current letter
    int displayPos = 0;
    
    // If we're at the start of the text, just show from the beginning
    if (currentLetterIndex < 1) {
        displayPos = 0;
    } else {
        // Position the text so the current letter is always at position 0
        displayPos = currentLetterIndex;
    }
    
    // Draw the letter at a fixed position
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_SetFont(&u8g2, u8g2_font_inb16_mf);
    u8g2_DrawStr(&u8g2, 0, 22, &para[displayPos]);
    
    // Use XOR mode for the cursor box at fixed position
    u8g2_SetFontMode(&u8g2, 1);  // Ensure transparent mode
    u8g2_SetDrawColor(&u8g2, 2); // XOR mode
    u8g2_DrawBox(&u8g2, 0, 6, 14, 17);
    
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_SetFont(&u8g2, u8g2_font_unifont_t_symbols);
}

void Morsed::draw_menu() {
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_DrawBox(&u8g2, 20, 5, 92, 62);
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_DrawFrame(&u8g2, 20, 5, 92, 62);
    
    // Menu options
    bool followOptionVisible = (hand == 1) && IO_selfClockEnabledByHand[0];
    int yPos = followOptionVisible ? 13 : 15;
    int optionSpacing = followOptionVisible ? 10 : 12;
    int xPos=26;

    int line = 0;
    int followLineY = 0;
    if (followOptionVisible) {
        followLineY = yPos + optionSpacing * line;
        u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
        u8g2_DrawStr(&u8g2, xPos, followLineY, "follow LH:");
        u8g2_DrawStr(&u8g2, xPos+65, followLineY, followLeftClock ? "ON" : "OFF");
        line++;
    }
    
    // Self clock option
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    int selfLineY = yPos + optionSpacing * line;
    u8g2_DrawStr(&u8g2, xPos, selfLineY, "self clk:");
    u8g2_DrawStr(&u8g2, xPos+65, selfLineY, autoClock ? "ON" : "OFF");
    line++;
   
    int bpmLineY = yPos + optionSpacing * line;
    int multLineY = yPos + optionSpacing * (line + 1);
    bool localClockFields = autoClock && !followLeftClock;

    if(localClockFields)
    { 
        u8g2_DrawStr(&u8g2, xPos, bpmLineY, "clk BPM:");
        sprintf(bpmText, "%3d", bpm);
        u8g2_DrawStr(&u8g2, xPos+60, bpmLineY, bpmText);

        u8g2_DrawStr(&u8g2, xPos, multLineY, "clk mult:   X");
        sprintf(multText, "%3d", mult);
        u8g2_DrawStr(&u8g2, xPos+52, multLineY, multText);
    } else {

        u8g2_DrawStr(&u8g2, xPos, bpmLineY, "---------");

        u8g2_DrawStr(&u8g2, xPos, multLineY, "---------");
    }

    if (followOptionVisible && followLeftClock) {
        int strikeYOff = 4;
        u8g2_DrawHLine(&u8g2, xPos, selfLineY - strikeYOff, 86);
        u8g2_DrawHLine(&u8g2, xPos, bpmLineY - strikeYOff, 86);
        u8g2_DrawHLine(&u8g2, xPos, multLineY - strikeYOff, 86);
    }
    
    int outsLineY = yPos + optionSpacing * (line + 2);
    u8g2_DrawStr(&u8g2, 23, outsLineY, "Outs:");
    u8g2_DrawStr(&u8g2, xPos+37, outsLineY, splitOutput ? "SPLIT" : "COMBO");

    int ltrLineY = yPos + optionSpacing * (line + 3);
    u8g2_DrawStr(&u8g2, 23, ltrLineY, "Ltr spaces:");
    u8g2_DrawStr(&u8g2, xPos+65, ltrLineY, letterSpaces ? "YES" : "NO");
    
    // Highlight the selected option
    u8g2_SetDrawColor(&u8g2, 2);
    u8g2_DrawBox(&u8g2, 21, yPos - 8 + (menuIndex * optionSpacing), 86, 10);
}

void Morsed::menu_controls() {
    int joyValue = con_joyAccelerate();
    bool followOptionVisible = (hand == 1) && IO_selfClockEnabledByHand[0];
    int maxMenuIndex = followOptionVisible ? 5 : 4;
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
    if (joyValue == 3 || joyValue == 4 || joyValue==5) {
        if (followOptionVisible && menuIndex == 0) {
            followLeftClock = !followLeftClock;
        } else {
            int effectiveIndex = followOptionVisible ? (menuIndex - 1) : menuIndex;
            if (!(followLeftClock && effectiveIndex <= 2)) {
                // Left or Right - toggle option value
                switch (effectiveIndex) {
                    case 0: // Self clock
                        autoClock = !autoClock;
                        break;
                    case 1: // BPM
                        if (joyValue == 4 && bpm < 300) {   // allow raising up to 300
                            bpm++;
                            inTick = Interval(60000/(bpm*mult), 50);
                        } else if (joyValue == 3 && bpm > 60) {  // allow lowering down to 60
                            bpm--;
                            inTick = Interval(60000/(bpm*mult), 50);
                        }
                        break;
                    case 2: // Mult
                        if (joyValue == 3 && mult > 1) {
                            mult=mult/2;
                            inTick = Interval(60000/(bpm*mult), 50);
                        } else if (joyValue == 4 && mult<16) {
                            mult*=2;
                            inTick = Interval(60000/(bpm*mult), 50);
                        } 
                        break;             
                    case 3: // Output mode
                        splitOutput = !splitOutput;

                    case 4: // Letter spaces
                        letterSpaces = !letterSpaces;
                        
                        string_create();
                        string_sep();
                        make_points();

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

void Morsed::conCheck() {
    if (showMenu) {
        menu_controls();
        return;
    }

    int val=con_joyMomentary();

    if(val==5)
    {
        placing=!placing;
    }

    if(placing)
    {
        switch(val) 
        {
            case 1:
                if(ph.max-16 > 0)
                {
                    ph.max-=16; 
                }
                break;
            case 2:
                if(ph.max+16 < totalValidPoints) { // Constrain to valid points
                    ph.max+=16;
                }
                break;
            case 3:
                if(ph.max > 2)
                {
                    ph.max--;
                }
                break;
            case 4:
                if(ph.max < totalValidPoints+1) { // Constrain to valid points
                    ph.max++;
                }
                break;
        }
    }

    if(!placing)
    {
        switch(con_joyAccelerate()) 
        {
            case 1:
                // glyph_y=(glyph_y-moveSpeed);
                break;
            case 2:
                // glyph_y+=moveSpeed;
                break;
            case 3:
                rotatePoints(-1);
                break;
            case 4:
                rotatePoints(1);
                break;
        }
    }
    
    // Check for button press to toggle menu
    if (button_momentary(1) == true) {
        showMenu = !showMenu;
        menuIndex = 0; // Reset menu selection when opening
    }
}

void Morsed::draw() {
    //IO_check(hand*2);
    
    conCheck();
    u8g2_ClearBuffer(&u8g2);
    
    // Ensure entire display is cleared and reset properly
    u8g2_SetFontMode(&u8g2, 1);  // Set transparent mode
    
    // Execute drawing functions in correct order
    draw_points();
    draw_text();
    draw_ph();
    
    // Draw menu if toggled on
    if (showMenu) {
        draw_menu();
    }
    
    u8g2_UpdateDisplay(&u8g2);
}

void Morsed::runFast() {
    IO_check(hand*2);
    IO_check((hand*2)+1);

    if ((hand == 1) && followLeftClock && !IO_selfClockEnabledByHand[0]) {
        followLeftClock = false;
    }

    bool followActive = (hand == 1) && followLeftClock && IO_selfClockEnabledByHand[0];
    if (followActive) {
        bool leftClkHigh = IO_inStates[0];
        if (!followLeftClockPrev && followLeftClock && leftClkHigh) {
            ph.flag = true;
        }
        IO_inStates[hand*2] = leftClkHigh;
    } else if(autoClock) {
        IO_selfClockEnabledByHand[hand] = true;
        IO_inStates[hand*2] = inTick.run();
    }
    followLeftClockPrev = followLeftClock;
    
    // Update outputs without redrawing the screen
    if(IO_inStates[hand*2] == 1) {
        if(!ph.flag) {
            ph_step();
            ph.flag = true;
        }
    } else {
        // FIXED VERSION - Handle dash length consistently in both split and combo modes
        if (ph.loc < totalValidPoints && points[ph.loc].type == 1) {
            // For dashes, use the same timing logic regardless of mode
            if (ph.restCount == 2) {
                if (splitOutput) {
                    IO_outStates[(hand * 2) + 1] = false; // Turn off dash output
                } else {
                    IO_outStates[hand * 2] = false; // Turn off combo output
                    IO_outStates[(hand * 2) + 1] = false;
                }
                ph.rest = true;
            }
        } else {
            // For dots and spaces
            IO_outStates[hand * 2] = false;
            IO_outStates[(hand * 2) + 1] = false;
            ph.rest = true;
        }
        ph.flag = false;
    }
    
    if(IO_inStatesMomentary[(hand*2)+1])
    {
        if(inUsedForReset)
        {
            ph.x=0;
            ph.y=0;
            ph.loc=0;
            ph.flag=false;
        } else {
            rotatePoints(1);
        }
    }

    IO_write(hand);
}

void Morsed::run() {
    ms = to_ms_since_boot(get_absolute_time());

    if(ms > msTillNewWords)
    {
        //NEW WORDS
        //TODO
        //SHOW TIME GRAPIC 
        //test the time resetting thing

        resetOffsetCount++;        
        string_create();
        string_sep();
        make_points();

        msTillNewWords=ms+10800000;
    }
}
