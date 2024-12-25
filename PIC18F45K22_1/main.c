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
#define MIN_PRESSURE 0
#define MAX_PRESSURE 90

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
bool w_pressed, a_pressed, s_pressed, d_pressed;

const double precalc = 13.595; // el valor de ln(R2_VALUE/A_VALUE) = 13.595 truncado hacia arriba

struct DC_values {
	uint8_t MSb; // CCPR3L
	uint8_t LSb; // CCP3CON<5:4>
};

struct DC_values DC_configurations[3] = {
	{0x32, 0x0},  // 40%
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
	if (TMR2IF && TMR2IE) {TMR2IF = 0;}
	
	if (TMR0IF && TMR0IE) {
		TMR0IF = 0;
		TMR0H = TIMER_STARTH;
		TMR0L += TIMER_STARTL;
		tic();
	}
	
	if (ADIF && ADIE) {
		ADIF = 0;
		handleADCresult();
	}
	
	if (PIR1bits.RCIF && PIE1bits.RC1IE) {handleUsartInput();}
}

 double calculate_temp(const double precalc, int adc_temp) {
	
	double result;
	result = (VCC_VAL / adc_temp) - 1;
	result = B_VALUE / (precalc + log(result));
	result -= K_ABS_ZERO;
	return result; 

} 

// Para hacer la conversion del valor del adc a la presion (0:1023) --> (0:90)
// Lo que hacemos es escalar el valor 90/1023 a 58982/2^16 para poder hacer
// una division por potencia de 2.  

unsigned int getReadPressure(int adc_press) {
	unsigned int pressure;
	return ((adc_press*58982)/65536);
	// return ((adc_press*58982) >> 16) no estoy seguro de que esto funcione
	// a lo mejor el compilador hace su magia pero meh, no me quiero arriesgar
}

// Podriamos pasar solo los valores del channel 6 y 7, no hay necesidad de copiar el array entero
unsigned int getCompressorTime(int adc_channel_values[28]) {
	unsigned int result;
	int adc_temp = adc_channel_values[6];
	int read_press = getReadPressure(adc_channel_values[7]);
	int t_ambient = (int)calculate_temp(precalc, adc_temp); // esto puede llegar a truncar bastante
	result = ((pressure_perc-read_press)/2) - (25 - t_ambient);
	result *= 10; // esto para que este en decimas de segundo
	return result;
}

void write_pressure(){
	char buff[256];
    sprintf(buff,"PSI selected: %3d",pressure_perc);
	
	clearChars(0,0,17); // Sujeto a cambiar, ns como funciona esto
   	writeTxt(0,0,buff);
}

void change_pwm_values(struct DC_values values) {
	CCP3CON |= (values.LSb << 4);
	CCPR3L = values.MSb;
}

void change_selected_pressure(int change) {
	int old_pressure = pressure_perc;
	pressure_perc += change;
	if (pressure_perc <= MIN_PRESSURE) pressure_perc = MIN_PRESSURE;
	else if (pressure_perc >= MAX_PRESSURE) pressure_perc = MAX_PRESSURE;

	// Comprobar si la presion pasa de un intervalo a otro y, 
	// si lo hace, cambiar los valores de los registros que controlan el duty cycle "potencia del compresor"
	
	if (old_pressure > 30 && pressure_perc <= 30) {
		// Entra al intervalo <= 30 psi
		change_pwm_values(DC_configurations[0]);
	} else if ((old_pressure <= 30 && pressure_perc > 30) || (old_pressure > 60 && pressure_perc <= 60)) {
		// Entra al intervalo <= 60 psi
		change_pwm_values(DC_configurations[1]);
	} else if (old_pressure <= 60 && pressure_perc > 60) {
		// Entra al intervalo <= 90 psi
		change_pwm_values(DC_configurations[2]);
	}
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
	
	// selecciona la presion y la pone a 50% por defecto junto al medidor
	pressure_perc = 50;
	update_medidor(0, 50);

	// Variable que dicta si se escribe la temperatura en pantalla 
	// y se recalcula la presion ajustada a la temperatura
	double temperature = 25.0;

	int adc_channel_values[28];

	while (1)
	{   
		// adc related update flags to 0
		bool change_temp = false;
		bool change_press = false;
		// no hay conversion / ultima conversion ha acabado
		
		// Si el canal de la conversion tiene un valor distinto del anterior, 
		// last_updated_channel = CHS; else last_updated_channel = -1;
		int last_updated_channel = ADC_ConversionLogic(adc_channel_values);
		
		if (last_updated_channel == 6) change_temp = true;
		else if (last_updated_channel == 7) change_press = true;

		write_adc_values(change_temp, change_press, adc_channel_values);
		
		if (change_temp) {

			// escribe temperatura
			clearGLCD(2,2, 63, 127);
			char buff[128];
			temperature = calculate_temp(precalc, adc_channel_values[6]);
			sprintf(buff, "%.2f C", temperature);
			writeTxt(2, 16, buff);
			write_pressure();
		}

        PREV_C = READ_C; // PREVIO <- ACTUAL
		READ_C = PORTC; // ACTUAL <- PORTC
		
		bool timer_end = (timer_state == Running && time_left == 0);

		// DETECTOR DE INPUTS
		if (timer_state == Running)	{

			// RUNNING -> STOPPED
			if (inputDetector(PREV_C, READ_C, 3, 0) || w_pressed || timer_end) { // || punxada
				w_pressed = false;
				timer_state = set_next_state(timer_state);
				updateStateTextTimer(timer_state);
			}

			// bool punxada =  detectar_punx()
			// if (punxada) bla bla bla

		} else {
			// RC0 button checking
			if ((!RC0_pressed && inputDetector(PREV_C, READ_C,  0, FALLING)) || d_pressed ){
				d_pressed = false;
				RC0_pressed = true;
				RC0_update = true;
				
				// cambio la presi�n seleccionada, la ajusta, y la establece
				change_selected_pressure(1);

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
				if (timer_state == Ready) {
					// next state == Running, calculamos el tiempo
					// esto es un poco chapuza pq me da pereza organizar el codigo bien
					//
					// esto tmb lo hago aqui pq el propio set_state enciende los timers, 
					// y se debe calcular el tiempo antes de encender el propio timer
					
					time_left = getCompressorTime(adc_channel_values);
				}
				timer_state = set_next_state(timer_state);
				updateStateTextTimer(timer_state);
			}

		} 

        updateRunningTimer(timer_state);
	}
}
