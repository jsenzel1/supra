#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "Controls.h"
#include "pico/time.h"

//int b0=28;
int b0=8;
int b1=28;
int b2=29;
//int b1=2;
//^real b1

bool b0Pressed;
bool b1Pressed;
bool b2Pressed;

//int inUp=11;
int inUp=0;
int inDown=2;
int inLeft=1;
int inRight=3;
//int inCenter=13;
int inCenter=4;

bool centerPressed=false;
bool upPressed=false;
bool downPressed=false;
bool leftPressed=false;
bool rightPressed=false;

#define INITIAL_DELAY 350 // ms before repeating starts
#define MIN_REPEAT_DELAY 20 // ms between repeats at max speed
#define ACCELERATION_RATE 0.65 // multiplier for decreasing delay

    void con_init()
    {

      //stdio_init_all();

      gpio_init(b0);
      gpio_init(b1);
      gpio_init(b2);

      gpio_init(inUp);
      gpio_init(inDown);
      gpio_init(inLeft);
      gpio_init(inRight);
      gpio_init(inCenter);

      gpio_set_function(inUp,GPIO_FUNC_SIO);

      gpio_pull_up(b0);
      gpio_pull_up(b1);
      gpio_pull_up(b2);

      gpio_pull_up(inUp);
      gpio_pull_up(inDown);
      gpio_pull_up(inLeft);
      gpio_pull_up(inRight);
      gpio_pull_up(inCenter);

      gpio_set_dir(b0,GPIO_IN);
      gpio_set_dir(b1,GPIO_IN);
      gpio_set_dir(b2,GPIO_IN);

      gpio_set_dir(inUp,GPIO_IN);
      gpio_set_dir(inDown,GPIO_IN);
      gpio_set_dir(inLeft,GPIO_IN);
      gpio_set_dir(inRight,GPIO_IN);
      gpio_set_dir(inCenter,GPIO_IN);

    }

    bool button_contin(int button)
    {
        //placeholder for testing, in the future button 2 will be different
        if(button==0)
        {
            if(gpio_get(b0)==0)
            {
                return true;
            } else {
                return false;
            }
        }

        if(button==1)
        {
            if(gpio_get(b1)==0)
            {
                return true;
            } else {
                return false;
            }
        }
        if(button==2)
        {
            if(gpio_get(b2)==0)
            {
                return true;
            } else {
                return false;
            }
        }
    }


    bool button_momentary(int button)
    {
       if(button==0)
       {
           if(gpio_get(b0)==0)
           {
                if(!b0Pressed)
                {
                    b0Pressed=true;
                    printf("b0 press\n");
                    return true;
                } else {
                    return false;
                }
           } else {
                b0Pressed=false;
                return false;
           }
       }

       if(button==1)
       {
           if(gpio_get(b1)==0)
           {
                if(!b1Pressed)
                {
                    b1Pressed=true;
                    printf("b1 press\n");
                    return true;
                } else {
                    return false;
                }
           } else {
                b1Pressed=false;
                return false;
           }
       }

       if(button==2)
       {
           if(gpio_get(b2)==0)
           {
                if(!b2Pressed)
                {
                    b2Pressed=true;
                    printf("b2 press\n");
                    return true;
                } else {
                    return false;
                }
           } else {
                b2Pressed=false;
                return false;
           }
       }
    }

    int con_joyMomentary()
    {

      //

      if(gpio_get(inUp)==0)
      {
          if(!upPressed)
          {
              printf("UP \n ");
              upPressed=true;
              return 1;
          }
      }

      if(gpio_get(inUp)==1)
      {
          upPressed=false;
      }

      //

      if(gpio_get(inDown)==0)
      {
          if(!downPressed)
          {
          printf("DOWN \n ");
          downPressed=true;
          return 2;
          }
      }

      if(gpio_get(inDown)==1)
      {
          downPressed=false;
      }

      //

      if(gpio_get(inLeft)==0)
      {
          if(!leftPressed)
          {
          printf("LEFT \n ");
          leftPressed=true;
          return 3;
          }
      }

      if(gpio_get(inLeft)==1)
      {
          leftPressed=false;
      }

      //

      if(gpio_get(inRight)==0)
      {
          if(!rightPressed)
          {
          printf("RIGHT \n ");
          rightPressed=true;
          return 4;
          }
      }

      if(gpio_get(inRight)==1)
      {
          rightPressed=false;
      }

      //

      if(gpio_get(inCenter)==0)
      {
          if(!centerPressed)
          {
          printf("CENTER \n ");
          centerPressed=true;
          return 5;
          }
      }

      if(gpio_get(inCenter)==1)
      {
          centerPressed=false;
      }

      //
      return 0;
    }

    int con_joyContin()
    {

      if(gpio_get(inUp)==0)
      {
          //printf("UP \n ");
          return 1;
      }

      if(gpio_get(inDown)==0)
      {
          //printf("DOWN \n");
          return 2;
      }

      if(gpio_get(inLeft)==0)
      {
          //printf("LEFT \n");
          return 3;
      }

      if(gpio_get(inRight)==0)
      {
          //printf("RIGHT \n");
          return 4;
      }

      if(gpio_get(inCenter)==0)
      {
          //printf("PUSH \n");
          return 5;
      }

      return 0;
    }

    int con_joyAccelerate() {
        static int lastDirection = 0;
        static uint64_t pressStartTime = 0;
        static uint64_t lastRepeatTime = 0;
        static double currentRepeatDelay = INITIAL_DELAY;

        uint64_t currentTime = to_ms_since_boot(get_absolute_time());
        int direction = con_joyContin(); // Use the existing continuous function to check current state

        if (direction != lastDirection) {
            // Direction changed or button released
            lastDirection = direction;
            if (direction != 0) {
                // New direction pressed
                pressStartTime = currentTime;
                lastRepeatTime = currentTime;
                currentRepeatDelay = INITIAL_DELAY;
                return direction;
            }
        } else if (direction != 0) {
            // Same direction held
            uint64_t holdTime = currentTime - pressStartTime;

            if (holdTime >= INITIAL_DELAY) {
                // Check if it's time for a repeat
                if (currentTime - lastRepeatTime >= currentRepeatDelay) {
                    lastRepeatTime = currentTime;

                    // Accelerate the repeat rate
                    currentRepeatDelay *= ACCELERATION_RATE;
                    if (currentRepeatDelay < MIN_REPEAT_DELAY) {
                        currentRepeatDelay = MIN_REPEAT_DELAY;
                    }

                    return direction;
                }
            }
        }

        return 0; // No input to report
    }

