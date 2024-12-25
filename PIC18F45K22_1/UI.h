#ifndef UI_H
#define UI_H 

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "typedefs.h"
#include "GLCD.h"

#define MIN_PRESSURE 0
#define MAX_PRESSURE 90
#define ANCHURA_MEDIDOR 10
void update_medidor(int current_psi, int change_psi);

#endif // !UI_H
#define UI_H 


