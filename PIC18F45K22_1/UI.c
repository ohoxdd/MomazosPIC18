#include "UI.h"

button_t ui_button_settup(int fil, int col, char* icon) {
	button_t result;
	result.icon = icon;
	result.fil = fil;
	result.col = col;
	result.pressed = false;
	result.change = true;
	return result;
}


void ui_button_draw(bool active, button_t button) {
	// parte superior
	char* icon = button.icon;
	int fil = button.fil;
	int col = button.col;
	bool pressed = button.pressed;
	for (int i = 1; i < 8; ++i) {
		ui_writeByteAnywhere(fil, col + i, 0x01);
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
		ui_writeByteAnywhere(fil + 1, col + j, draw);
	}
	// lados
	ui_writeByteAnywhere(fil + 1, col, 0x7f);
	ui_writeByteAnywhere(fil + 1, col + 8, 0x7f);
}

void ui_drawSprite(sprite_t sprite, int start_row, int start_col) {
	int filas = sprite.fil;
	int columnas = sprite.col;
	char* matrix = sprite.matrix;

	int array_ind = 0;
	int written_rows = 0;
	for (int i = 0; i < filas; ++i) {
		for (int j = 0; j < columnas; ++j) {
			ui_writeByteAnywhere(start_row + written_rows, start_col + j, matrix[array_ind]);
			array_ind++;
		}
		written_rows += 8;
	} 
}

void ui_drawSelectedCols(char* matrix, int f_array, int c_array,  int f_glcd, int c_glcd, int c_ini, int c_fin) {
	// int f_array = sprite.fil;
	// int c_array = sprite.col;
	// char* matrix = sprite.matrix;

	int written_rows = 0;
	int ncols = c_fin  - c_ini;
	for (int i = 0; i < f_array; ++i) {
		// comienza en fila i
		// avanza hasta la columna c_ini
		int array_ind = i * c_array + c_ini;
		for (int j = 0; j < ncols; ++j) {
			ui_writeByteAnywhere(f_glcd + written_rows, c_glcd + j, matrix[array_ind]);
			array_ind++;
		}
		written_rows += 8;
	} 
}

void ui_drawScrolled(sprite_t sprite, int start_row, int start_col, int offset) {
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
			ui_writeByteAnywhere(start_row + written_rows, start_col + j, matrix[array_ind]);
			// array_ind++;
		}
		written_rows += 8;
	} 
}

void ui_writeByteAnywhere(int start_row, int start_col, int draw) {

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

void ui_medidor_steup(int fil, int col, int longit, int pintar){
	ui_datos_medidor.base_f = fil+2;
	ui_datos_medidor.base_c = col+1;
	ui_datos_medidor.longitud = longit;
	ui_datos_medidor.pintados = 0;

	int borde_izq = ui_datos_medidor.base_c - 1;
	int borde_der = ui_datos_medidor.base_c + ui_datos_medidor.longitud + 2;
	int borde_sup = ui_datos_medidor.base_f - 8;
	int borde_inf = ui_datos_medidor.base_f + 8;

	ui_writeByteAnywhere(borde_sup,  borde_izq, 0x80);
	ui_writeByteAnywhere(borde_sup,  borde_der, 0x80);
	for (int i = borde_izq + 1; i < borde_der; ++i) {
		ui_writeByteAnywhere(borde_sup,  i, 0x40);
	}
	ui_writeByteAnywhere(ui_datos_medidor.base_f, borde_izq, 0xFF);
	ui_writeByteAnywhere(ui_datos_medidor.base_f, borde_der, 0xFF);

	ui_writeByteAnywhere(borde_inf,  borde_izq, 0x01);
	ui_writeByteAnywhere(borde_inf,  borde_der, 0x01);
	for (int i = borde_izq + 1; i < borde_der; ++i) {
		ui_writeByteAnywhere(borde_inf,  i, 0x02);
	}


}

void ui_medidor_clear() {
	int fil = ui_datos_medidor.base_f;
	int col = ui_datos_medidor.base_c  + 1;
	for (int i = 0; i < ui_datos_medidor.longitud; ++i) {
		ui_writeByteAnywhere(fil, col + i, 0x00);
	}
}
void ui_medidor_draw(const int valor, const int max_valor, const int min_valor){
	if ((valor > max_valor) || (valor < min_valor)) return;
	int range = max_valor - min_valor;
	int new_height = (ui_datos_medidor.longitud * valor) / range;
	
	int diff = new_height - ui_datos_medidor.pintados;

	int num_its = abs(diff);
	int col = ui_datos_medidor.base_c + ui_datos_medidor.pintados;

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
		ui_writeByteAnywhere(ui_datos_medidor.base_f, col, draw);
		col += change;
	}

	ui_datos_medidor.pintados = new_height;
}







