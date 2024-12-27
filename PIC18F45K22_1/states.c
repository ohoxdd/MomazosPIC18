#include "states.h"

void states_set(state_t state) {
    switch (state) {
        case RUNNING: 
            // READY ---> RUNNING
			

			PORTA = 0x2;
            TRISEbits.RE0 = 0; // Enable output driver
            T2CONbits.TMR2ON = 1;

            TMR0H = TIMER_STARTH;
            TMR0L = TIMER_STARTL;
            T0CONbits.TMR0ON = 1;
            break;
            
        case STOPPED: 
            // RUNNING ---> STOPPED
			PORTA = 0x4;
            T2CONbits.TMR2ON = 0;
            TRISEbits.RE0 = 1; // Disable output driver

            TMR0H = TIMER_STARTH;
            TMR0L = TIMER_STARTL;
            break;
            
        case READY: 
            // STOPPED ---> READY
			PORTA = 0x1;
            
            T0CONbits.TMR0ON = 0;
            break;
    }
}

state_t states_set_next(state_t state){
	state_t new_state;

	switch (state) {
		case READY: 	{new_state = RUNNING; break;}
		case RUNNING:	{new_state = STOPPED; break;}
		case STOPPED: 	{new_state = READY; break;}
	}

    states_set(new_state);
	return new_state;
}
