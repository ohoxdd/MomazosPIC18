#include "utils.h"
#include <xc.h>



void putc_usart1 (char xc)
{
	while (!TXSTA1bits.TRMT);
	TXREG1 = xc;
}
/* Transmit a null-terminated string using the previous function */
void puts_usart1 (unsigned char *cptr)
{
	while(*cptr)
	putc_usart1 (*cptr++);
	if (*cptr == '\0')
		putc_usart1(0x0D); // Carry return Ascii, para hacer un newline en la terminal
}

unsigned char getc_usart1 (void)
{
	while (!PIR1bits.RCIF);
	return RCREG1; /* RCIF clears automatically */
}

// RECEIVE DATA
void gets_usart1 (const char *cptr)
{
	char* ptr = cptr;
	while (1)
	{
		char xx = getc_usart1(); /* read a character */
		if (xx == 0x0D ) { /* is it a carriage return?*/ 
			*ptr++ = '\0'; /* terminate the string with a NULL */
			return; 
		}
		*ptr++ = xx; /* store the received character in the buffer */
	}
}

int calc_center_spacing(char * string){
    return 13 - strlen(string)/2;
}

void writeTxt(char page, char y, char * s) {
   int i=0;
   while (*s!='\n' && *s!='\0') 
   {    
      putchGLCD(page, y+i, *(s++));
      i++;
   };
}

void clearChars(byte page, byte y, int length) {
	byte column = y * 5;
	byte column_end = column + length * 5 - 1;
	 clearGLCD(page,page,column,column_end);
}

format_t getFormatedTime(unsigned int deciseconds) {
    format_t time;

	time.dec = deciseconds % 10;
	deciseconds /= 10; // seconds
	
	time.segs = deciseconds;

    return time;
}
