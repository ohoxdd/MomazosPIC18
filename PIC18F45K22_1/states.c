#include "states.h"
#include "compressor.h"
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
