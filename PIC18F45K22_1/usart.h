#ifndef USART_H
#define USART_H

#include <xc.h>
#include <string.h>
#include <stdbool.h>

#include "initPIC.h"

bool w_pressed, a_pressed, s_pressed, d_pressed;

void usart_handle_input_RSI();

void usart_1_putc (char xc);

void usart_1_puts (unsigned char *cptr);

unsigned char usart_1_getc (void);

void usart_1_gets (const char *cptr);

#endif
