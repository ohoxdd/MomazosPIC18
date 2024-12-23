#ifndef ADC_H
#define ADC_H


void ADC_start(int channel);

int ADC_selectedChannel();

int ADC_ConversionLogic(int adc_values_arr[28]);

#endif
