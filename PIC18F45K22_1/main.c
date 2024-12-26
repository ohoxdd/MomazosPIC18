/* Main.c 
 * Processor: PIC18F45K22
 * Compiler:  MPLAB XC8
 */

//  #include "defines.h" // comenta esto y descomenta lo de arriba pa q compile


#include <xc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "config.h"
#include "initPIC.h"
#include "ADC.h"
#include "states.h"
#include "UI.h"
#include "GLCD.h"
#include "splash.h"
#include "utils.h"
#include "usart.h"
#include "typedefs.h"

#define VCC_VAL 1023.0
#define R2_VALUE 4751.0
#define A_VALUE 0.0059257
#define B_VALUE 4050.0
#define K_ABS_ZERO 273.15

// Variable global temperatura precalculada
const double precalc = 13.595; 

// Variables globales de presion (habra que quitarlas)
unsigned int pressure_perc;
unsigned int adjust_pressure;

struct DC_values DC_configurations[3] = {
	{0x32, 0x0},  // 40%
	{0x54, 0x0}, // 80%
	{0x76, 0xB}  // 95%
};

// Variables de la RSI
bool change_time = true;
unsigned int time_left = TIEMPO_INICIAL;
int counter_decimas = 0;
bool check_pressure;

void tic(void) {
	time_left--;
	counter_decimas++;
	if (counter_decimas == 10) {
		counter_decimas = 0;
		check_pressure = true;
	}
	change_time = true;
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
		ADC_handle_result_RSI();
	}
	
	if (PIR1bits.RCIF && PIE1bits.RC1IE) {usart_handle_input_RSI();}
}

/* factorizamos R2/A
	
	   -1 * R2 * (1 - VCC/ VOUT) 
	 -------------------------------
	              A
	
	= R2/A * (VCC/VOUT - 1)

	por propiedades de logaritmos
	
			ln (R2/A * (VCC/VOUT - 1))
			= ln(R2/A) + ln(VCC/VOUT - 1)

	usamos ln(R2/A) previamente calculado 
 	precalc el valor redondeado de ln(R2/A) = 13.595  	
*/

double calculate_temp(int adc_temp) {
	
	double result;
	result = (VCC_VAL / adc_temp) - 1;
	result = B_VALUE / (precalc + log(result));
	result -= K_ABS_ZERO;
	return result; 

} 
// Para hacer la conversion del valor del adc a la presion (0:1023) --> (0:90)
// Lo que hacemos es escalar el valor 90/1023 a 58982/2^16 para poder hacer
// una division por potencia de 2.  

unsigned int getReadPressure(unsigned long int adc_press) {
	//return ((adc_press*58982)/65536);
	return (adc_press*88)/1000;
	// return ((adc_press*58982) >> 16) no estoy seguro de que esto funcione
	// a lo mejor el compilador hace su magia pero meh, no me quiero arriesgar
}

// Podriamos pasar solo los valores del channel 6 y 7, no hay necesidad de copiar el array entero
unsigned int getCompressorTime(int adc_channel_values[28]) {
	int result;
	int adc_temp = adc_channel_values[6];
	int read_press = getReadPressure(adc_channel_values[7]);

	double t_ambient_double = calculate_temp(adc_temp);
	int t_ambient = (int)t_ambient_double; // esto puede llegar a truncar bastante
	result = ((pressure_perc-read_press)/2) - (t_ambient - 25);
	result *= 10; // esto para que este en decimas de segundo
	if (result < 0) result = 0;
	return (unsigned int)result;
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
		clearChars(6,15,11);
		sprintf(buff, "ADC 6: %d\n", adc_values_arr[6]);
		writeTxt(6, 15, buff);
	}
	if (change_press) {
		clearChars(7,15,11);
		//read_press = getReadPressure(adc_values_arr[7]);
		sprintf(buff, "ADC 7: %d\n", adc_values_arr[7]);
		writeTxt(7, 15, buff);
	}
	//sprintf(buff, "ADC CHAN: %d\n", ADCON0bits.CHS);
	//writeTxt(5, 11, buff);
}



void writeTimerCountdown(state_t timer_state){
		format_t ftime;
	    char buff[128];

		ftime = getFormatedTime(time_left);
		sprintf(buff, "Time: %02d.%0d\n", ftime.segs, ftime.dec);
		
		writeTxt(5, 0, buff); 
		
		change_time = false;
}


void updateStateTextTimer(state_t timer_state) {
    format_t ftime;

	char countdown[256];
	char stateText[50];
  
	// Update segun los estados
    clearGLCD(1,1,60,127); // Clear del texto "Ready"/"Running..."/"Stopped"
    
    switch (timer_state) { // EL VALOR ACTUAL DEL STATUS
        case Ready:

            sprintf(stateText, "Ready\n");
            
            writeTimerCountdown(time_left);
            writeTxt(1,19, stateText); 
            break;
            
        case Running:
            // Aqui, como el countdown de running se tiene que actualizar cada decima de segundo
            // solo nos preocupamos de escribir el texto del estado de "Running..."
            
            sprintf(stateText, "Running...\n"); 
            
            writeTxt(1,14, stateText);
            break;
            
        case Stopped:
            // Es necesario escribir el valor del timer en stopped 
            // para tener el valor correcto en caso de pausa

            sprintf(stateText, "Stopped!\n");
            
			writeTimerCountdown(time_left);
            writeTxt(1,16, stateText);
            
            break;
    }
}

// Tasas a partir de las que consideramos
// pinchazo
#define THRESHOLD_HIGH -5	// 90-61 PSI
#define THRESHOLD_MID -3	// 60-31 PSI
#define THRESHOLD_LOW -1	// 30-0  PSI

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

