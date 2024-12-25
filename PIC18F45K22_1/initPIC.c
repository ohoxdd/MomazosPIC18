#include "initPIC.h"


void initPIC_config()
{
	T2CONbits.TMR2ON = 0;
	T0CONbits.TMR0ON = 0;
	
	// Port config
	
	ANSELA = 0x00;  				// Port A como digital
	ANSELC = 0x00;  				// Port C como digital
	ANSELB = 0x00;  				// Port B como digital
	ANSELD = 0x00;  				// Port D como digital
	ANSELE = 0xFF;  				// Port E como analogico
	
	// IO Driver config

	TRISC = 0x8F;   				// RC0 y RC1 y RC2 y RC3 como input, RC6 output (TX1), RC7 input (RX1)
	TRISA = 0x00;   				// Port A como ouptut
	TRISD = 0x00;   				// Port D como ouptut
	TRISE = 0x06;   				// AN6 input, AN7 input, los dem�s output
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
		//
		// !!! Aqui tenemos que cambiar los canales pq tenemos que leer del sensor
		// de presion y del de temperatura !!!
		//
		// (0) GO/nDONE: No posem cap conversi� en progr�s
		// (1) ADON: ADC habilitat

	ADCON0bits.CHS = 0x6;

	ADIF= 0;
		// Baixem el flag
	ADIE = 1;
		// Habilitem l'inerrupci� del ADC
	
	
	// Timer0 config para el temporizador
	
	T0CONbits.T08BIT = 0;			// Timer de 16 bits
	T0CONbits.T0CS = 0;				// Usamos Fosc/4 como Clock Source
	T0CONbits.PSA = 0;				// Habilitamos preescalado
	T0CONbits.T0SE = 0;				// Este valor realmente no importa ya que T0CS = 0
	T0CONbits.T0PS = 0b001;			// Preescalamos el valor a factor 1:4

	// CCP PWM config usando Timer2
	
	TRISEbits.RE0 = 0x01; 			// deshabilita output driver
	CCPTMRS0 = 0x00;				// Selecciona TMR2 para el CCP3
	CCP3CON = 0x0C;  				// Configura CCP3 en modo PWM
	
	TMR2IF = 0;      				// Limpia el flag de interrupci�n del Timer2
	TMR2IE = 1;						// Habilitamos el interrupt de timer2
	T2CON = 0x03;    				// Configura el Timer2 con un preescalador de 1:16 y lo habilita
	PR2 = 0x7C;       				// Periodo para 1kHz (con Fosc de 8MHz y preescalador 1:16)
		
		// Empezamos con un valor inicial de presion seleccionada de 50psi. Segun el pdf, esto
		//		debe dejarnos con un duty cycle inicial del 80%
	
		// Duty Ratio = VAL / 4*(124+1)
		// Duty Ratio = VAL / 500 = 80%
		// VAL = 400 = 0x190
		// 2 LSb de 0xFA = 0x0
		// 8 MSb de 0xFA = 0x54
								
	CCP3CON |= ( 0x0<< 4); 
	CCPR3L = 0x54;   				// Inicializa el duty cycle del PWM de CCP3 al 50%

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
