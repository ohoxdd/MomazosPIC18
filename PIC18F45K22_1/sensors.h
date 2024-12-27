#ifndef SEONSORS_H
#define SEONSORS_H

#include <xc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "typedefs.h"
#include "initPIC.h"
#include "UI.h"

#define VCC_VAL 1023.0
#define R2_VALUE 4751.0
#define A_VALUE 0.0059257
#define B_VALUE 4050.0
#define K_ABS_ZERO 273.15

#define THRESHOLD_HIGH -5	// 90-61 PSI
#define THRESHOLD_MID -3	// 60-31 PSI
#define THRESHOLD_LOW -1	// 30-0  PSI

// Variable global temperatura precalculada
const double precalc = 13.595; 

unsigned int getReadPressure(unsigned long int adc_press);

double calculate_temp(int adc_temp);

unsigned int getCompressorTime(int adc_channel_values[28], int selected_pressure);

bool change_selected_pressure(int change, int* pselected);

bool detectPuncture(unsigned int prev, unsigned int val);

#endif
