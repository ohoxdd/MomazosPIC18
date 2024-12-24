/* Main.c 
 * Processor: PIC18F45K22
 * Compiler:  MPLAB XC8
 */

#define _XTAL_FREQ 8000000  

#include "ADC.h"
#include "states.h"
#include "UI.h"
#include "config.h"
#include "GLCD.h"
#include "splash.h"
#include "utils.h"
#include "initPIC.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define TIMER_STARTL 0xB0
#define TIMER_STARTH 0x3C
#define TIEMPO_INICIAL 100
#define MIN_PRESSURE 15
#define MAX_PRESSURE 100

#define VCC_VAL 1023.0
#define R2_VALUE 4751.0
#define A_VALUE 0.0059257
#define B_VALUE 4050.0
#define K_ABS_ZERO 273.15

// Varibales globales
// habilitar deshabilitar splash screen
unsigned int time_left = TIEMPO_INICIAL;
unsigned char change_time = 1;
unsigned int pressure_perc;
unsigned int adjust_pressure;
uint16_t adc_value;
int duty_one_pc;
bool change_temp;
bool adc_update;
int adc_channel_values[28];
bool w_pressed, a_pressed, s_pressed, d_pressed;

const double precalc = 13.595; // el valor de ln(R2_VALUE/A_VALUE) = 13.595 truncado hacia arriba

struct DC_values {
	uint8_t MSb; // CPPR3L
	uint8_t LSb; // CPP3CON<5:4>
};

struct DC_values DC_configurations[3] = {
	{0x32, 0x0}, // 40%
	{0x54, 0x0}, // 80%
	{0x76, 0xB}  // 95%
};

void handleUsartInput() {
	unsigned char input = RCREG1; // esto levanta ya la flag
	switch (input) {
		case 'w': {w_pressed = true; break;}
		case 'a': {a_pressed = true; break;}
		case 's': {s_pressed = true; break;}
		case 'd': {d_pressed = true; break;}
	}
}

void tic(void)
{
	time_left--;
	change_time = 1;
}

void handleADCresult() {
	adc_value = (ADRESH << 8) | ADRESL;
}

void interrupt RSI(){
	if (TMR2IF == 1 && TMR2IE == 1) {
		TMR2IF = 0;
		// TMR2IE = 0;
		// Bajando el enable no he notado una diferencia de comportamiento
	}
	
	if (TMR0IF == 1 && TMR0IE == 1) {
		TMR0IF = 0;

		// avanzamos el timer para que OVF ocurra en 0.1s
		// asignamos al buffer de TMR0H el valor de timer_starth
		// sumamos timer_startl a tmr0l, (el buffer high escribe en TMR0H)  
		// para tener en cuenta las instrucciones pasadas entre el OVF y la asignaci�n
		
		// Necesario mover esto a tic() ?
		TMR0H = TIMER_STARTH;
		TMR0L += TIMER_STARTL;

		tic();
	}
	
	if (ADIF == 1 && ADIE == 1) {
		ADIF = 0;
		handleADCresult();
	}
	
	if (PIR1bits.RCIF && PIE1bits.RC1IE) {
		PIE1bits.RC1IE = 0;
		handleUsartInput();
		PIE1bits.RC1IE = 1;
	}
}

 double calculate_temp(const double precalc, int adc_temp) {
	
	double result;
	result = (VCC_VAL / adc_temp) - 1;
	result = B_VALUE / (precalc + log(result));
	result -= K_ABS_ZERO;
	return result; 

} 

void write_pressure(){
	char buff[256];
    sprintf(buff,"%3d",pressure_perc);
	
	clearChars(0,0,5); // Sujeto a cambiar, ns como funciona esto
   	writeTxt(0,0,buff);
}

void change_selected_pressure(int change) {
	pressure_perc += change;
	if (pressure_perc <= MIN_PRESSURE) pressure_perc = MIN_PRESSURE;
	else if (pressure_perc >= MAX_PRESSURE) pressure_perc = MAX_PRESSURE;
}

int get_adjusted_pressure(const double temperature){
	// + 0.5 para redondear, ya que se trunca al pasar a int
	double change = 0.4 * (25.0 - temperature) + 0.5; 
	int adjusted_pressure = pressure_perc + change;
	return adjusted_pressure;
}


int set_pwm_pressure(const int adjusted_pressure){

	int pwm_val = duty_one_pc * adjusted_pressure;

	int pwm_high = pwm_val >> 2; 
	int pwm_low = pwm_val & 0x3;

	CCP3CON |= (pwm_low << 4);
	CCPR3L = pwm_high;

	return adjusted_pressure;
}

void write_adc_values(bool change_temp, bool change_press, int adc_values_arr[28]) {
	char buff[64];
	int current_chan = ADCON0bits.CHS;

	if (change_temp){
		clearChars(6,11,11);
		sprintf(buff, "ADC 6: %d\n", adc_values_arr[6]);
		writeTxt(6, 11, buff);
	}
	if (change_press) {
		clearChars(7,11,11);
		sprintf(buff, "ADC 7: %d\n", adc_values_arr[7]);
		writeTxt(7, 11, buff);
	}
	sprintf(buff, "ADC CHAN: %d\n", ADCON0bits.CHS);
	writeTxt(5, 11, buff);
}

