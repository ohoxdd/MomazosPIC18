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

#define MAX_BUTTONS 10

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

int medidor_base_f;
int medidor_base_c;



void update_medidor(int current_psi, int change_psi);

void writeCharMatrixRow(sprite_t sprite, int start_row, int start_col);

writeByteAnywhere(int start_row, int start_col, int draw);

button_id setup_button(button_frame_t button_frames, int fil, int col);

void draw_button(button_id id);

bool CheckUpdateButtonAnim(button_id id, bool pressed, int anim_state);

void setup_medidor(int fil, int col);

void invertDot(int row, int col);

void new_update_medidor(int current_psi, int change_psi);

#endif 


