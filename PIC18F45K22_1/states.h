#ifndef STATES_H
#define STATES_H

#include <xc.h>

#include "typedefs.h"
#include "initPIC.h"



void states_set(state_t state);
state_t states_set_next(state_t state);

#endif
