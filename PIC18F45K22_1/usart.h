#ifndef USART_H
#define USART_H

#include <xc.h>
#include <string.h>

#include "initPIC.h"

void usart_1_putc (char xc);

void usart_1_puts (unsigned char *cptr);

unsigned char usart_1_getc (void);

void usart_1_gets (const char *cptr);

#endif
