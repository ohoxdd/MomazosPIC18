#ifndef UI_H
#define UI_H 

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

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

button_t setup_button(int fil, int col, char* icon);

datos_medidor_t datos_medidor;

void write_button(bool active, button_t button);

void writeSpriteAnywhere(sprite_t sprite, int start_row, int start_col);

void writeSelectionAnywhere(char* matrix, int f_array, int c_array,  int f_glcd, int c_glcd, int c_ini, int c_fin);

void writeSpriteOffset(sprite_t sprite, int start_row, int start_col, int offset);

void writeByteAnywhere(int start_row, int start_col, int draw);

void clear_medidor();

void setup_medidor(int fil, int col, int longit, int pintar);

void new_update_medidor(const int valor, const int max_valor, const int min_valor);

#endif 


