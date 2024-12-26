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

#define MAX_BUTTONS 4

typedef int button_id;
enum states_animation {
	UNPRESSED,
	CHANGING,
	PRESS,
};

int num_buttons = 0;
button_frame_t idbutton_frames[MAX_BUTTONS];
int idbutton_row[MAX_BUTTONS];
int idbutton_col[MAX_BUTTONS];

typedef struct{
	int base_f;
    int base_c;
    int longitud;
    int pintados;
} datos_medidor_t;

datos_medidor_t datos_medidor;

void writeSpriteAnywhere(sprite_t sprite, int start_row, int start_col);

void writeByteAnywhere(int start_row, int start_col, int draw);

button_id setup_button(button_frame_t button_frames, int fil, int col);

void draw_button(button_id id);

bool CheckUpdateButtonAnim(button_id id, bool pressed, int anim_state);

void clear_medidor();

void setup_medidor(int fil, int col, int longit, int pintar);

void invertDot(int row, int col);

void new_update_medidor(const int valor, const int max_valor, const int min_valor);

#endif 


