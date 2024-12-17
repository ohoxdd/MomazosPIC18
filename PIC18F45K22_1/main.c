/* Main.c 
 * Processor: PIC18F45K22
 * Compiler:  MPLAB XC8
 */

#include "xc.h"
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

typedef enum{
 Ready,
 Running,
 Stopped
}state_t;

// Varibales globales 
unsigned int time_left = TIEMPO_INICIAL;
unsigned char change_time = 1;
unsigned int pressure_perc;
unsigned int adjust_pressure;
int duty_one_pc;
uint16_t adc_value;
bool change_temp;
bool w_pressed, a_pressed, s_pressed, d_pressed;

void configPIC()
{
	T0CONbits.TMR0ON = 0;
	T2CONbits.TMR2ON = 0;
	
	T0CONbits.TMR0ON = 0;
	
	// Port config
	
	ANSELA = 0x00;  				// Port A como digital
	ANSELC = 0x00;  				// Port C como digital
	ANSELB = 0x00;  				// Port B como digital
	ANSELD = 0x00;  				// Port D como digital
	ANSELE = 0xFF;  				// Port E como analogico
	
	// IO Driver config

	TRISC = 0x87;   				// RC0 y RC1 y RC2 como input, RC6 output (TX1), RC7 input (RX1)
	TRISA = 0x00;   				// Port A como ouptut
	TRISD = 0x00;   				// Port D como ouptut
	TRISE = 0x02;   				// AN6 input, los dem�s output
	TRISEbits.RE1 = 1;				
	TRISB = 0x00;   				// Port B como ouptut
	
	// Init port state

	PORTA = 0x00;   				// Port C a 0
	PORTC = 0x00;   				// Port C a 0
	PORTD = 0x00;   				// Port D a 0
	PORTB = 0x00;   				// Port B a 0
	
	// Interrrupt config
	
	IPEN = 0;						// Disable  all priorities
	GIE = 1;						// Enable all interrupts
	PEIE = 1;						// Enable periferal interrupts
	
	// ADC config
	
	ADCON2 = 0x89;  				
		// (1)0(001)(001) 
		// (1) ADFM: Posa el resultat justificat a la dreta
		// (001) ACQT: Posem el temps d'acquisici� a 2TAD
		// (001) ADCS: Conversion Clock = Fosc/8, estableix el periode de clock de conversi� (TAD)

		// TAD = 8/FOSC = 1us >= 1us (limit del fabricant)
		// TACQ = 2 TAD = 2us > 1.4 us (limit del fabricant)

    ADCON1 = 0;     				
		// 0000(00)(00) Referencies
		// (00) PVCFG: Triem de referencia positiva VDD
		// (00) NVCFG: Triem de referencia negativa VSS
    ADCON0 = 0x19;
		// 0(00110)(0)(0)    
		// (00110) CHS: Triem el canal AN6
		// (0) GO/nDONE: No posem cap conversi� en progr�s
		// (1) ADON: ADC habilitat
	ADIE = 1;
		// Habilitem l'inerrupci� del ADC
	ADIF= 0;
		// Baixem el flag
	
	
	// Timer0 config para el temporizador
	
	T0CONbits.T08BIT = 0;			// Timer de 16 bits
	T0CONbits.T0CS = 0;				// Usamos Fosc/4 como Clock Source
	T0CONbits.PSA = 0;				// Habilitamos preescalado
	T0CONbits.T0SE = 0;				// Este valor realmente no importa ya que T0CS = 0
	T0CONbits.T0PS = 0b001;			// Preescalamos el valor a factor 1:4

	// CCP PWM config usando Timer2
	
	TRISEbits.RE0 = 0x01; 			// deshabilita output driver
	CCPTMRS0 = 0x00;				// Selecciona TMR2 para el CCP3
	CCP3CON =0x0C;  				// Configura CCP3 en modo PWM
	
	TMR2IF = 0;      				// Limpia el flag de interrupci�n del Timer2
	T2CON = 0x03;    				// Configura el Timer2 con un preescalador de 1:16 y lo habilita
	PR2 = 0x7C;       				// Periodo para 1kHz (con Fosc de 8MHz y preescalador 1:16)

		// Duty Ratio = VAL / 4*(124+1)
		// Duty Ratio = VAL / 500 = 50%
		// VAL = 250 = 0xFA
		// 2 LSb de 0xFA = 0x2
		// 8 MSb de 0xFA = 0x3E
								
	CCP3CON |= ( 0x2<< 4); 
	CCPR3L = 0x3E;   				// Inicializa el duty cycle del PWM de CCP3 al 50%

	duty_one_pc = ((PR2 + 1)*4)/100; // calculo para el valor de un porcentage del duty cycle

	TMR2IE = 1;						// Habilitamos el interrupt de timer0
	TMR0IF = 0;						// Bajamos el flag de timer	
	TMR0IE = 1;						// Habilitamos el interrupt de timer0

	// USART config
	
	RCSTA1 = 0x90;
		// SPEN = 1, serial port enable
		// CREN = 1, habilitamos receptor
	
	BAUDCON = 0x08; // 0b00001000
		// Baud rate 16 bits (SPBRGH y SPBRG)
	TXSTA1 = 0x24;
		// TX9 = 0, 8 bits
		// TXEN = 1, habilitamos transmisor
		// SYNC = 0, asincrono
		// SENDBG ?
		// BRGH = 1, high speed
		// TRMT ?
	
	SPBRGH = 0x00;
	SPBRG = 0x10;
		// FORMULA BAUDS
		// 8Mhz / [4 (n+1)] = 115200
		// n = 16.3611
		// baud rate = 8*10e6 / [4 (16+1)]  = 117467 aprox
		// error = (117467 - 115200) / 115200 
		// error = 2.124% < 7% (error aceptable)
	PIE1bits.RC1IE = 1;
}

