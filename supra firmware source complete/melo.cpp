#include "melo.h"

const char* NOTE_NAMES[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
static inline int mod12(int x){int r=x%12;return r<0?r+12:r;}

int decimal_to_pitch_class_array(int dec,int* out){
    if(dec<0||dec>4095)return 0;
    int n=0;for(int i=0;i<12;++i)if(dec&(1<<i))out[n++]=i;return n;
}

Melo::Melo(int hand):Module(hand),autoClock(false),clkDelay(140),ms(0){}

void Melo::loadScale(int scaleDecimal){
    int pcs[12]; int sz=decimal_to_pitch_class_array(scaleDecimal,pcs);
    currentLibSize=(sz<libSize)?sz:libSize;
    for(int i=0;i<sz&&i<libSize;++i)possiblePitches[i]=pcs[i];

    library[0].val=-1; library[0].glyph='x'; library[0].on=true;

    for(int i=0;i<libSize;++i){
        int p=(i<currentLibSize)?possiblePitches[i]:0;
        library[i+1].val=p;                 library[i+1].glyph=pitchGlyphs[mod12(p)];
        library[i+1].on=(i<currentLibSize); library[i+1].row=0;
        p+=12;
        library[i+libSize+1].val=p;         library[i+libSize+1].glyph=pitchGlyphs[mod12(p)];
        library[i+libSize+1].on=(i<currentLibSize); library[i+libSize+1].row=1;
    }
    int tshift=transposeSemis+transposeOct*12;
    if(tshift)for(int i=1;i<17;++i)library[i].val+=tshift;
}

void Melo::initScaleTime()
{
    if(hour<12)
    {
        isDaytime=true;
    } else {
        isDaytime=false;
    }


    if ( (month == 3 && day >= 20) || month == 4 || month == 5 ||
         (month == 6 && day <= 20) ) {
        selectedScale=springScales[weekday][isDaytime];
        printf("SPRING\n");
    }
    else if ( (month == 6 && day >= 21) || month == 7 || month == 8 ||
              (month == 9 && day <= 21) ) {
        selectedScale=summerScales[weekday][isDaytime];
        printf("SUMMEr\n");
    }
    else if ( (month == 9 && day >= 22) || month == 10 || month == 11 ||
              (month == 12 && day <= 20) ) {
        selectedScale=fallScales[weekday][isDaytime];
        printf("FALL\n");
        printf("month %d\n",month);
    }

    else { // must be winter
        // (Dec 21‑31, Jan, Feb, Mar 1‑19)
        selectedScale=winterScales[weekday][isDaytime];
        printf("WINTER\n");
    }

    printf("weedkay: %d\n",weekday);
    printf("isDaytime: %d\n",isDaytime);
}

void Melo::initialize(){
    con_init(); u8g2_SetFontPosCenter(&u8g2);
    for(int i=0;i<32;++i){sequence[i].glyph=9633;sequence[i].on=false;}
    library[0].x=0;library[0].y=16;

    for(int i=0;i<libSize;++i){
        library[i+1].x=(i+1)*xSpacing; library[i+1].y=16;
        library[i+libSize+1].x=(i+1)*xSpacing; library[i+libSize+1].y=6;
    }

    initScaleTime();
    loadScale(selectedScale);
}

void Melo::recalcSeqPositions(){
    for(int i=0;i<seqLen;++i){
        int col=i/8;
        sequence[i].x=(i*xSpacingSeq)-((8*xSpacingSeq)*col);
        sequence[i].y=29 + (ySpacing*col);
    }
}

void Melo::shiftLibrary(int semis){
    for(int i=1;i<=libSize;++i){if(!library[i].on)continue;
        library[i].val+=semis; library[i].glyph=pitchGlyphs[mod12(library[i].val)];}
    for(int i=libSize+1;i<17;++i){
        if(!library[i-libSize].on){library[i].on=false;continue;}
        library[i].val=library[i-libSize].val+12;
        library[i].glyph=pitchGlyphs[mod12(library[i].val)];
    }
}
void Melo::shiftSequence(int semis){
    for(int i=0;i<seqLen;++i)if(sequence[i].on)sequence[i].val+=semis;
    if(flashingNote)flashingNoteData.val+=semis;
    shiftLibrary(semis);
}

void Melo::startNoteEdit(int index) {
    if (index < 0 || index >= seqLen) return;
    
    // Store the entire original sequence for reference
    originalSeqLen = seqLen;
    for (int i = 0; i < seqLen; i++) {
        originalSequence[i] = sequence[i];
    }
    
    flashingNote = true;
    flashingNoteOrigIndex = index;
    flashingNoteData = sequence[index];
    flashTimer = to_ms_since_boot(get_absolute_time());
    flashState = true;
    
    // Mark the cursor position
    seqCursor = index;
    
    // Rearrange sequence for initial state
    rearrangeSequenceForCursor();
}

void Melo::finishNoteEdit() {
    if (!flashingNote) return;
    
    // Place the flashing note at current cursor position
    // At this point the sequence should already be correctly arranged
    sequence[seqCursor] = flashingNoteData;
    sequence[seqCursor].on = true;
    
    flashingNote = false;
    flashingNoteOrigIndex = -1;
    
    // Recalculate positions for clean display
    recalcSeqPositions();
}

void Melo::updateFlashingState() {
    if (!flashingNote) return;
    
    long currentTime = to_ms_since_boot(get_absolute_time());
    if (currentTime - flashTimer >= kFlashPeriodMs) {
        flashState = !flashState;
        flashTimer = currentTime;
    }
}

void Melo::rearrangeSequenceForCursor() {
    if (!flashingNote) return;
    
    // We're completely rebuilding the sequence based on original sequence
    // and the current cursor position
    
    // First, restore all notes from the original sequence except the edited note
    int destIdx = 0;
    for (int i = 0; i < originalSeqLen; i++) {
        if (i == flashingNoteOrigIndex) continue; // Skip the edited note
        
        if (destIdx == seqCursor) destIdx++; // Leave room for cursor position
        
        if (destIdx < seqLen) {
            sequence[destIdx] = originalSequence[i];
            // Ensure note properties are maintained properly
            sequence[destIdx].on = originalSequence[i].on;
            sequence[destIdx].glyph = originalSequence[i].glyph;
            sequence[destIdx].val = originalSequence[i].val;
            destIdx++;
        }
    }
    
    // Ensure the cursor position is properly initialized but empty
    // (we'll draw the flashing note here separately)
    if (seqCursor < seqLen) {
        sequence[seqCursor].on = false;
    }
    
    // Recalculate all visual positions
    recalcSeqPositions();
}

void Melo::drawSequence(){
    for(int i=0;i<seqLen;++i){
        // At cursor position, we'll draw the flashing note instead
        if(flashingNote && i == seqCursor) {
            // Draw an empty box to indicate position when not flashing
            if (!flashState) {
                u8g2_DrawGlyph(&u8g2,sequence[i].x,sequence[i].y,32);
            }
            continue;
        }
        
        if(!sequence[i].on){
            // Draw empty box
            u8g2_DrawGlyph(&u8g2,sequence[i].x,sequence[i].y,9633);
            continue;
        }
        
        // Draw the actual note
        int idx=mod12(sequence[i].val);
        u8g2_SetFont(&u8g2,u8g2_font_5x8_tf);
        u8g2_SetFont(&u8g2,u8g2_font_spleen5x8_mf);
        u8g2_DrawStr(&u8g2,sequence[i].x,sequence[i].y,NOTE_NAMES[idx]);
        u8g2_SetFont(&u8g2,u8g2_font_unifont_t_symbols);
    }
    
    // Draw the flashing note at cursor position when it should be visible
    if(flashingNote && flashState && seqCursor < seqLen) {
        int idx = mod12(flashingNoteData.val);
        u8g2_SetDrawColor(&u8g2,2);  // Inverted color for highlighting
        u8g2_SetFont(&u8g2,u8g2_font_5x8_tf);
        u8g2_SetFont(&u8g2,u8g2_font_spleen5x8_mf);
        u8g2_DrawStr(&u8g2,sequence[seqCursor].x,sequence[seqCursor].y,NOTE_NAMES[idx]);
        u8g2_SetDrawColor(&u8g2,1);  // Restore normal color
        u8g2_SetFont(&u8g2,u8g2_font_unifont_t_symbols);
    }
}

void Melo::drawLibrary(){
    u8g2_SetFont(&u8g2,u8g2_font_5x7_tf);
    u8g2_DrawStr(&u8g2,library[0].x,library[0].y,"x");
    for(int i=1;i<=currentLibSize;++i)
        u8g2_DrawStr(&u8g2,library[i].x,library[i].y,NOTE_NAMES[mod12(library[i].val)]);
    for(int i=libSize+1;i<=libSize+currentLibSize;++i){
        char buf[4];snprintf(buf,sizeof(buf),"%s+",NOTE_NAMES[mod12(library[i].val)]);
        u8g2_DrawStr(&u8g2,library[i].x,library[i].y,buf);}
    u8g2_SetFont(&u8g2,u8g2_font_unifont_t_symbols);
}

void Melo::drawCursors(){
    if(editingLib){
        int ix=(libRow==0)?(libCursor+1):(libCursor+1+libSize);
        if(libCursor==-1)u8g2_DrawHLine(&u8g2,library[0].x,library[0].y+3,8);
        else if(libCursor<currentLibSize)
        {
            u8g2_DrawHLine(&u8g2,library[ix].x,library[ix].y+3,10);
            u8g2_DrawHLine(&u8g2,library[ix].x,library[ix].y+4,10);
        }
    }else if(seqCursor<seqLen){
        // Always draw cursor
        u8g2_DrawHLine(&u8g2,sequence[seqCursor].x,sequence[seqCursor].y+3,10);
        u8g2_DrawHLine(&u8g2,sequence[seqCursor].x,sequence[seqCursor].y+4,10);
    }
}

void Melo::drawPlayhead(){
    
    u8g2_SetFontPosCenter(&u8g2);
    if(playheadPos<seqLen){
        u8g2_SetDrawColor(&u8g2,2);
        u8g2_DrawGlyphX2(&u8g2,sequence[playheadPos].x-4,sequence[playheadPos].y,9679);
        //u8g2_DrawBox(&u8g2,sequence[playheadPos].x-1,sequence[playheadPos].y-5,10,8);
        u8g2_SetDrawColor(&u8g2,1);
    }
}

void Melo::draw_menu(){
    u8g2_SetFontPosCenter(&u8g2);
    u8g2_SetDrawColor(&u8g2,0);u8g2_DrawBox(&u8g2,14,5,100,32);
    u8g2_SetDrawColor(&u8g2,1);u8g2_DrawFrame(&u8g2,14,5,100,32);
    const int x=20,y0=16,line=12;
    u8g2_SetFont(&u8g2,u8g2_font_6x10_tf);
    char b1[24];snprintf(b1,sizeof(b1),"transpose: %+d",transposeSemis);
    char b2[24];snprintf(b2,sizeof(b2),"octave: %+d",transposeOct);
    u8g2_DrawStr(&u8g2,x,y0,b1);u8g2_DrawStr(&u8g2,x,y0+line,b2);
    u8g2_SetDrawColor(&u8g2,2);u8g2_DrawBox(&u8g2,16,y0-8+menuIndex*line,96,10);
    u8g2_SetDrawColor(&u8g2,1);
}

void Melo::menu_controls(){
    int joy=con_joyAccelerate();
    if(joy==1&&menuIndex>0)menuIndex--;
    if(joy==2&&menuIndex<1)menuIndex++;
    if(joy==3||joy==4){
        int dir=(joy==3)?-1:1;
        if(menuIndex==0){
            int ns=transposeSemis+dir,no=transposeOct;
            if(ns<-11){ns+=12;--no;} if(ns>11){ns-=12;++no;}
            if(no<kOctMin||no>kOctMax)return;
            transposeSemis=ns;transposeOct=no;shiftSequence(dir);
        }else{
            int no=transposeOct+dir;if(no<kOctMin||no>kOctMax)return;
            transposeOct=no;shiftSequence(dir*12);
        }
    }
    if(button_momentary(1))showMenu=false;
}

void Melo::controls_tick(){
    if(showMenu){menu_controls();return;}
    int key=con_joyAccelerate();

    if(button_momentary(2) && !flashingNote)editingLib=!editingLib;

    if(button_momentary(3) && !flashingNote){
        currentScaleIndex=(currentScaleIndex+1)%3;
        ////loadScale(scales[currentScaleIndex]);
        if(libCursor>=currentLibSize)libCursor=currentLibSize-1;
    }

    /* cursor moves */
    if(editingLib){
        if(key==2&&libRow>0)libRow--;
        if(key==1&&libRow<1)libRow++;
        if(key==3&&libCursor>-1)libCursor--;
        if(key==4&&libCursor<currentLibSize-1)libCursor++;
    }else{
        int oldCursor = seqCursor;
        
        if(key==1&&seqCursor-8>-1)seqCursor-=8;
        if(key==2&&seqCursor+8<seqLen)seqCursor+=8;

        // Allow cursor movement in edit mode or normal mode
        if(key==3&&seqCursor>0) {
            seqCursor--;
        }
        if(key==4&&seqCursor<seqLen-1) {
            seqCursor++;
        }
        
        // If cursor moved while in edit mode, rearrange the sequence
        if(flashingNote && oldCursor != seqCursor) {
            rearrangeSequenceForCursor();
        }
    }

    /* add / delete */
    if(key==5&&editingLib && !flashingNote){
        if(libCursor==-1&&seqLen>0){sequence[--seqLen].on=false;}
        else if(libCursor!=-1&&seqLen<32&&libCursor<currentLibSize){
            int src=(libRow==0)?(libCursor+1):(libCursor+1+libSize);
            sequence[seqLen]=library[src]; sequence[seqLen].on=true; ++seqLen;
            recalcSeqPositions();
        }
    }

    /* edit note in sequence-edit mode */
    if(!editingLib && key==5){
        if(!flashingNote && seqCursor<seqLen && sequence[seqCursor].on){
            // Start edit mode (pick up)
            startNoteEdit(seqCursor);
        }else if(flashingNote){
            // Finish edit mode (place)
            finishNoteEdit();
        }
    }

    yCollumns=seqLen/8;
    if(button_momentary(1) && !flashingNote){showMenu=true;menuIndex=0;}
    
    // Update flashing state for visual feedback
    updateFlashingState();
}

void Melo::processTrigger(){
    bool trig=IO_inStates[hand*2];
    if(trig&&!triggerFlag){triggerFlag=true;
        if(seqLen>0){playheadPos=(playheadPos+1)%seqLen;outputNote(playheadPos);}}
    else if(!trig&&triggerFlag)triggerFlag=false;
}

void Melo::outputNote(int idx){
    if(!sequence[idx].on)return;
    int n=sequence[idx].val+60;                 // +5 V centre
    n=std::clamp(n,kSemiMin,kSemiMax);
    IO_writeDAC(hand*2,(4096/120)*n);
}

void Melo::draw(){

    u8g2_SetFont(&u8g2, u8g2_font_spleen5x8_mf);
    u8g2_SetFontPosCenter(&u8g2);

    controls_tick();
    u8g2_ClearBuffer(&u8g2);u8g2_SetDrawColor(&u8g2,1);
    drawSequence();drawLibrary();drawCursors();drawPlayhead();

    u8g2_SetDrawColor(&u8g2,2);
    u8g2_DrawBox(&u8g2,11,1,135,20);
    u8g2_SetDrawColor(&u8g2,1);

    if(showMenu)draw_menu();u8g2_UpdateDisplay(&u8g2);
}

void Melo::runFast(){IO_check(hand*2);processTrigger();}
void Melo::run(){}
void Melo::resume(){}
