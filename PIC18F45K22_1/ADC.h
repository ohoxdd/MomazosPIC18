#ifndef ADC_H
#define ADC_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

uint16_t adc_value;

void ADC_handle_result_RSI();

void ADC_start(int channel);

int ADC_selectedChannel();

int ADC_ConversionLogic(int adc_values_arr[28]);

#endif
