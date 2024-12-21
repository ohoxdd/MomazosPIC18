/* Main.c 
 * Processor: PIC18F45K22
 * Compiler:  MPLAB XC8
 */

//#include "xc.h"
#include <xc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "GLCD.h"
#include "test.h"
#include "splash.h"
#include "utils.h"
#include <stdint.h>
#include <math.h>
#include "initPIC.h"
#include <stdbool.h>

#define _XTAL_FREQ 8000000  
#define TIMER_STARTL 0xB0
#define TIMER_STARTH 0x3C
#define TIEMPO_INICIAL 100
#define MIN_PRESSURE 15
#define MAX_PRESSURE 100
#define TITOL "L7 Projecte\n"
#define NOM1 "Angel A. Quinones\n"
#define NOM2 "Hector Fdez. de Sevilla\n"

// Varibales globales
bool splash_on = 0;
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

#define VCC_VAL 1023.0
#define R2_VALUE 4751.0
#define A_VALUE 0.0059257
#define B_VALUE 4050.0
#define K_ABS_ZERO 273.15

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
	// change_temp = (result != adc_value);
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

		// No soy consicente de hasta que punto es necesario 
		// bajar el enable ya que ahora solo analizamos un char a la vez,
		// el enable lo bajaba cuando estaba procesando string inputs del USART.

		PIE1bits.RC1IE = 0;
		handleUsartInput();
		PIE1bits.RC1IE = 1;
	}
}

void updateRunningTimer(state_t timer_state){
    if (timer_state == Running && change_time) {
		format_t ftime;
	    char buff[128];

		ftime = getFormatedTime(time_left);
		sprintf(buff, "%02d.%0d\n", ftime.segs, ftime.dec);
		
		writeTxt(0, 20, buff); 
		
		change_time = 0;
    }
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
            
            ftime = getFormatedTime(time_left);
            sprintf(countdown, "%02d.%0d\n", ftime.segs, ftime.dec);

            
            writeTxt(0, 20, countdown); 
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
            
            ftime = getFormatedTime(time_left);
            sprintf(countdown, "%02d.%0d\n", ftime.segs, ftime.dec);

            
            writeTxt(0, 20, countdown); 
            writeTxt(1,16, stateText);
            
            break;
    }
}


void set_state(state_t state) {
    switch (state) {
        case Running: 
            // READY ---> RUNNING
			PORTA = 0x2;
            TRISEbits.RE0 = 0; // Enable output driver
            T2CONbits.TMR2ON = 1;
            T0CONbits.TMR0ON = 1;
            break;
            
        case Stopped: 
            // RUNNING ---> STOPPED
			PORTA = 0x4;
            T2CONbits.TMR2ON = 0;
            T0CONbits.TMR0ON = 0;
            TRISEbits.RE0 = 1; // Disable output driver
            break;
            
        case Ready: 
            // STOPPED ---> READY
			PORTA = 0x1;
            time_left = TIEMPO_INICIAL;
            TMR0H = TIMER_STARTH;
            TMR0L = TIMER_STARTL;
            break;
    }
}

state_t set_next_state(state_t state){
	state_t new_state;

	switch (state) {
		case Ready: 	{new_state = Running; break;}
		case Running:	{new_state = Stopped; break;}
		case Stopped: 	{new_state = Ready; break;}
	}

    set_state(new_state);
	return new_state;
}

char* splash_text[] = {
		TITOL,
		"-------------\n",
		NOM1,
		NOM2
};

