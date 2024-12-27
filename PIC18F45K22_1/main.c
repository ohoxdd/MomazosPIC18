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
unsigned int adjust_pressure;

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
// Lo que hacemos es escalar el valor 90/1023 a 58982/2^16 para poder hacer
// una division por potencia de 2.  

unsigned int getReadPressure(unsigned long int adc_press) {
	//return ((adc_press*58982)/65536);
	return (adc_press*88)/1000;
	// return ((adc_press*58982) >> 16) no estoy seguro de que esto funcione
	// a lo mejor el compilador hace su magia pero meh, no me quiero arriesgar
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
	writeTxt(4,13,buff);
    sprintf(buff,"%2d", selected_pressure);
   	writeTxt(4,19,buff);
}

void change_pwm_values(struct DC_values values) {
	CCP3CON |= (values.LSb << 4);
	CCPR3L = values.MSb;
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

void write_adc_values(bool change_temp, bool change_press, int adc_values_arr[28]) {
	char buff[16];
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



void writeTimerCountdown(int time){
		format_t ftime;
	    char buff[16];

		ftime = getFormatedTime(time);
		sprintf(buff, "Time: %02d.%0d\n", ftime.segs, ftime.dec);
		
		writeTxt(0, 13, buff); 
		
		change_time = false;
}


void updateStateTextTimer(state_t timer_state) {
	char stateText[50];

	int fil = 0;
	int col = 0;

	int sprite_fil = 0;
	int sprite_col = 0;
  
	// Update segun los estados
    // clearChars(fil,col,10); // Clear del texto "Ready"/"Running..."/"Stopped"
    
    switch (timer_state) { // EL VALOR ACTUAL DEL STATUS
        case READY:

            sprintf(stateText, "Ready\n");
			int page = sprite_fil/8;
			clearChars(page, sprite_col, 5);
			clearChars(page+2, sprite_col, 5);
			writeSpriteAnywhere(stateReady, sprite_fil + 8, sprite_col);
            // writeTxt(fil,col, stateText); 
            break;
            
        case RUNNING:
            // Aqui, como el countdown de running se tiene que actualizar cada decima de segundo
            // solo nos preocupamos de escribir el texto del estado de "Running..."
            
            sprintf(stateText, "Running...\n"); 
            // writeTxt(fil,col, stateText);

			// writeSpriteAnywhere(stateRunning, sprite_fil, sprite_col);
            break;
            
        case STOPPED:
            // Es necesario escribir el valor del timer en stopped 
            // para tener el valor correcto en caso de pausa

            sprintf(stateText, "Stopped!\n");
            // writeTxt(fil,col, stateText);

			writeSpriteAnywhere(stateStopped, sprite_fil, sprite_col);
            
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
	writeSpriteAnywhere(notifWarning, 40, 0);
	writeTxt(5, 5, "PRESS.");
	writeTxt(6, 5, "LOSS!");
}

void clearNotifs() {
	clearGLCD(5, 7, 0, 23);
	clearChars(5, 5, 6);
	clearChars(6, 5, 5);
}

write_temp(double temperature){
	// clearGLCD(2,2, 63, 127);
	char buff[10];
	sprintf(buff, "%5.1f C", temperature);
	writeTxt(0, 5, buff);
}

write_ambient_pressure(unsigned int read_press){
	// clearGLCD(3,3, 63, 127);
	char buff[10];
	sprintf(buff, "%2d PSI", read_press);
	writeTxt(1, 6, buff);
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
		clicked = inputDetector(PREV, READ,  pin, flanc_press);
		if (clicked) {
			// actualizamos animación
			anim_boton->pressed = true;
			anim_boton->change = true;
		}
	// si esta presionado, intentamos detectar flanco de elevación
	} else {
		bool released = inputDetector(PREV, READ,  pin, !flanc_press);
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
	writeTxt(6, 13, "SLCT");
	writeTxt(7, 13, "STOP");
}


void main(void)
{ 
	initPIC_config();
	
	GLCDinit();			//Inicialitzem la pantalla
	clearGLCD(0,7,0,127);	//Esborrem pantalla
	setStartLine(0);		//Definim linia d'inici
   
	// animación de inicio
	splash_play();
	
	// variables responsables la detecci�n de flancos
   	uint8_t READ_C = PORTC;
	uint8_t PREV_C = READ_C;
	bool add_pressed = false;

	// variable de animación de estados
	int anim_running_offset = 0;

	button_t boton_resta = setup_button(32, 85, sub);
	button_t boton_suma = setup_button(32, 106, add);
	button_t boton_select = setup_button((6*8)-1, 87, select);
	button_t boton_stop = setup_button((7*8)-1, 87, stop);
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
	setup_medidor(8, 65, 50, 0);
	
	// inicializa el estado
	state_t timer_state = READY;
	states_set(timer_state);
	// inicializa texto de estado
    updateStateTextTimer(timer_state);

	int time_max;
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
			write_button(true, boton_suma);
		}

		if (boton_resta.change) {
			boton_resta.change = false;
			write_button(true, boton_resta);
		}

		if (boton_select.change) {
			boton_select.change = false;
			write_button(true, boton_select);
		}

		if (boton_stop.change) {
			boton_stop.change = false;
			write_button(true, boton_stop);
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
				writeSpriteOffset(stateRunning, 0, 0, anim_running_offset);
				// scrollSection(1, 0, 3, 24, 0);
				
			}

			// actualiza barra de progreso
			new_update_medidor(time_max - time_left, time_max, 0);

			/* RUNNING -> STOPPED */
			bool timer_end = time_left == 0;

			if (command_stop || timer_end) {
clearNotifs();
				writeTimerCountdown(time_left);
				timer_state = states_set_next(timer_state);
				updateStateTextTimer(timer_state);
			}

			// detección y aviso pinchazo
			if (check_pressure) {
				if (prev_pressure == -1) {
					prev_pressure = getReadPressure(adc_channel_values[7]);
				} else {
					unsigned int actual_pressure = getReadPressure(adc_channel_values[7]);
					punxada = detectPuncture(prev_pressure, actual_pressure);
					prev_pressure = actual_pressure;
				}
			}

			if (punxada) {
				// displayPunctureWarning();
				// Como funciona esto?
				//
				displayPunctureWarning();

				char buff[32];
				sprintf(buff, "Pinchazo detectado!!!\n");
				usart_1_puts(buff);
				punxada = false;
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
				updateStateTextTimer(timer_state);
				
					/* DEBUG USART LINES */
					/* DEBUG USART LINES */

					char debug[128];
					sprintf(debug, "Tiempo configurado a %02d.%d segundos\n", time_left/10, time_left%10);
					usart_1_puts(debug);
					/* unsigned int read_press = getReadPressure(adc_channel_values[7]);
					sprintf(debug, "Diferencia de presion = %d - %d = %d\n", pressure_perc, read_press, pressure_perc - read_press);
					usart_1_puts(debug);
					//sprintf(debug, "Formula:\n");
					usart_1_puts(debug);
					sprintf(debug, "s = (%d-%d)/2 - (%d - 25)\n", pressure_perc, read_press, (int)calculate_temp(adc_channel_values[6]));
					usart_1_puts(debug); */	
			} 
		}

		/* ESTADO STOPPED */
		/* ESTADO STOPPED */

		else if (timer_state == STOPPED){

			/* STOPPED -> READY */
			clearNotifs();

			if (command_sel) {
				clearNotifs();
				clear_medidor();
				timer_state = states_set_next(timer_state);
				updateStateTextTimer(timer_state);

				// reinicia el tiempo del contador
				time_left = getCompressorTime(adc_channel_values, selected_press);
				writeTimerCountdown(time_left);
			} 
		}
	}
}
