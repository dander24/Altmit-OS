#include "VGA.h"
#include "keyboard.h"
#include "interrupt.h"
#include "bio.h"
#include "serial.h"
#include "mmu.h"
#include "string.h"

void test_mmu()
{
	int *temp1, *temp2, *temp3, *temp4;
	temp1 = (int*) alloc_pf();
	printk("T1: %p\n", temp1);
	temp2 = (int*) alloc_pf();
	printk("T2: %p\n", temp2);
	temp3 = (int*) alloc_pf();
	printk("T3: %p\n", temp3);
	//set
	for(int i = 0; i < 512; i++)
	{
		temp1[i] = 1;
		temp2[i] = 2;
		temp3[i] = 3;
	}
	//check
	for(int j = 0; j < 512; j++)
	{
		if(temp1[j] != 1)
		{
			printk("TEMP1 ERR");
			asm("hlt");
		}
		if(temp2[j] != 2)
		{
			printk("TEMP2 ERR");
			asm("hlt");
		}
		if(temp3[j] != 3)
		{
			printk("TEMP3 ERR");
			asm("hlt");
		}
	}	
	int i = 1;
	free_pf(temp2);
	//while(i);
	free_pf(temp3);
	
	temp4 = (int*) alloc_pf();
	printk("T4: %p\n", temp4);
	temp3 = (int*) alloc_pf();
	printk("T3: %p\n", temp3);
	for(int k = 0; k < 512; k++)
	{
		temp3[k] = 4;
		temp4[k] = 5;
	}
	//check
	for(int f = 0; f < 512; f++)
	{
		if(temp3[f] != 4)
		{
			printk("TEMP3 ERR");
			asm("hlt");
		}
		if(temp4[f] != 5)
		{
			printk("TEMP4 ERR");
			asm("hlt");
		}
	}
}

void kernel_main(void* ELF)
{
	uint64_t i = 1;
	void* temp4;
	VGA_clear();
	initPS2();
	IRQ_init();
	//while(i);
	SER_init();
	//while(i)	
	init_mmu(ELF);
	while(1)
	{
		
	}	
	
	return; //this should never happen
}
