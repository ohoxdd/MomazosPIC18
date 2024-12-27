#ifndef UTILS_H
#define UTILS_H

#include <xc.h>
#include <string.h>
#include <stdint.h>

#include "initPIC.h"
#include "GLCD.h"
#include "typedefs.h"



char utils_inputDetector(uint8_t REG_ant, uint8_t REG_act, char num_pin, char flanc);

int utils_calc_center_spacing(char * text);

void utils_writeTxt(byte page, byte y, char * s);

void utils_clearChars(byte page, byte y, int length);

format_t utils_getFormatedTime(unsigned int deciseconds);

#endif
