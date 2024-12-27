#include "sensors.h"

double calculate_temp(int adc_temp) {
	
	double result;
	result = (VCC_VAL / adc_temp) - 1;
	result = B_VALUE / (precalc + log(result));
	result -= K_ABS_ZERO;
	return result; 

} 

unsigned int getReadPressure(unsigned long int adc_press) {
    unsigned int result = adc_press* 45 + 256;
    result /= 512;
    return result;
}

unsigned int getCompressorTime(int adc_channel_values[28], int selected_pressure) {
	int result;
	int adc_temp = adc_channel_values[6];
	int read_press = getReadPressure(adc_channel_values[7]);

	double t_ambient_double = calculate_temp(adc_temp);
	int t_ambient = (int)t_ambient_double; // esto puede llegar a truncar bastante
	result = ((selected_pressure - read_press)/2) - (t_ambient - 25);
	result *= 10; // esto para que este en decimas de segundo
	if (result < 0) result = 0;
	return (unsigned int)result;
}

bool change_selected_pressure(int change, int* pselected) {
	
	int old_pressure = *pselected;
	int new_pressure = old_pressure + change;

	if (new_pressure <= MIN_PRESSURE) {
		new_pressure = MIN_PRESSURE;
	} else if (new_pressure >= MAX_PRESSURE) {
		new_pressure = MAX_PRESSURE;
	}
	bool has_changed = old_pressure != new_pressure;

	if (has_changed) {
		*pselected = new_pressure;
	}
	
	return has_changed;
}

bool detectPuncture(unsigned int prev, unsigned int val) {
	signed int diff_pressure = val - prev; // ponemos el signed explicito por si acaso
	if (diff_pressure >= 0) return false;

	// Evaluamos si lo detectamos como pinchazo o no
	// en base a los thresholds definidos
	
	if (prev > 60) {
		return diff_pressure <= THRESHOLD_HIGH;
	}
	else if (prev > 30) {
		return diff_pressure <= THRESHOLD_MID;
	}

	return diff_pressure <= THRESHOLD_LOW;
}