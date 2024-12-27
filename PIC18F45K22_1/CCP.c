#include "CCP.h"

void change_pwm_values(struct DC_values values) {
	CCP3CON |= (values.LSb << 4);
	CCPR3L = values.MSb;
}

void change_pwm_profile(int selected_pressure) {

	struct DC_values DC_configurations[3] = {
		{0x32, 0x0}, // 40%
		{0x54, 0x0}, // 80%
		{0x76, 0xB}  // 95%
	};

	if (selected_pressure <= 30) {
		// Entra al intervalo <= 30 psi
		change_pwm_values(DC_configurations[0]);
	} else if (selected_pressure <= 60) {
		// Entra al intervalo <= 60 psi
		change_pwm_values(DC_configurations[1]);
	} else {
		// Entra al intervalo <= 90 psi
		change_pwm_values(DC_configurations[2]);
	}
}
