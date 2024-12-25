#include "usart.h"


void usart_handle_input_RSI() {
	unsigned char input = RCREG1; // esto levanta ya la flag
	switch (input) {
		case 'w': {w_pressed = true; break;}
		case 'a': {a_pressed = true; break;}
		case 's': {s_pressed = true; break;}
		case 'd': {d_pressed = true; break;}
	}
}

void usart_1_putc (char xc)
{
	while (!TXSTA1bits.TRMT);
	TXREG1 = xc;
}

/* Transmit a null-terminated string using the previous function */
void usart_1_puts (unsigned char *cptr)
{
	while(*cptr)
	usart_1_putc (*cptr++);
	if (*cptr == '\0')
		usart_1_putc(0x0D); // Carry return Ascii, para hacer un newline en la terminal
}

unsigned char usart_1_getc (void)
{
	while (!PIR1bits.RCIF);
	return RCREG1; /* RCIF clears automatically */
}

// RECEIVE DATA
void usart_1_gets (const char *cptr)
{
	char* ptr = cptr;
	while (1)
	{
		char xx = usart_1_getc(); /* read a character */
		if (xx == 0x0D ) { /* is it a carriage return?*/ 
			*ptr++ = '\0'; /* terminate the string with a NULL */
			return; 
		}
		*ptr++ = xx; /* store the received character in the buffer */
	}
}

