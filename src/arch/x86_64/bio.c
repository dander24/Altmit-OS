#include <stdint-gcc.h>
#include "VGA.h"

//definitely "borrowed" from the osdev wiki (credit where it's due)

void outb(uint16_t port, uint8_t val)
{
	asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

uint8_t inb(uint16_t port)
{
	uint8_t ret;
	asm volatile("inb %1, %0" : "=a"(ret): "Nd"(port));
	return ret;
}

int are_intr_enabled(void)
{
	uint64_t ret;
	asm("PUSHFQ");
	asm("pop %0" : "=a"(ret) :);
	return (ret & 0x200)? 1 : 0; //1 if intr enabled (bit 9 en) else 0
}
