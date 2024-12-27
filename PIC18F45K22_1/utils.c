#include "utils.h"

char utils_inputDetector(uint8_t REG_ant, uint8_t REG_act, char num_pin, char flanc) {
	char ret = 0;

	char pin_act = (REG_act >> num_pin) & 0x01;
	
	char pin_ant = (REG_ant >> num_pin) & 0x01;

	if (flanc == RISING) {
		ret = pin_act && !pin_ant;
	} else {
		ret = !pin_act && pin_ant;
	}
	
	if (ret) __delay_ms(10);
	
	
	return ret;
}

int utils_calc_center_spacing(char * string){
    return 13 - strlen(string)/2;
}

void utils_writeTxt(char page, char y, char * s) {
   int i=0;
   while (*s!='\n' && *s!='\0') 
   {    
      putchGLCD(page, y+i, *(s++));
      i++;
   };
}

void utils_clearChars(byte page, byte y, int length) {
	byte column = y * 5;
	byte column_end = column + length * 5 - 1;
	 clearGLCD(page,page,column,column_end);
}

format_t utils_getFormatedTime(unsigned int deciseconds) {
    format_t time;

	time.dec = deciseconds % 10;
	deciseconds /= 10; // seconds
	
	time.segs = deciseconds;

    return time;
}
