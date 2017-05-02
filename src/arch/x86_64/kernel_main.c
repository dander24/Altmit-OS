#include "VGA.h"
#include "keyboard.h"
#include "interrupt.h"
#include "bio.h"
#include "serial.h"

void kernel_main(void* ELF)
{
	int i = 1;
	VGA_clear();
	initPS2();
	IRQ_init();
	//while(i);
	SER_init();
	//while(i)	
	init_mmu(ELF);
	while(1)
	{
		//printk("%c",  kbGetChar());
	}	
	
	return; //this should never happen
}
