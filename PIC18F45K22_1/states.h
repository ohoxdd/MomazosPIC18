#ifndef STATES_H
#define STATES_H

#include <xc.h>

#include "typedefs.h"
#include "initPIC.h"

#define TIEMPO_INICIAL 100


void states_set(state_t state);
state_t states_set_next(state_t state);

#endif
