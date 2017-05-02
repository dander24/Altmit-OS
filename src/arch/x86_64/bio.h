#ifndef BIO
#define BIO
#include <stdint-gcc.h>

#define CLI asm("CLI")
#define STI asm("STI")

#define LOCK int enable_ints = 0;if(are_intr_enabled()){enable_ints = 1;CLI;}
#define UNLOCK if(enable_ints)STI;

void outb(uint16_t, uint8_t);
uint8_t inb(uint16_t);
int are_intr_enabled(void);
#endif
