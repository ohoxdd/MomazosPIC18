#ifndef UI_H
#define UI_H 

#include <xc.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "typedefs.h"
#include "GLCD.h"
#include "sprites.h"

#define MIN_PRESSURE 0
#define MAX_PRESSURE 90
#define ANCHURA_MEDIDOR 10
#define LONGITUD_MEDIDOR 50


typedef struct{
	int base_f;
    int base_c;
    int longitud;
    int pintados;
} datos_medidor_t;

typedef struct {
	char* icon;
	int fil;
	int col;
	bool pressed;
	bool change;
} button_t;

button_t ui_button_settup(int fil, int col, char* icon);

datos_medidor_t ui_datos_medidor;

void ui_button_draw(bool active, button_t button);

void ui_drawSprite(sprite_t sprite, int start_row, int start_col);

void ui_drawSelectedCols(char* matrix, int f_array, int c_array,  int f_glcd, int c_glcd, int c_ini, int c_fin);

void ui_drawScrolled(sprite_t sprite, int start_row, int start_col, int offset);

void ui_writeByteAnywhere(int start_row, int start_col, int draw);

void ui_medidor_clear();

void ui_medidor_steup(int fil, int col, int longit, int pintar);

void ui_medidor_draw(const int valor, const int max_valor, const int min_valor);



#endif 


