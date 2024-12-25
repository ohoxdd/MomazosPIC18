#ifndef ADC_H
#define ADC_H
#include <stdint.h>

uint16_t adc_value;

void ADC_start(int channel);

int ADC_selectedChannel();

int ADC_ConversionLogic(int adc_values_arr[28]);

#endif
