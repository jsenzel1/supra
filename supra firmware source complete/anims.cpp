void swapAnim(int hand)
{
    u8g2_ClearBuffer(&u8g2);

    //u8g2_DrawBox(&u8g2,42,0,85,64);
    
    // Draw menu title
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    u8g2_SetFontPosTop(&u8g2);


    if(hand ==0)
    {
        u8g2_SetDrawColor(&u8g2, 1);

        int delay=1;
        int xoff=15;
        u8g2_DrawXBM(&u8g2,xoff,0,128,64,arrowjump2);
        sleep_ms(delay);
        u8g2_UpdateDisplay(&u8g2);

        u8g2_DrawXBM(&u8g2,xoff,0,128,64,arrowjump3);
        sleep_ms(delay);
        u8g2_UpdateDisplay(&u8g2);

        u8g2_DrawXBM(&u8g2,xoff,0,128,64,arrowjump4);
        sleep_ms(delay);
        u8g2_UpdateDisplay(&u8g2);

        u8g2_DrawXBM(&u8g2,xoff,0,128,64,arrowjump5);
        sleep_ms(delay);
        u8g2_UpdateDisplay(&u8g2);

        u8g2_DrawXBM(&u8g2,xoff,0,128,64,arrowjump6);

        u8g2_UpdateDisplay(&u8g2);
        sleep_ms(90);
    }

    if(hand ==1)
    {
        u8g2_SetDrawColor(&u8g2, 1);

        int delay=1;
        int xoff=-15;
        u8g2_DrawXBM(&u8g2,xoff,0,128,64,rarrowjump2);
        sleep_ms(delay);
        u8g2_UpdateDisplay(&u8g2);

        u8g2_DrawXBM(&u8g2,xoff,0,128,64,rarrowjump3);
        sleep_ms(delay);
        u8g2_UpdateDisplay(&u8g2);

        u8g2_DrawXBM(&u8g2,xoff,0,128,64,rarrowjump4);
        sleep_ms(delay);
        u8g2_UpdateDisplay(&u8g2);

        u8g2_DrawXBM(&u8g2,xoff,0,128,64,rarrowjump5);
        sleep_ms(delay);
        u8g2_UpdateDisplay(&u8g2);

        u8g2_DrawXBM(&u8g2,xoff,0,128,64,rarrowjump6);

        u8g2_UpdateDisplay(&u8g2);
        sleep_ms(90);
    }
}
void clockAnim()
{

    for(int i=0; i<16; i++)
    {
        u8g2_DrawXBM(&u8g2,0,0,128,64,clkArray[i]);
        u8g2_UpdateDisplay(&u8g2);
        sleep_ms(12);
    }

}
void selectAnim()
{
    printf("select Anim\n");
    int time=0;
    int t1=0;
    int t2=0;
    int t3=0;
    bool done=false;
    while(done==false)
    {
        u8g2_SetDrawColor(&u8g2, 1);
        t1=time*time*time;
        u8g2_DrawBox(&u8g2, 64-(t1/2),32-(t1/2),t1,t1);

        u8g2_SetDrawColor(&u8g2, 0);
        t2=time*time*2;
        u8g2_DrawBox(&u8g2, 64-(t2/2),32-(t2/2),t2,t2);

        
        u8g2_SetDrawColor(&u8g2, 1);
        t3=time;
        u8g2_DrawBox(&u8g2, 64-(t3/2),32-(t3/2),t3,t3);
        
        u8g2_UpdateDisplay(&u8g2);

        time++;

        if(time>8)
        {
            printf("DONE\n");
            done=true;
            break;
        }
    }
    return;
}

