#include "splash.h"



void splash_play() {
	if (!splash_on) {
        return;
    }

    writeMatrix(0, 0, 8, 128, splash_bitmap);
    __delay_ms(2000);
    clearGLCD(0, 8, 0, 128);
    int n = sizeof(splash_text)/sizeof(splash_text[0]);
    for (int i = 0; i < n; i++) {
        int column = utils_calc_center_spacing(splash_text[i]);
        utils_writeTxt(2+i, column, splash_text[i]); 
    }

	
    __delay_ms(2000);
    clearGLCD(0,7,0,127); 
}

void writeMatrix(int start_page, int start_col, int fil, int col, char* matrix) {
    int array_ind = 0;
    for (int i = 0; i < fil; ++i) {
        for (int j = 0; j < col; ++j) {
            writeByte(start_page + i, start_col + j, matrix[array_ind]);
            array_ind++;
        }
    }
}
