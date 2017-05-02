#ifndef VGADISP
#define VGADISP
#include "constants.h"
#include <stdint-gcc.h>

//COLORS
#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define MAGENTA 5
#define BROWN 6
#define LGRAY 7
//fgc only
#define DGRAY 8
#define LBLUE 9
#define LGREEN 10
#define LCYAN 11
#define LRED 12
#define LMAGENTA 13
#define YELLOW 14
#define WHITE 15


#define VGA_BASE 0xb8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

typedef struct vgachar{
	uint8_t chr;
	uint8_t fgc:4;
	uint8_t bgc:3;
	uint8_t blink:1;
/*
	uint8_t blink:1;
	uint8_t bgc:3;
	uint8_t fgc:4;
	uint8_t chr;*/ 
} __attribute__((packed)) vga_char;

extern void VGA_clear(void);
extern void VGA_display_char(char);
extern void VGA_display_str(const char *);

extern int printk(const char *,...) __attribute__ ((format(printf,1,2)));
#endif
