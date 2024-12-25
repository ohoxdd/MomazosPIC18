#ifndef UTILS_H
#define UTILS_H

#include <xc.h>
#include <string.h>
#include <stdint.h>

#include "initPIC.h"
#include "GLCD.h"
#include "typedefs.h"



char inputDetector(uint8_t REG_ant, uint8_t REG_act, char num_pin, char flanc);

int calc_center_spacing(char * text);

void writeTxt(byte page, byte y, char * s);

void clearChars(byte page, byte y, int length);

format_t getFormatedTime(unsigned int deciseconds);

#endif
