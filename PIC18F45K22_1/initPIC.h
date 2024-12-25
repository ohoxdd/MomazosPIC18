#ifndef INITPIC_H
#define INITPIC_H

#include <xc.h>

#define _XTAL_FREQ 8000000  

#define TIMER_STARTL 0xB0
#define TIMER_STARTH 0x3C

// esto va a ser irrelevante porque calcularemos el tiempo inicial del timer
#define TIEMPO_INICIAL 100

// Variables de la RSI
unsigned char change_time = 1;
unsigned int time_left = TIEMPO_INICIAL;

void tic(void);
void initPIC_config();

#endif
