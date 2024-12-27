#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum{
 READY,
 RUNNING,
 STOPPED
}state_t;

typedef enum{
	FALLING,
	RISING, 
} flanc_t;

typedef struct{
	unsigned int segs;
	unsigned int dec;
} format_t;

struct DC_values {
	uint8_t MSb; // CCPR3L
	uint8_t LSb; // CCP3CON<5:4>
};

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

#endif
