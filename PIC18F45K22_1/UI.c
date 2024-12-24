#include "UI.h"

int medidor_base_f = 60;
int medidor_base_c = 5;

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
