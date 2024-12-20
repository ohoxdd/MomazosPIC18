#ifndef UTILS_H
#define UTILS_H
#include "GLCD.h"
#include "typedefs.h"
#include <string.h>

void putc_usart1 (char xc);
void puts_usart1 (unsigned char *cptr);
unsigned char getc_usart1 (void);
void gets_usart1 (const char *cptr);

int calc_center_spacing(char * text);

void writeTxt(byte page, byte y, char * s);

void clearChars(byte page, byte y, int length);

format_t getFormatedTime(unsigned int deciseconds);

#endif
