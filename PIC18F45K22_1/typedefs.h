#ifndef TYPEDEFS_H
#define TYPEDEFS_H

typedef enum{
 Ready,
 Running,
 Stopped
}state_t;

enum flanc_t {
	FALLING,
	RISING, 
};

typedef struct{
	unsigned int segs;
	unsigned int dec;
} format_t;
#endif