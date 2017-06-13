#include "VGA.h"
#include "keyboard.h"
#include "interrupt.h"
#include "bio.h"
#include "serial.h"
#include "mmu.h"
#include "string.h"
#include "mmu.h"
#include "kmalloc.h"

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

void test_mmu2()
{
	long* zastack = alloc_stack();
	printk("%p \n", zastack);
	zastack = zastack - 4096;
	*zastack = 40;
	printk("%d \n", *zastack);
	alloc_heap(511);
	long* temp2 = alloc_heap(2);
	printk("temp2 %p\n", temp2);
	*temp2 = 30;
	temp2[513] = 50;
	printk("temp2 %d %d\n", *temp2, temp2[513]);
	free_heap(temp2, 2);
	temp2 = alloc_heap(2);
	printk("temp2 %p\n", temp2);
	*temp2 = 50;
	temp2[513] = 30;
	printk("temp2 %d %d\n", *temp2, temp2[513]);
	free_heap(temp2, 2);
}

static unsigned int seed = 1;
static void srand (int newseed) {
    seed = (unsigned)newseed & 0x7fffffffffffffffU;
}

int rand (void) {
    seed = (seed * 6364136223846793005U + 1442695040888963407U)
                 & 0x7fffffffffffffffU;
    return (int)seed;
}

void test_kmal()
{
	unsigned int i = 1, z = 1, ran, size;
	uint64_t* data;
	void* temp = 0;
	data = (uint64_t*) kmalloc(sizeof(uint64_t) * 3000);	
	for(int q = 0; q < 3000; q++)
		data[q] = 0;
	
	while(i)
	{
		ran = rand();
		ran %= 3000;
		printk("rand %u\n", ran);
		temp = data[ran];
		size = rand();
		size %= 10000;
		printk("size %d\n",size); 
		//while(z);
		if(temp == 0)
		{
			data[ran] = kmalloc(size);
		}
		else
		{
			kfree(data[ran]);
			data[ran] = 0;
		}
	}
}

void test_kmal2()
{
	void* temp = kmalloc(50);
	unsigned int ran;
	kfree(temp);
	ran = rand();
	ran %= 3000;
	printk("rand %u\n", ran);
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
	init_kmalloc();
	test_kmal();
	while(1)
	{
		
	}	
	
	return; //this should never happen
}