#define VCC_VAL 1023.0
#define R2_VALUE 4751.0
#define A_VALUE 0.0059257
#define B_VALUE 4050.0
#define K_ABS_ZERO 273.15

void tic(void)
{
	time_left--;
	change_time = 1;
}

void interrupt RSI(){
	if (TMR2IF == 1 && TMR2IE == 1) {
		TMR2IF = 0;
		TMR2IE = 0;
	}
	
	if (TMR0IF == 1 && TMR0IE == 1) {
		// avanzamos el timer para que OVF ocurra en 0.1s
		// asignamos al buffer de TMR0H el valor de timer_starth
		// sumamos timer_startl a tmr0l, (el buffer high escribe en TMR0H)  
		// para tener en cuenta las instrucciones pasadas entre el OVF y la asignaci�n
		
		TMR0H = TIMER_STARTH;
		TMR0L += TIMER_STARTL;
		
		tic();

		TMR0IF = 0;
	}
	
	if (ADIF == 1 && ADIE == 1) {
		ADIF = 0;
		
		uint16_t result = (ADRESH << 8) | ADRESL;
		
		change_temp = (result != adc_value);
		
		adc_value = result;		
	}
	
	if (PIR1bits.RCIF && PIE1bits.RC1IE) {
		PIE1bits.RC1IE = 0;
		unsigned char input = RCREG1; // esto levanta ya la flag
		switch (input) {
			case 'w': {w_pressed = true; break;}
			case 'a': {a_pressed = true; break;}
			case 's': {s_pressed = true; break;}
			case 'd': {d_pressed = true; break;}
		}
		PIE1bits.RC1IE = 1;
	}
}

