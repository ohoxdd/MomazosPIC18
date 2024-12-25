#include "ADC.h"

void ADC_handle_result_RSI() {
	adc_value = (ADRESH << 8) | ADRESL;
}

void ADC_start(int channel) {
	ADCON0bits.CHS = channel;
	GO_nDONE = 1;    
}

int ADC_selectedChannel(){
	return ADCON0bits.CHS;
}

int ADC_ConversionLogic(int adc_values_arr[28]){
	int updated_channel = -1;
	// check if there is no ongoing conversion
	if (!GO_nDONE) {
		// check if the current channel was updated
		int channel = ADC_selectedChannel();
		bool new_val = adc_values_arr[channel] != adc_value;
		// if it was, set the new value
		if (new_val) {
			adc_values_arr[channel] = adc_value;
			updated_channel = channel;
		}
		// swap channels
		int next_chan;
		if (channel == 6) next_chan = 7;
		else if (channel == 7) next_chan = 6;
		// start conversion
		ADC_start(next_chan);
	}
	return updated_channel;
}

