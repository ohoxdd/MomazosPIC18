#ifndef STATES_H
#define STATES_H

#include <xc.h>

#include "typedefs.h"

#define TIMER_STARTL 0xB0
#define TIMER_STARTH 0x3C
#define TIEMPO_INICIAL 100

unsigned int time_left = TIEMPO_INICIAL;

void set_state(state_t state);
state_t set_next_state(state_t state);

#endif
