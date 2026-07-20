#include <stdio.h>
#include "pico/stdlib.h"

extern bool upPressed;

void con_init();
bool button_contin(int);
bool button_momentary(int);

int con_joyMomentary();
//^ 1UP 2DOWN 3LEFT 4RIGHT 5PRESS 0NONE

int con_joyContin();
int con_joyAccelerate();