void main(void)
{ 
	configPIC();

	duty_one_pc = ((PR2 + 1)*4)/100; // calculo para el valor de un porcentage del duty cycle
	
	GLCDinit();			//Inicialitzem la pantalla
	clearGLCD(0,7,0,127);	//Esborrem pantalla
	setStartLine(0);		//Definim linia d'inici
   
	play_splash_screen();
	
	// variables responsables la detecci�n de flancos
   	uint8_t READ_C = PORTC;
	uint8_t PREV_C = READ_C;

	bool RC0_pressed = !PORTCbits.RC0;
	bool RC1_pressed = PORTCbits.RC1;
	
	bool RC0_update = true;
	bool RC1_update = true;
	
	state_t timer_state = Ready;
	set_state(timer_state);
    updateStateTextTimer(timer_state);
	// bool timer_end = false;
	
	// selecciona la presion y la pone a 50% por defecto junto al medidor
	pressure_perc = 50;
	update_medidor(0, 50);

	// Variable que dicta si se escribe la temperatura en pantalla 
	// y se recalcula la presion ajustada a la temperatura
	double temperature = 25.0;
	// Podriamos usar un formato de coma fija?
	int adjusted_pressure = pressure_perc;

	int val = 0;
	
	for (int i = 0; i < 4; i++) {
		if (i == 1) continue;
		puts_usart1(splash_text[i]);
	}
	
	while (1)
	{   
		// adc related update flags to 0
		bool change_temp = false;
		bool change_press = false;
		// no hay conversion / ultima conversion ha acabado

		int updated_channel = ADC_ConversionLogic(adc_channel_values);

		if (updated_channel == 6) change_temp = true;
		else if (updated_channel == 7) change_press = true;

		write_adc_values(change_temp, change_press, adc_channel_values);
		
		if (change_temp) {
			// escribe temperatura
			clearGLCD(2,2, 63, 127);
			char buff[128];
			temperature = calculate_temp(precalc, adc_channel_values[6]);
			sprintf(buff, "%.2f C", temperature);
			writeTxt(2, 16, buff);

			// ajusta la presion segun la temperatura
			adjusted_pressure = get_adjusted_pressure(temperature);
			set_pwm_pressure(adjusted_pressure);
			write_pressure();
		}
		
        PREV_C = READ_C; // PREVIO <- ACTUAL
		READ_C = PORTC; // ACTUAL <- PORTC
		
		bool timer_end = (timer_state == Running && time_left == 0);

		// bool punxada =  detectar_punx()
		
		char buff[128];
		// DETECTOR DE INPUTS
		if (timer_state == Running)	{
			// RUNNING -> STOPPED
			if (inputDetector(PREV_C, READ_C, 3, 0) || w_pressed || timer_end) { // || punxada
				w_pressed = false;
				timer_state = set_next_state(timer_state);
				updateStateTextTimer(timer_state);
			}
		} else {
			// RC0 button checking
			if ((!RC0_pressed && inputDetector(PREV_C, READ_C,  0, FALLING)) || d_pressed ){
				d_pressed = false;
				RC0_pressed = true;
				RC0_update = true;
				
				// cambio la presi�n seleccionada, la ajusta, y la establece
				change_selected_pressure(1);
				adjusted_pressure = get_adjusted_pressure(temperature);
				set_pwm_pressure(adjusted_pressure);
				// actualiza la pantalla en base a los cambios
				update_medidor(pressure_perc, 1);
				write_pressure();


			} else if (RC0_pressed && inputDetector(PREV_C, READ_C,  0, RISING)){
				RC0_pressed = false;
				RC0_update = true;
			}

			//RC1 button checking
			if ((!RC1_pressed && inputDetector(PREV_C, READ_C,  1, RISING)) || a_pressed){
				a_pressed = false;
				RC1_pressed = true;
				RC1_update = true;

				// cambio la presi�n seleccionada, la ajusta, y la establece
				change_selected_pressure(-1);
				adjusted_pressure = get_adjusted_pressure(temperature);
				set_pwm_pressure(adjusted_pressure);
				// actualiza la pantalla en base a los cambios
				update_medidor(pressure_perc,-1);
				write_pressure();


			} else if (RC1_pressed && inputDetector(PREV_C, READ_C,  1, FALLING) ){
				RC1_pressed = false;
				RC1_update = true;
			}

			// STOPPED -> RUNNING
			// READY -> RUNNING
			// Termina bajo condiciones normales

			// SELECT
			if (inputDetector(PREV_C, READ_C, 2, 0) || s_pressed) {
				s_pressed = false;
				timer_state = set_next_state(timer_state);
				updateStateTextTimer(timer_state);
			}

		} 

        updateRunningTimer(timer_state);
	}
}
