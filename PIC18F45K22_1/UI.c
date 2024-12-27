#include "UI.h"

button_t setup_button(int fil, int col, char* icon) {
	button_t result;
	result.icon = icon;
	result.fil = fil;
	result.col = col;
	result.pressed = false;
	result.change = true;
	return result;
}


void write_button(bool active, button_t button) {
	// parte superior
	char* icon = button.icon;
	int fil = button.fil;
	int col = button.col;
	bool pressed = button.pressed;
	for (int i = 1; i < 8; ++i) {
		writeByteAnywhere(fil, col + i, 0x01);
	}
	// cuerpo
	byte body;

	if (pressed) {
		body = 0xFF;
	} else {
		body = 0x80;
	}

	for (int j = 1; j < 8; ++j) {
		byte draw = body;
		if ((active) && j > 1 && j < 7) {
			int icon_i = j - 2;
			draw = draw ^ icon[icon_i];
		}
		writeByteAnywhere(fil + 1, col + j, draw);
	}
	// lados
	writeByteAnywhere(fil + 1, col, 0x7f);
	writeByteAnywhere(fil + 1, col + 8, 0x7f);
	
}

void writeSpriteAnywhere(sprite_t sprite, int start_row, int start_col) {
	int filas = sprite.fil;
	int columnas = sprite.col;
	char* matrix = sprite.matrix;

	int array_ind = 0;
	int written_rows = 0;
	for (int i = 0; i < filas; ++i) {
		for (int j = 0; j < columnas; ++j) {
			writeByteAnywhere(start_row + written_rows, start_col + j, matrix[array_ind]);
			array_ind++;
		}
		written_rows += 8;
	} 
}

void writeSpriteOffset(sprite_t sprite, int start_row, int start_col, int offset) {
	int filas = sprite.fil;
	int columnas = sprite.col;
	char* matrix = sprite.matrix;

	
	int written_rows = 0;
	for (int i = 0; i < filas; ++i) {
		// pone el indice a la fila correspondiente
		int arr_page_index = i * columnas;
		for (int j = 0; j < columnas; ++j) {
			// desplaza el indice, cuando llega sobrepasa el numero de filas vuelve al principio;
			int array_ind = arr_page_index + (offset + j) % columnas;
			writeByteAnywhere(start_row + written_rows, start_col + j, matrix[array_ind]);
			// array_ind++;
		}
		written_rows += 8;
	} 
}

void scrollSection(int start_page, int start_col, const int pages, int cols, int offset){
	uint8_t aux_pages[8];

	int aux_col;

	/* if (offset < 0) {
		aux_col = start_col;
	} else {
		aux_col = start_col + cols - 1;
	} */

	// save rightmost
	aux_col = start_col + cols - 1;

	for (int i = 0; i < pages; i++) {
		aux_pages[i] = readByte(start_page+i,aux_col);
	}

	for (int i = cols; i > 0; i--) {
		for (int j = 0; j < pages; j++) {
			// read from left
			int current_col = start_col+i;
			int current_page = start_page+j;
			uint8_t read = readByte(current_page, current_col-1);
			// write to right
			writeByte(current_page, current_col, read);
		}
	}

	for (int i = 0; i < pages; i++) {
		writeByte(start_page+i, start_col, aux_pages[i]);
	}
}

void writeByteAnywhere(int start_row, int start_col, int draw) {

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

void setup_medidor(int fil, int col, int longit, int pintar){
	datos_medidor.base_f = fil+2;
	datos_medidor.base_c = col+1;
	datos_medidor.longitud = longit;
	datos_medidor.pintados = 0;

	int borde_izq = datos_medidor.base_c - 1;
	int borde_der = datos_medidor.base_c + datos_medidor.longitud + 2;
	int borde_sup = datos_medidor.base_f - 8;
	int borde_inf = datos_medidor.base_f + 8;

	writeByteAnywhere(borde_sup,  borde_izq, 0x80);
	writeByteAnywhere(borde_sup,  borde_der, 0x80);
	for (int i = borde_izq + 1; i < borde_der; ++i) {
		writeByteAnywhere(borde_sup,  i, 0x40);
	}
	writeByteAnywhere(datos_medidor.base_f, borde_izq, 0xFF);
	writeByteAnywhere(datos_medidor.base_f, borde_der, 0xFF);

	writeByteAnywhere(borde_inf,  borde_izq, 0x01);
	writeByteAnywhere(borde_inf,  borde_der, 0x01);
	for (int i = borde_izq + 1; i < borde_der; ++i) {
		writeByteAnywhere(borde_inf,  i, 0x02);
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

void clear_medidor() {
	int fil = datos_medidor.base_f;
	int col = datos_medidor.base_c  + 1;
	for (int i = 0; i < datos_medidor.longitud; ++i) {
		writeByteAnywhere(fil, col + i, 0x00);
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
		writeByteAnywhere(datos_medidor.base_f, col, draw);
		col += change;
	}

	datos_medidor.pintados = new_height;
}



