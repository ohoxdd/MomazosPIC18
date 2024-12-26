#include "UI.h"



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

void writeCharMatrixRow(sprite_t sprite, int start_row, int start_col) {
	int fil = sprite.fil;
	int col = sprite.col;
	char* matrix = sprite.matrix;

	int start_page = start_row/8;
	int bit_offset = start_row % 8;

	if (bit_offset == 0){
		int array_ind = 0;
		for (int i = 0; i < fil; ++i) {
			for (int j = 0; j < col; ++j) {
				writeByte(start_page + i, start_col + j, matrix[array_ind]);
				array_ind++;
			}
		}
	} else {

		uint8_t mask = 0xFF >> (8 - bit_offset); 	
		
		for (int j = 0; j < col; ++j) {
			int array_index = j;
			uint8_t read = readByte(start_page, start_col + j); 	//
			uint8_t prev = read & mask; 			
			
			uint8_t draw, res; 

			for (int i = 0; i < fil; ++i ){
				draw = matrix[array_index]; 
				res = draw << bit_offset; 
				res |= prev; 
				writeByte(start_page + i, start_col + j, res);
				array_index += col;

				prev = draw >> (8 - bit_offset); 
				
			}

			int final_page = start_page + fil;
			read = readByte(final_page, start_col + j); 
			read = read & (~mask); 
			res = read | prev; 
			writeByte(final_page, start_col + j, res);
		}
	}
}

writeByteAnywhere(int start_row, int start_col, int draw) {

	int start_page = start_row/8;
	int bit_offset = start_row % 8;

	if (bit_offset == 0){
		writeByte(start_page, start_col, draw);
	} else {
		
		// mascara selecciona bits arriba
		uint8_t mask = 0xFF >> (8 - bit_offset); 	
		
		// -- pagina superior
		// lee bits que habia, los junta con la parte de draw que cabe en la pagina
		uint8_t read = readByte(start_page, start_col); 	
		uint8_t prev = read & mask; 			
		
		
		uint8_t res; 
		res = draw << bit_offset; 
		res |= prev; 
		writeByte(start_page, start_col, res);

		// -- pagina inferior
		// combina los bits desplazados de draw con lo que hay en la pagina inferior
		uint8_t final_page = start_page + 1;
		prev = draw >> (8 - bit_offset); 
		read = readByte(final_page, start_col); 
		read = read & (~mask); 
		res = read | prev; 
		writeByte(final_page, start_col, res);
	}
}
button_id setup_button(button_frame_t button_frames, int fil, int col){
	if (num_buttons == MAX_BUTTONS) return;
	idbutton_frames[num_buttons] = button_frames;
	idbutton_row[num_buttons] = fil;
	idbutton_col[num_buttons] = col;

	button_id id = num_buttons;
	num_buttons++;
	return id;
}

void draw_button(button_id id) {
	button_frame_t frames = idbutton_frames[id];
	int fil = idbutton_row[id];
	int col = idbutton_col[id];
	sprite_t* pbase = frames.base;
	sprite_t* punpressed = frames.unpressed;
	writeCharMatrixRow(*pbase,fil+16,col);
	writeCharMatrixRow(*punpressed,fil,col);
}

bool CheckUpdateButtonAnim(button_id id, bool pressed, int anim_state) {
	button_frame_t frames = idbutton_frames[id];
	int fil = idbutton_row[id];
	int col = idbutton_col[id];

	bool write = false;
	if (pressed && anim_state != PRESS) {
		anim_state++;
		write = true;
	}
	else if (!pressed && anim_state != UNPRESSED) {
		anim_state--;
		write = true;
	}

	if (write){
	sprite_t* pframe;
		switch (anim_state)
		{
		case UNPRESSED:
			pframe = frames.unpressed;
			break;
		case CHANGING:
			pframe = frames.changing;
			break;
		case PRESS:
			pframe = frames.pressed;
			break;
		}
		writeCharMatrixRow(*pframe,fil,col);
	}
	return anim_state;
}

void setup_medidor(int fil, int col, int longit, int pintar){
	datos_medidor.base_f = fil;
	datos_medidor.base_c = col;
	datos_medidor.longitud = longit;
	datos_medidor.pintados = 0;
	// new_update_medidor(pintar, longit, 0);
	int borde_izq = datos_medidor.base_c - 1;
	int borde_der = datos_medidor.base_c + datos_medidor.longitud + 2;
	int borde_sup = datos_medidor.base_f - 1;
	int borde_inf = datos_medidor.base_f + 1;

	writeByte(borde_sup,  borde_izq, 0x80);
	writeByte(borde_sup,  borde_der, 0x80);
	for (int i = borde_izq + 1; i < borde_der; ++i) {
		writeByte(borde_sup,  i, 0x40);
	}
	writeByte(datos_medidor.base_f, borde_izq, 0xFF);
	writeByte(datos_medidor.base_f, borde_der, 0xFF);

	writeByte(borde_inf,  borde_izq, 0x01);
	writeByte(borde_inf,  borde_der, 0x01);
	for (int i = borde_izq + 1; i < borde_der; ++i) {
		writeByte(borde_inf,  i, 0x02);
	}


}

void invertDot(int row, int col) {
	int page = row/8;
	int offset = (row%8);
	int read = readByte (page, col);
	bool set = (read >> offset) & 0x01;
	if (set) {
		ClearDot(row, col);	
	} else {
		SetDot(row, col);	
	}
}

void new_update_medidor(const int valor, const int max_valor, const int min_valor){
	if ((valor > max_valor) || (valor < min_valor)) return;
	int range = max_valor - min_valor;
	int new_height = (datos_medidor.longitud * valor) / range;
	
	int diff = new_height - datos_medidor.pintados;

	int num_its = abs(diff);
	int col = datos_medidor.base_c + datos_medidor.pintados;

	byte draw;
	int change;	
	if (diff > 0) {
		draw = 0xFF;
		change = 1; // positive means one row right
		col++; // we don't need to change the current row
	} else if (diff < 0) {
		draw = 0x00;
		change = -1; // negative means one row left
	} else return; // return if diff = 0

	for (int i = 0; i < num_its; ++i) {
		writeByte(datos_medidor.base_f, col, draw);
		col += change;
	}

	datos_medidor.pintados = new_height;
}