//void displayPunctureWarning() {
	// 
//}

write_temp(double temperature){
	clearGLCD(2,2, 63, 127);
	char buff[128];
	sprintf(buff, "%.2f C", temperature);
	writeTxt(2, 16, buff);
}

write_ambient_pressure(unsigned int read_press){
	clearGLCD(3,3, 63, 127);
	char buff[128];
	sprintf(buff, "%2d PSI", read_press);
	writeTxt(3, 16, buff);
}

void main(void)
{ 
	initPIC_config();
	
	GLCDinit();			//Inicialitzem la pantalla
	clearGLCD(0,7,0,127);	//Esborrem pantalla
	setStartLine(0);		//Definim linia d'inici
   
	splash_play();
	
	// variables responsables la detecci�n de flancos
   	uint8_t READ_C = PORTC;
	uint8_t PREV_C = READ_C;

	bool RC0_pressed = !PORTCbits.RC0;
	bool RC1_pressed = PORTCbits.RC1;
	
	bool RC0_update = true;
	bool RC1_update = true;
	
	
	
	// selecciona la presion y la pone a 50% por defecto junto al medidor
	pressure_perc = 50;

	// Variable que dicta si se escribe la temperatura en pantalla 
	// y se recalcula la presion ajustada a la temperatura
	// double temperature = 25.0;

	int adc_channel_values[28];

	// mover a Usart.h
	for (int i = 0; i < 4; i++) {
		if (i == 1) continue;
		usart_1_puts(splash_text[i]);
	}

	int prev_pressure = -1;
	bool punxada = false;

	setup_medidor(48, 1, 50, 0);
	
	state_t timer_state = Ready;
	states_set(timer_state);
    updateStateTextTimer(timer_state);

	int time_max;
	// escribe por primera vez presión selected
	write_pressure();
	// escribe por primera vez temp y presión ambiental
	write_temp(25.0);
	write_ambient_pressure(0);
	while (1)
	{   
		// adc related update flags to 0
		bool change_temp = false;
		bool change_press = false;
		// no hay conversion / ultima conversion ha acabado
		
		// Si el canal de la conversion tiene un valor distinto del anterior, 
		// last_updated_channel = CHS; 
		// else last_updated_channel = -1;
		int last_updated_channel = ADC_ConversionLogic(adc_channel_values);
		
		if (last_updated_channel == 6) change_temp = true;
		else if (last_updated_channel == 7) change_press = true;

		// write_adc_values(change_temp, change_press, adc_channel_values);
		
		if (change_temp) {
			double temperature = calculate_temp(adc_channel_values[6]);
			write_temp(temperature);
		}
		
		if (change_press) {
			unsigned int read_press = getReadPressure(adc_channel_values[7]);
			write_ambient_pressure(read_press);
		}

		if (timer_state == Running && change_time) {
			writeTimerCountdown(timer_state); 
		}

        PREV_C = READ_C; // PREVIO <- ACTUAL
		READ_C = PORTC; // ACTUAL <- PORTC
		
		bool timer_end = (timer_state == Running && time_left == 0);

		// DETECTOR DE INPUTS
		if (timer_state == Running)	{


			// RUNNING -> STOPPED
			if (inputDetector(PREV_C, READ_C, 3, 0) || w_pressed || timer_end) { // || punxada
				w_pressed = false;
				timer_state = states_set_next(timer_state);
				updateStateTextTimer(timer_state);
			}

			if (check_pressure) {
				if (prev_pressure == -1) prev_pressure = getReadPressure(adc_channel_values[7]);
				else {
					unsigned int actual_pressure = getReadPressure(adc_channel_values[7]);
					punxada = detectPuncture(prev_pressure, actual_pressure);
					prev_pressure = actual_pressure;
				}

			}

			new_update_medidor(time_max - time_left, time_max, 0);

			if (punxada) {
				// displayPunctureWarning();
				// Como funciona esto?
				//

				char buff[256];
				sprintf(buff, "Pinchazo detectado!!!\n");
				usart_1_puts(buff);
				punxada = false;
			} 


		} else {
			// RC0 button checking
			if ((!RC0_pressed && inputDetector(PREV_C, READ_C,  0, FALLING)) || d_pressed ){
				d_pressed = false;
				RC0_pressed = true;
				RC0_update = true;
				
				// cambio la presi�n seleccionada, la ajusta, y la establece
				change_selected_pressure(1);

				// actualiza la pantalla en base a los cambios
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
					// esto tmb lo hago aqui pq el propio states_set enciende los timers, 
					// y se debe calcular el tiempo antes de encender el propio timer
					
					//time_left = TIEMPO_INICIAL;
					time_left = getCompressorTime(adc_channel_values);
					time_max = time_left;
					
					/* DEBUG USART LINES */
					/* DEBUG USART LINES */

						char debug[128];
						sprintf(debug, "Tiempo configurado a %02d.%d segundos\n", time_left/10, time_left%10);
						usart_1_puts(debug);
						//unsigned int read_press = getReadPressure(adc_channel_values[7]);
						//sprintf(debug, "Diferencia de presion = %d - %d = %d\n", pressure_perc, read_press, pressure_perc - read_press);
						//usart_1_puts(debug);
						////sprintf(debug, "Formula:\n");
						//usart_1_puts(debug);
						//sprintf(debug, "s = (%d-%d)/2 - (%d - 25)\n", pressure_perc, read_press, (int)calculate_temp(adc_channel_values[6]));
						//usart_1_puts(debug);

					/* DEBUG USART LINES */
					/* DEBUG USART LINES */
				}

				timer_state = states_set_next(timer_state);
				updateStateTextTimer(timer_state);
				
			}

		} 
	}
}