void play_splash_screen() {
	if (!splash_on) {
        return;
    }

    writeMatrix(0, 0, 8, 128, splash_bitmap);
    __delay_ms(2000);
    clearGLCD(0, 8, 0, 128);
    int n = sizeof(splash_text)/sizeof(splash_text[0]);
    for (int i = 0; i < n; i++) {
        int column = calc_center_spacing(splash_text[i]);
        writeTxt(2+i, column, splash_text[i]); 
    }
    __delay_ms(2000);
    clearGLCD(0,7,0,127); 
}

 double calculate_temp(const double precalc, int adc_temp) {
	
	// factorizamos R2/A
	//
	//    -1 * R2 * (1 - VCC/ VOUT) 
	//  -------------------------------
	//               A
	// 		= R2/A * (VCC/VOUT - 1)

	// por propiedades de logaritmos
	//
	// 		ln (R2/A * (VCC/VOUT - 1))
	// 		= ln(R2/A) + ln(VCC/VOUT - 1)

	// usamos ln(R2/A) previamente calculado

	double result;
	result = (VCC_VAL / adc_temp) - 1;
	result = B_VALUE / (precalc + log(result));
	result -= K_ABS_ZERO;
	return result; 

} 

void write_pressure(const int adjusted_pressure){
	char buff[256];
    sprintf(buff,"%3d adj:%3d",pressure_perc, adjusted_pressure);
	
	clearChars(0,0,11);
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


char inputDetector(uint8_t REG_ant, uint8_t REG_act, char num_pin, char flanc) {
	char ret = 0;

	char pin_act = (REG_act >> num_pin) & 0x01;
	
	char pin_ant = (REG_ant >> num_pin) & 0x01;

	if (flanc == RISING) {
		ret = pin_act && !pin_ant;
	} else {
		ret = !pin_act && pin_ant;
	}
	
	if (ret) __delay_ms(10);
	
	
	return ret;
}


int medidor_base_f = 60;
int medidor_base_c = 5;

#define ANCHURA_MEDIDOR 10
void update_medidor(int current_psi, int change_psi){
	int new_psi = current_psi + change_psi;
	if ((new_psi > MAX_PRESSURE) || (new_psi < MIN_PRESSURE)) return;

	int height = current_psi/2;
	int new_height = new_psi/2;

	int diff = new_height - height;

	int num_its = abs(diff);
	int row = medidor_base_f - height;

	int change;
	bool increase;
	if (diff > 0) {
		increase = true; // negative means one row up
		change = -1;
	} else if (diff < 0) {
		increase = false; // positive means one row down
		change = 1;
	} else return; // return if diff = 0

	for (int i = 0; i < num_its; ++i) {
		for (int j = 0; j < ANCHURA_MEDIDOR; j++) {
			if (increase)
			SetDot(row, medidor_base_c+j);
			else
			ClearDot(row, medidor_base_c+j);
		}
		row += change;
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
	int adjusted_pressure = pressure_perc;

	// Valor constante precalculado para simplificar calculo de la temperatura
	const double precalc = log( R2_VALUE / A_VALUE);
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
			write_pressure(adjusted_pressure);
		}
		
        PREV_C = READ_C; // PREVIO <- ACTUAL
		READ_C = PORTC; // ACTUAL <- PORTC
		
		char buff[128];

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
			write_pressure(adjusted_pressure);


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
			write_pressure(adjusted_pressure);


		} else if (RC1_pressed && inputDetector(PREV_C, READ_C,  1, FALLING)){
			RC1_pressed = false;
			RC1_update = true;
		}


		bool timer_end = timer_state == Running && time_left == 0;
		
		if (inputDetector(PREV_C, READ_C, 2, 0) || timer_end) {
			timer_state = set_next_state(timer_state);
            updateStateTextTimer(timer_state);
		}
        updateRunningTimer(timer_state);
		

		// Actualiza los textos en pantalla para los botones RC0 RC1
		// if (RC0_update) {
		// 	RC0_update = false;
		// 	sprintf(buff, "RC0 pressed: %d\n", RC0_pressed);
		// 	writeTxt(5, 11, buff);
		// }
		// if (RC1_update) {
		// 	RC1_update = false;
		// 	sprintf(buff, "RC1 pressed: %d\n", RC1_pressed);
		// 	writeTxt(6, 11, buff);
		// }
		
		// val = sum(val,1);
		// val++;
		
	}
}
