#ifndef INITPIC_H
#define INITPIC_H

#include <xc.h>

#define _XTAL_FREQ 8000000  

#define TIMER_STARTL 0xB0
#define TIMER_STARTH 0x3C

void initPIC_config();
#endif