char updateTimerText(state_t timer_state, char timer_changed_state) {
    format_t ftime;

	char countdown[256];
	char stateText[50];
  
	// Update del contador si esta activo
    if (timer_state == Running && change_time) {
		
		ftime = getFormatedTime(time_left);
		sprintf(countdown, "%02d.%0d\n", ftime.segs, ftime.dec);
		
		writeTxt(0, 20, countdown); 
		
		change_time = 0;
    }
  
	// Update segun los estados
	if (timer_changed_state) {
		clearGLCD(1,1,60,127); // Clear del texto "Ready"/"Running..."/"Stopped"
		
		// se ha tratado el cambio de estado
		timer_changed_state = false;
		
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
	
	return timer_changed_state;
}

state_t next_timer_state(state_t state) {
	state_t new_state;
	switch (state) {
	case Ready: 
		// READY ---> RUNNING
		new_state = Running;
		
		T2CONbits.TMR2ON = 1;
		T0CONbits.TMR0ON = 1;
		break;
		
	case Running: 
		// Caso 2: RUNNING ---> STOPPED
		// Se presiona RC2 
		new_state = Stopped;
		// timer_end = false;
		
		T2CONbits.TMR2ON = 0;
		T0CONbits.TMR0ON = 0;
		break;
		
	case Stopped: 
		// STOPPED ---> READY
		time_left = TIEMPO_INICIAL;
		
		TMR0H = TIMER_STARTH;
		TMR0L = TIMER_STARTL;
		
		new_state = Ready;
		break;
	}
	return new_state;
}

char* splash_text[] = {
		TITOL,
		"-------------\n",
		NOM1,
		NOM2
};

void play_splash_screen() {
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

 double calculate_temp(const double precalc) {
	
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
	result = (VCC_VAL / adc_value) - 1;
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

enum flanc_t {
	FALLING,
	RISING, 
};

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

void main(void)
{ 
	configPIC();
	GLCDinit();			//Inicialitzem la pantalla
	clearGLCD(0,7,0,127);	//Esborrem pantalla
	setStartLine(0);		//Definim linia d'inici
   
	play_splash_screen();
	
	TRISEbits.RE0 = 0; // Enable output driver
	
	// variables responsables la detecci�n de flancos
   	uint8_t READ_C = PORTC;
	uint8_t PREV_C = READ_C;

	bool RC0_pressed = !PORTCbits.RC0;
	bool RC1_pressed = PORTCbits.RC1;
	
	bool RC0_update = true;
	bool RC1_update = true;
	
	state_t timer_state = Ready;
	T0CONbits.TMR0ON = 0;
	T2CONbits.TMR2ON = 0;
	
	bool timer_changed_state = true;
	bool timer_end = false;
	
	// selecciona la presion y la pone a 50% por defecto junto al medidor
	pressure_perc = 50;
	update_medidor(0, 50);

	// Variable que dicta si se escribe la temperatura en pantalla 
	// y se recalcula la presion ajustada a la temperatura
	change_temp = true;
	double temperature = 25.0;
	int adjusted_pressure = pressure_perc;

	// Valor constante precalculado para simplificar calculo de la temperatura
	const double precalc = log( R2_VALUE / A_VALUE);
	int val = 0;
	
	for (int i = 0; i < 4; i++) {
		if (i == 1) continue;
		puts_usart1(splash_text[i]);
	}
	
	/*char buffer[128];
	gets_usart1(buffer);
	puts_usart1(buffer);*/

	while (1)
	{   
		// Inicia una nueva conversi�n del adc cuando no hay una en curso
		if (!GO_nDONE) {
			GO_nDONE = 1;
		}
		
		if (change_temp) {
			// escribe temperatura
			clearGLCD(2,2, 63, 127);
			char buff[128];
			temperature = calculate_temp(precalc);
			sprintf(buff, "%.2f C", temperature);
			writeTxt(2, 16, buff);

			// ajusta la presion segun la temperatura
			adjusted_pressure = get_adjusted_pressure(temperature);
			set_pwm_pressure(adjusted_pressure);
			write_pressure(adjusted_pressure);
		}
		
		READ_C = PORTC;
		
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

		// Caso 1: RUNNING ---> STOPPED
		// Timer llega a 0
		if (timer_state == Running && time_left == 0) {
			timer_end = true;
		}
		
		if (inputDetector(PREV_C, READ_C, 2, 0) || timer_end) {
			if (timer_state == Running) {
				timer_end = false;
			}
			// logica de los casos de transici�n del timer
			timer_state = next_timer_state(timer_state);
			timer_changed_state = true;
		}
		// Actualiza la GLCD segun los estados del Timer, y el contador.
		// Devuelve el nuevo booleano segun si se ha tratado o no el cambio o no.
		timer_changed_state = updateTimerText(timer_state, timer_changed_state);

		// Actualiza el estado previo de los botones del PORTC
		PREV_C = READ_C;

		// Actualiza los textos en pantalla para los botones RC0 RC1
		if (RC0_update) {
			RC0_update = false;
			sprintf(buff, "RC0 pressed: %d\n", RC0_pressed);
			writeTxt(5, 11, buff);
		}
		if (RC1_update) {
			RC1_update = false;
			sprintf(buff, "RC1 pressed: %d\n", RC1_pressed);
			writeTxt(6, 11, buff);
		}
		
		val = sum(val,1);
		val++;
		
	}
}
