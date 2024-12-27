int ANSELA = 0;
int ANSELB = 0;
int ANSELC = 0;
int ANSELD = 0;
int PORTC = 0;
int TRISA=0x03; 		
int TRISD=0x00;
int TRISB=0x00;
int PORTD=0x00;
int PORTB=0x00;  
int IPEN = 1;				
int GIEH = 1;				
int GIEL = 1;		
int TRISE;
int CCPTMRS0;
unsigned char RCREG1;

typedef struct type2{
    int CCP3;
    int C3TSEL;
    int RE0;
    int TMR2ON;
    int RC0;
    int RC1;
	int CHS;
} type2;
type2 ADCON0bits;
type2 PORTCbits;
type2 TRISEbits;
type2 CCPTMRS0bits;
type2 T2CONbits;
int ADIF, ADIE;

int GO_nDONE;

typedef struct PIR1bits {
	int RCIF;
} skibidi;
typedef struct PIE1bits {
	int RC1IE;
} toilet;
skibidi PIR1bits;
toilet PIE1bits;

typedef struct T0CONbits  {
int T08BIT;
int T0CS;
int PSA;
int T0SE;
int T0PS;
int TMR0ON;
} type;		
type T0CONbits;
int PORTA;
int TMR0IF = 0;				
int TMR0IP = 1;				
int TMR0IE = 1;				
int TMR0H = 'TIMER_STARTH';
int TMR0L = 'TIMER_STARTL';
int TMR0ON = 1;
int ADRESH, ADRESL;
int TMR2IE;
#define interrupt
#define high_priority;
#define low_priority;
int ANSELE;
int TRISC;
int CCP3CON;
int CCPR3L;
int TMR2IF;
int T2CON;
int PR2;

void __delay_ms(int s);
void __delay_us(int s);
