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
#include "CCP.h"

#define VCC_VAL 1023.0
#define R2_VALUE 4751.0
#define A_VALUE 0.0059257
#define B_VALUE 4050.0
#define K_ABS_ZERO 273.15

// Variable global temperatura precalculada
const double precalc = 13.595; 


// Variables de la RSI
bool change_time = true;
unsigned int time_left;
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


unsigned int getReadPressure(unsigned long int adc_press) {
	return (adc_press*88)/1000;
}

// Podriamos pasar solo los valores del channel 6 y 7, no hay necesidad de copiar el array entero
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

void write_pressure(int selected_pressure){
	char buff[16];
    sprintf(buff,"PSI:");
	utils_writeTxt(4,13,buff);
    sprintf(buff,"%2d", selected_pressure);
   	utils_writeTxt(4,19,buff);
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

void writeTimerCountdown(int time){
		format_t ftime;
	    char buff[16];

		ftime = utils_getFormatedTime(time);
		sprintf(buff, "Time: %02d.%0d\n", ftime.segs, ftime.dec);
		
		utils_writeTxt(0, 13, buff); 
		
		change_time = false;
}

void writeStateSprite(state_t timer_state, int running_offset) {
	int sprite_fil = 0;
	int sprite_col = 0;
	int page = sprite_fil / 8;
    switch (timer_state) { 
        case READY:
			utils_clearChars(page, sprite_col, 5);
			utils_clearChars((page + 2), sprite_col, 5); 
			ui_drawSprite(stateReady, sprite_fil + 8, sprite_col);
		break;
		
		case RUNNING:
			ui_drawScrolled(stateRunning, sprite_fil, sprite_col, running_offset);
        break;
            
        case STOPPED:
			ui_drawSprite(stateStopped, sprite_fil, sprite_col);
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

void displayPunctureWarning() {
	ui_drawSprite(notifWarning, 40, 0);
	utils_writeTxt(5, 5, "PRESS.");
	utils_writeTxt(6, 5, "LOSS!");
}

bool displayEndAnimation(bool anim_end_on, int anim_end_offset) {
	if (!anim_end_on) return;
	bool write_msg = false;
	if (anim_end_offset <= notifCar.col) {
		int start_col = notifCar.col - anim_end_offset;
		ui_drawSelectedCols(notifCar.matrix, notifCar.fil, notifCar.col, 40, 0, start_col, notifCar.col);
	} else {
		utils_writeTxt(6, 6, "DONE!");
		anim_end_on = false;
	}
	return anim_end_on;
} 

void clearNotifs() {
	clearGLCD(5, 7, 0, 23);
	utils_clearChars(5, 5, 6);
	utils_clearChars(6, 5, 6);
}

void write_temp(double temperature){
	// clearGLCD(2,2, 63, 127);
	char buff[10];
	sprintf(buff, "%5.1f C", temperature);
	utils_writeTxt(0, 5, buff);
}

void write_ambient_pressure(unsigned int read_press){
	// clearGLCD(3,3, 63, 127);
	char buff[10];
	sprintf(buff, "%2d PSI", read_press);
	utils_writeTxt(1, 6, buff);
}

typedef enum  {
	PULLUP,
	PULLDOWN,
} pull_t;

bool button_input_and_anim(byte PREV, byte READ, int pin, pull_t pull, button_t* anim_boton) {
	flanc_t flanc_press = (pull == PULLUP) ? FALLING : RISING;
	
	// si no esta presionado, intentamos detectar flanco de presión
	bool clicked = false;
	if (!anim_boton->pressed) {
		clicked = utils_inputDetector(PREV, READ,  pin, flanc_press);
		if (clicked) {
			// actualizamos animación
			anim_boton->pressed = true;
			anim_boton->change = true;
		}
	// si esta presionado, intentamos detectar flanco de elevación
	} else {
		bool released = utils_inputDetector(PREV, READ,  pin, !flanc_press);
		if (released) {
			// actualizamos animación
			anim_boton->pressed = false;
			anim_boton->change = true;
		}
	}
	// devolvemos si ha habido click
	return clicked;
}

void write_button_labels(){
	utils_writeTxt(6, 13, "SLCT");
	utils_writeTxt(7, 13, "STOP");
}


void main(void)
{ 
	initPIC_config();
	
	GLCDinit();			
	clearGLCD(0,7,0,127);	
	setStartLine(0);		
   
	// animación de inicio
	splash_play();
	
	// variables responsables la detecci�n de flancos
   	uint8_t READ_C = PORTC;
	uint8_t PREV_C = READ_C;
	bool add_pressed = false;

	// variables de animación 
	int anim_running_offset = 0;
	// test animación coche
	bool anim_end_on = false;
	int anim_end_offset = 1;

	button_t boton_resta = ui_button_settup(32, 85, sub);
	button_t boton_suma = ui_button_settup(32, 106, add);
	button_t boton_select = ui_button_settup((6*8)-1, 87, select);
	button_t boton_stop = ui_button_settup((7*8)-1, 87, stop);
	write_button_labels();

	// selecciona la presion y la pone a 50% por defecto junto al medidor
	unsigned int selected_press = 50;

	int adc_channel_values[28];

	// mover a Usart.h
	for (int i = 0; i < 4; i++) {
		if (i == 1) continue;
		usart_1_puts(splash_text[i]);
	}

	// variables detección pinchazo
	int prev_pressure = -1;
	bool punxada = false;

	// inicializa la barra de progreso
	int time_max;
	ui_medidor_steup(8, 65, 50, 0);
	
	// inicializa el estado
	state_t timer_state = READY;
	states_set(timer_state);
	// inicializa texto de estado
    writeStateSprite(timer_state, 0);

	
	// escribe por primera vez presión selected
	write_pressure(selected_press);
	// escribe por primera vez temp y presión ambiental
	write_temp(25.0);
	write_ambient_pressure(0);

	

	while (1)
	{   
		/* ALWAYS */
		/* ALWAYS */

		// adc related update flags to 0
		bool change_temp = false;
		bool change_press = false;
		
		// vale -1 si no ha habido actualizaciones, 
		// valor del canal actualizado en caso de haberlas
		int last_updated_channel = ADC_ConversionLogic(adc_channel_values);
		
		if (last_updated_channel == 6) change_temp = true;
		else if (last_updated_channel == 7) change_press = true;

		
		if (change_temp) {
			double temperature = calculate_temp(adc_channel_values[6]);
			write_temp(temperature);
		}
		
		if (change_press) {
			unsigned int read_press = getReadPressure(adc_channel_values[7]);
			write_ambient_pressure(read_press);
		}

		/* INPUT DIGITAL */
		/* INPUT DIGITAL */

        PREV_C = READ_C; // PREVIO <- ACTUAL
		READ_C = PORTC; // ACTUAL <- PORTC

		bool add_click = button_input_and_anim(PREV_C, READ_C, 0, PULLUP, &boton_suma);
		bool command_add = add_click || d_pressed;
		d_pressed = false;

		bool sub_click = button_input_and_anim(PREV_C, READ_C, 1, PULLDOWN, &boton_resta);
		bool command_sub = sub_click || a_pressed;
		a_pressed = false;

		bool select_click = button_input_and_anim(PREV_C, READ_C, 2, PULLDOWN, &boton_select);
		bool command_sel = select_click || s_pressed;
		s_pressed = false;
		
		bool stop_click = button_input_and_anim(PREV_C, READ_C, 3, PULLDOWN, &boton_stop);
		bool command_stop = stop_click || w_pressed;
		w_pressed = false;

		/* ANIMACIÓN BOTONES */
		/* ANIMACIÓN BOTONES */
		if (boton_suma.change) {
			boton_suma.change = false;
			ui_button_draw(true, boton_suma);
		}

		if (boton_resta.change) {
			boton_resta.change = false;
			ui_button_draw(true, boton_resta);
		}

		if (boton_select.change) {
			boton_select.change = false;
			ui_button_draw(true, boton_select);
		}

		if (boton_stop.change) {
			boton_stop.change = false;
			ui_button_draw(true, boton_stop);
		}

		/* ESTADO RUNNING */
		/* ESTADO RUNNING */
		if (timer_state == RUNNING)	{
			
			// actualiza texto de countdown
			if (change_time) {
				writeTimerCountdown(time_left); 
				
				anim_running_offset--;
				if (anim_running_offset < 0) {
					anim_running_offset = 23;
				}
				writeStateSprite(timer_state, anim_running_offset);
				// ui_drawScrolled(stateRunning, 0, 0, anim_running_offset);
				// scrollSection(1, 0, 3, 24, 0);
				
			}

			// actualiza barra de progreso
			ui_medidor_draw(time_max - time_left, time_max, 0);

			/* RUNNING -> STOPPED */
			bool timer_end = time_left == 0;

			if (command_stop || timer_end) {
				if (timer_end) {
					anim_end_on = true;
					anim_end_offset = 1;
				}
				clearNotifs();
				writeTimerCountdown(time_left);
				timer_state = states_set_next(timer_state);
				writeStateSprite(timer_state, 0);
			}

			// detección y aviso pinchazo
			if (!punxada && check_pressure) {
				if (prev_pressure == -1) {
					prev_pressure = getReadPressure(adc_channel_values[7]);
				} else {
					unsigned int actual_pressure = getReadPressure(adc_channel_values[7]);
					punxada = detectPuncture(prev_pressure, actual_pressure);
					prev_pressure = actual_pressure;

					if (punxada) {
						displayPunctureWarning();

						char buff[32];
						sprintf(buff, "Pinchazo detectado!!!\n");
						usart_1_puts(buff);
						//punxada = false;
					} 
		} 
				}
			}


		/* ESTADO READY */
		/* ESTADO READY */

		else if (timer_state == READY) {
			bool new_selected_press = false;
			// Aumenta presión
			if (command_add){
				new_selected_press = change_selected_pressure(1, &selected_press);
				write_pressure(selected_press);
			} 

			// Disminuye presión
			if (command_sub){
				new_selected_press = change_selected_pressure(-1, &selected_press);
				write_pressure(selected_press);
			}

			// actualiza el valor del contador si es necesario
			if (new_selected_press || change_temp || change_press) {
				time_left = getCompressorTime(adc_channel_values, selected_press);
				writeTimerCountdown(time_left); 
			}

			/* READY -> RUNNING */
			if (command_sel) {

				time_left = getCompressorTime(adc_channel_values, selected_press);
				time_max = time_left;

				change_pwm_profile(selected_press);

				timer_state = states_set_next(timer_state);
				anim_running_offset = 0;
				writeStateSprite(timer_state, anim_running_offset);
				
					/* DEBUG USART LINES */
					/* DEBUG USART LINES */

					char debug[128];
					sprintf(debug, "Tiempo configurado a %02d.%d segundos\n", time_left/10, time_left%10);
					usart_1_puts(debug);
	
			} 
		}

		/* ESTADO STOPPED */
		/* ESTADO STOPPED */

		else if (timer_state == STOPPED){
		
			if (change_time) {
				anim_end_on = displayEndAnimation(anim_end_on, anim_end_offset);
				anim_end_offset++;
			}

			/* STOPPED -> READY */
			if (command_sel) {
				clearNotifs();
				ui_medidor_clear();
				timer_state = states_set_next(timer_state);
				writeStateSprite(timer_state, 0);
				
				// reinicia el tiempo del contador
				time_left = getCompressorTime(adc_channel_values, selected_press);
				writeTimerCountdown(time_left);
			} 
		}
	}
}
