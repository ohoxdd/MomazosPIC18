#include "splash.h"
#include "test.h"
#include "utils.h"
#include <stdbool.h>
#include <xc.h>

bool splash_on = 0;

#define _XTAL_FREQ 8000000  

void play_splash_screen() {
	if (!splash_on) {
        return;
    }

    writeMatrix(0, 0, 8, 128, splash_bitmap);
    __delay_ms(2000);
    clearGLCD(0, 8, 0, 128);
    int n = sizeof(splash_text)/sizeof(splash_text[0]);
    for (int i = 0; i < n; i++) {
        int column = calc_center_spacing(splash_text[i]);
        writeTxt(2+i, column, splash_text[i]); 
    }
    __delay_ms(2000);
    clearGLCD(0,7,0,127); 
}
