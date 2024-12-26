#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <stdint.h>

typedef enum{
 READY,
 RUNNING,
 STOPPED
}state_t;

enum flanc_t {
	FALLING,
	RISING, 
};

typedef struct{
	unsigned int segs;
	unsigned int dec;
} format_t;

struct DC_values {
	uint8_t MSb; // CCPR3L
	uint8_t LSb; // CCP3CON<5:4>
};

#endif
