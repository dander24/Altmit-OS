#include <stdint-gcc.h>
#include "VGA.h"
#include "string.h"
#include "keyboard.h"
#include "bio.h"
#include "interrupt.h"

#define IDT_COUNT 256
#define TRAP_GATE 0xF
#define INT_GATE 0xE

//PIC DEFINES
#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)


#define PIC_READ_IRR                0x0a    /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR                0x0b    /* OCW3 irq service next CMD read */

#define PIC_EOI		0x20	

#define ICW1_ICW4	0x01		/* ICW4 (not) needed */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */
 
#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

//NEW STACKS
static char PFSTACK[4096], GPSTACK[4096], DFSTACK[4096]; //4k BYTE STATIC ERROR STACKS

//INTERNALS
static void build_irq_table();
static void configureTSS();

//EXTERNAL
extern uint64_t gdt64[];

static struct {
void * arg;
irq_handler_t handler;
} irq_table[IDT_COUNT];

typedef struct honnotss{
uint32_t RES6;
uint32_t RSP0_31_0;
uint32_t RSP0_63_32;
uint32_t RSP1_31_0;
uint32_t RSP1_63_32;
uint32_t RSP2_31_0;
uint32_t RSP2_63_32;
uint32_t RES5;
uint32_t RES4;
uint32_t IST1_31_0;
uint32_t IST1_63_32;
uint32_t IST2_31_0;
uint32_t IST2_63_32;
uint32_t IST3_31_0;
uint32_t IST3_63_32;
uint32_t IST4_31_0;
uint32_t IST4_63_32;
uint32_t IST5_31_0;
uint32_t IST5_63_32;
uint32_t IST6_31_0;
uint32_t IST6_63_32;
uint32_t IST7_31_0;
uint32_t IST7_63_32;
uint32_t RES3;
uint32_t RES2;
uint32_t RES1:16;
uint32_t IOBASE:16;
}__attribute__ ((packed)) TSSStruct;

typedef struct idtent{
uint32_t OFF15_0:16;
uint32_t TARGET:16;
uint32_t IST:3;
uint32_t RESERV2:5;
uint32_t TYPE:4;
uint32_t ZERO:1;
uint32_t DPL:2;
uint32_t P:1;
uint32_t OFF31_16:16;
uint32_t OFF63_32;
uint32_t RESERV;
}__attribute__ ((packed)) idtentry;

typedef struct TSSdes{
uint32_t SEGL15_0:16;
uint32_t BASE15_0:16;
uint32_t BASE23_16:8;
uint32_t TYPE:4;
uint32_t ZERO2:1;
uint32_t DPL:2;
uint32_t P:1;
uint32_t SEGL19_16:4;
uint32_t AVL:1;
uint32_t RES3:2;
uint32_t G:1;
uint32_t BASE31_24:8;
uint32_t BASE63_32;
uint32_t RES2:8;
uint32_t ZERO:5;
uint32_t RES:19;
}__attribute__ ((packed)) tssdesc;

typedef struct IDTR{
        uint16_t length;
        void*    base;
} __attribute__((packed, aligned(16))) IDTR;

idtentry IDT[IDT_COUNT];
static TSSStruct TSS;
 
/* Helper func */
static uint16_t __pic_get_irq_reg(int ocw3)
{
    /* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
     * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
    outb(PIC1_COMMAND, ocw3);
    outb(PIC2_COMMAND, ocw3);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}
 
/* Returns the combined value of the cascaded PICs irq request register */
uint16_t pic_get_irr(void)
{
    return __pic_get_irq_reg(PIC_READ_IRR);
}
 
/* Returns the combined value of the cascaded PICs in-service register */
uint16_t pic_get_isr(void)
{
    return __pic_get_irq_reg(PIC_READ_ISR);
}


void PIC_sendEOI(unsigned char irq)
{
	if(irq >= 8)
		outb(PIC2_COMMAND,PIC_EOI);
 
	outb(PIC1_COMMAND,PIC_EOI);
}

void IRQ_init(void)
{
	CLI;
	int i = 1;
	//GDT = gdt64[0];
	PIC_remap(0x20, 0x28);
	build_IDT();
	build_irq_table();
	IDTR newIDTR;
	newIDTR.length = sizeof(idtentry) * 256;
	newIDTR.base = &IDT;
	//update GDT
	//offset 0x10 (AKA GDT[1])
	//configure TSS
	configureTSS();
	//configure TSS Descriptor
	tssdesc TSSD;
	TSSD.SEGL15_0 = (uint16_t) (sizeof(TSS));
	TSSD.SEGL19_16 = 0;
	TSSD.BASE15_0 = (uint16_t) (&TSS);
	TSSD.BASE23_16 = (uint16_t) ((long)&TSS >> 16);
	TSSD.BASE31_24 = (uint16_t) ((long)&TSS >> 24);
	TSSD.BASE63_32 = (uint32_t) ((long)&TSS >> 32);
	TSSD.P = 1;
	TSSD.G = 0;
	TSSD.ZERO = 0;
	TSSD.TYPE = 9; 
	//load TSS descr
	kmemcpy(&gdt64[2],&TSSD,sizeof(TSSD));
	//while(i);
	uint16_t sel = 0x10;
	asm("ltr %0" : : "m"(sel));
	asm("lidt %0" : : "m"(newIDTR));
	IRQ_mask_all();
	//IRQ_clear_mask(1);
	STI;
}

static void configureTSS()
{
	uint64_t pfaddr = (uint64_t)&PFSTACK[4096];
	uint64_t gpaddr = (uint64_t)&GPSTACK[4096];
	uint64_t dfaddr = (uint64_t)&DFSTACK[4096];
	TSS.IST1_31_0 = (uint32_t) (pfaddr);
	TSS.IST1_63_32 = (uint32_t) (pfaddr >> 32);
	TSS.IST2_31_0 = (uint32_t) (gpaddr);
	TSS.IST2_63_32 = (uint32_t) (gpaddr >> 32);
	TSS.IST3_31_0 = (uint32_t) (dfaddr);
	TSS.IST3_63_32 = (uint32_t) (dfaddr >> 32);
}

void PIC_remap(unsigned int offset1, unsigned int offset2)
{

	unsigned char a1, a2;
 
	a1 = inb(PIC1_DATA);                        // save masks
	a2 = inb(PIC2_DATA); 
	
	outb(PIC1_COMMAND, ICW1_INIT+ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	outb(PIC2_COMMAND, ICW1_INIT+ICW1_ICW4);
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	outb(PIC1_DATA, ICW4_8086);
	outb(PIC2_DATA, ICW4_8086);
	outb(PIC1_DATA, a1);   // restore saved masks.
	outb(PIC2_DATA, a2);

}

void IRQ_mask_all()
{
	outb(PIC1_DATA, 0xFF);  
	outb(PIC2_DATA, 0xFF);
}

void IRQ_set_mask(unsigned char IRQline) {
    uint16_t port;
    uint8_t value;
 
    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) | (1 << IRQline);
    outb(port, value);        
}
 
void IRQ_clear_mask(unsigned char IRQline) {
    uint16_t port;
    uint8_t value;
 
    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) & ~(1 << IRQline);
    outb(port, value);        
}

void irq_c_handler(int irq, int error)
{
	void (*func) (int, int, void*);
	void* args = irq_table[irq].arg;
	func = irq_table[irq].handler;
	func(irq,error,args);
	if(irq > 0x1f && irq < 0x30)
		PIC_sendEOI(irq);
	//else
		//asm("hlt");
}

void irq_print_error(int irq, int error, void* args)
{
	int i;
	unsigned long val;
	printk("INTR %x, %p\n", irq, &i);
	if(irq == 14)
	{
		asm volatile ("mov %%cr2, %0" : "=r"(val));
		printk("CR2 %p ", val);
		asm volatile ("mov %%cr3, %0" : "=r"(val));
		printk("CR3 %p \n", val);
		asm("hlt");
	}
}

void irq_set_handler(int irq, irq_handler_t handler, void* arg)
{
	irq_table[irq].handler = handler;
	irq_table[irq].arg = arg;
}

static void build_irq_table()
{
	//generic error
	for(int i = 0; i < IDT_COUNT; i++)
	{
		irq_set_handler(i,&irq_print_error,NULL);
	}
	//manual settings
}


void build_IDT()
{
	//COMMON
	idtentry temp;
	temp.P = 1;
	temp.TYPE = INT_GATE;
	temp.DPL = 0;
	temp.TARGET = 0x08;
	temp.IST = 0; //same stack
	uint64_t irqaddr;
	//GENERATED CODE
	irqaddr = (uint64_t) &irq0_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[0] = temp;

	irqaddr = (uint64_t) &irq1_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[1] = temp;

	irqaddr = (uint64_t) &irq2_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[2] = temp;

	irqaddr = (uint64_t) &irq3_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[3] = temp;

	irqaddr = (uint64_t) &irq4_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[4] = temp;

	irqaddr = (uint64_t) &irq5_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[5] = temp;

	irqaddr = (uint64_t) &irq6_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[6] = temp;

	irqaddr = (uint64_t) &irq7_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[7] = temp;

	//DOUBLE FAULT	
	irqaddr = (uint64_t) &irq8_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	temp.IST = 3; //DF Stack
	IDT[8] = temp;
	temp.IST = 0; //same stack

	irqaddr = (uint64_t) &irq9_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[9] = temp;

	irqaddr = (uint64_t) &irq10_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[10] = temp;

	irqaddr = (uint64_t) &irq11_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[11] = temp;

	irqaddr = (uint64_t) &irq12_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[12] = temp;

	//GENERAL PROTECTION FAULT
	irqaddr = (uint64_t) &irq13_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	temp.IST = 2; //GPF STACK
	IDT[13] = temp;
	
	//PAGE FAULT
	irqaddr = (uint64_t) &irq14_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	temp.IST = 1; //PF Stack
	IDT[14] = temp;

	//rest normal
	temp.IST = 0; //same stack
	irqaddr = (uint64_t) &irq15_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[15] = temp;

	irqaddr = (uint64_t) &irq16_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[16] = temp;

	irqaddr = (uint64_t) &irq17_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[17] = temp;

	irqaddr = (uint64_t) &irq18_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[18] = temp;

	irqaddr = (uint64_t) &irq19_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[19] = temp;

	irqaddr = (uint64_t) &irq20_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[20] = temp;

	irqaddr = (uint64_t) &irq21_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[21] = temp;

	irqaddr = (uint64_t) &irq22_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[22] = temp;

	irqaddr = (uint64_t) &irq23_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[23] = temp;

	irqaddr = (uint64_t) &irq24_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[24] = temp;

	irqaddr = (uint64_t) &irq25_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[25] = temp;

	irqaddr = (uint64_t) &irq26_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[26] = temp;

	irqaddr = (uint64_t) &irq27_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[27] = temp;

	irqaddr = (uint64_t) &irq28_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[28] = temp;

	irqaddr = (uint64_t) &irq29_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[29] = temp;

	irqaddr = (uint64_t) &irq30_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[30] = temp;

	irqaddr = (uint64_t) &irq31_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[31] = temp;

	irqaddr = (uint64_t) &irq32_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[32] = temp;

	irqaddr = (uint64_t) &irq33_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[33] = temp;

	irqaddr = (uint64_t) &irq34_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[34] = temp;

	irqaddr = (uint64_t) &irq35_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[35] = temp;

	irqaddr = (uint64_t) &irq36_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[36] = temp;

	irqaddr = (uint64_t) &irq37_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[37] = temp;

	irqaddr = (uint64_t) &irq38_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[38] = temp;

	irqaddr = (uint64_t) &irq39_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[39] = temp;

	irqaddr = (uint64_t) &irq40_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[40] = temp;

	irqaddr = (uint64_t) &irq41_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[41] = temp;

	irqaddr = (uint64_t) &irq42_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[42] = temp;

	irqaddr = (uint64_t) &irq43_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[43] = temp;

	irqaddr = (uint64_t) &irq44_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[44] = temp;

	irqaddr = (uint64_t) &irq45_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[45] = temp;

	irqaddr = (uint64_t) &irq46_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[46] = temp;

	irqaddr = (uint64_t) &irq47_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[47] = temp;

	irqaddr = (uint64_t) &irq48_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[48] = temp;

	irqaddr = (uint64_t) &irq49_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[49] = temp;

	irqaddr = (uint64_t) &irq50_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[50] = temp;

	irqaddr = (uint64_t) &irq51_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[51] = temp;

	irqaddr = (uint64_t) &irq52_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[52] = temp;

	irqaddr = (uint64_t) &irq53_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[53] = temp;

	irqaddr = (uint64_t) &irq54_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[54] = temp;

	irqaddr = (uint64_t) &irq55_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[55] = temp;

	irqaddr = (uint64_t) &irq56_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[56] = temp;

	irqaddr = (uint64_t) &irq57_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[57] = temp;

	irqaddr = (uint64_t) &irq58_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[58] = temp;

	irqaddr = (uint64_t) &irq59_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[59] = temp;

	irqaddr = (uint64_t) &irq60_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[60] = temp;

	irqaddr = (uint64_t) &irq61_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[61] = temp;

	irqaddr = (uint64_t) &irq62_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[62] = temp;

	irqaddr = (uint64_t) &irq63_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[63] = temp;

	irqaddr = (uint64_t) &irq64_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[64] = temp;

	irqaddr = (uint64_t) &irq65_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[65] = temp;

	irqaddr = (uint64_t) &irq66_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[66] = temp;

	irqaddr = (uint64_t) &irq67_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[67] = temp;

	irqaddr = (uint64_t) &irq68_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[68] = temp;

	irqaddr = (uint64_t) &irq69_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[69] = temp;

	irqaddr = (uint64_t) &irq70_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[70] = temp;

	irqaddr = (uint64_t) &irq71_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[71] = temp;

	irqaddr = (uint64_t) &irq72_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[72] = temp;

	irqaddr = (uint64_t) &irq73_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[73] = temp;

	irqaddr = (uint64_t) &irq74_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[74] = temp;

	irqaddr = (uint64_t) &irq75_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[75] = temp;

	irqaddr = (uint64_t) &irq76_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[76] = temp;

	irqaddr = (uint64_t) &irq77_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[77] = temp;

	irqaddr = (uint64_t) &irq78_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[78] = temp;

	irqaddr = (uint64_t) &irq79_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[79] = temp;

	irqaddr = (uint64_t) &irq80_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[80] = temp;

	irqaddr = (uint64_t) &irq81_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[81] = temp;

	irqaddr = (uint64_t) &irq82_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[82] = temp;

	irqaddr = (uint64_t) &irq83_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[83] = temp;

	irqaddr = (uint64_t) &irq84_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[84] = temp;

	irqaddr = (uint64_t) &irq85_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[85] = temp;

	irqaddr = (uint64_t) &irq86_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[86] = temp;

	irqaddr = (uint64_t) &irq87_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[87] = temp;

	irqaddr = (uint64_t) &irq88_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[88] = temp;

	irqaddr = (uint64_t) &irq89_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[89] = temp;

	irqaddr = (uint64_t) &irq90_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[90] = temp;

	irqaddr = (uint64_t) &irq91_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[91] = temp;

	irqaddr = (uint64_t) &irq92_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[92] = temp;

	irqaddr = (uint64_t) &irq93_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[93] = temp;

	irqaddr = (uint64_t) &irq94_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[94] = temp;

	irqaddr = (uint64_t) &irq95_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[95] = temp;

	irqaddr = (uint64_t) &irq96_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[96] = temp;

	irqaddr = (uint64_t) &irq97_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[97] = temp;

	irqaddr = (uint64_t) &irq98_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[98] = temp;

	irqaddr = (uint64_t) &irq99_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[99] = temp;

	irqaddr = (uint64_t) &irq100_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[100] = temp;

	irqaddr = (uint64_t) &irq101_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[101] = temp;

	irqaddr = (uint64_t) &irq102_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[102] = temp;

	irqaddr = (uint64_t) &irq103_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[103] = temp;

	irqaddr = (uint64_t) &irq104_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[104] = temp;

	irqaddr = (uint64_t) &irq105_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[105] = temp;

	irqaddr = (uint64_t) &irq106_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[106] = temp;

	irqaddr = (uint64_t) &irq107_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[107] = temp;

	irqaddr = (uint64_t) &irq108_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[108] = temp;

	irqaddr = (uint64_t) &irq109_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[109] = temp;

	irqaddr = (uint64_t) &irq110_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[110] = temp;

	irqaddr = (uint64_t) &irq111_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[111] = temp;

	irqaddr = (uint64_t) &irq112_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[112] = temp;

	irqaddr = (uint64_t) &irq113_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[113] = temp;

	irqaddr = (uint64_t) &irq114_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[114] = temp;

	irqaddr = (uint64_t) &irq115_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[115] = temp;

	irqaddr = (uint64_t) &irq116_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[116] = temp;

	irqaddr = (uint64_t) &irq117_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[117] = temp;

	irqaddr = (uint64_t) &irq118_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[118] = temp;

	irqaddr = (uint64_t) &irq119_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[119] = temp;

	irqaddr = (uint64_t) &irq120_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[120] = temp;

	irqaddr = (uint64_t) &irq121_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[121] = temp;

	irqaddr = (uint64_t) &irq122_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[122] = temp;

	irqaddr = (uint64_t) &irq123_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[123] = temp;

	irqaddr = (uint64_t) &irq124_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[124] = temp;

	irqaddr = (uint64_t) &irq125_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[125] = temp;

	irqaddr = (uint64_t) &irq126_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[126] = temp;

	irqaddr = (uint64_t) &irq127_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[127] = temp;

	irqaddr = (uint64_t) &irq128_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[128] = temp;

	irqaddr = (uint64_t) &irq129_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[129] = temp;

	irqaddr = (uint64_t) &irq130_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[130] = temp;

	irqaddr = (uint64_t) &irq131_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[131] = temp;

	irqaddr = (uint64_t) &irq132_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[132] = temp;

	irqaddr = (uint64_t) &irq133_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[133] = temp;

	irqaddr = (uint64_t) &irq134_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[134] = temp;

	irqaddr = (uint64_t) &irq135_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[135] = temp;

	irqaddr = (uint64_t) &irq136_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[136] = temp;

	irqaddr = (uint64_t) &irq137_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[137] = temp;

	irqaddr = (uint64_t) &irq138_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[138] = temp;

	irqaddr = (uint64_t) &irq139_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[139] = temp;

	irqaddr = (uint64_t) &irq140_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[140] = temp;

	irqaddr = (uint64_t) &irq141_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[141] = temp;

	irqaddr = (uint64_t) &irq142_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[142] = temp;

	irqaddr = (uint64_t) &irq143_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[143] = temp;

	irqaddr = (uint64_t) &irq144_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[144] = temp;

	irqaddr = (uint64_t) &irq145_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[145] = temp;

	irqaddr = (uint64_t) &irq146_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[146] = temp;

	irqaddr = (uint64_t) &irq147_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[147] = temp;

	irqaddr = (uint64_t) &irq148_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[148] = temp;

	irqaddr = (uint64_t) &irq149_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[149] = temp;

	irqaddr = (uint64_t) &irq150_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[150] = temp;

	irqaddr = (uint64_t) &irq151_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[151] = temp;

	irqaddr = (uint64_t) &irq152_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[152] = temp;

	irqaddr = (uint64_t) &irq153_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[153] = temp;

	irqaddr = (uint64_t) &irq154_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[154] = temp;

	irqaddr = (uint64_t) &irq155_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[155] = temp;

	irqaddr = (uint64_t) &irq156_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[156] = temp;

	irqaddr = (uint64_t) &irq157_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[157] = temp;

	irqaddr = (uint64_t) &irq158_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[158] = temp;

	irqaddr = (uint64_t) &irq159_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[159] = temp;

	irqaddr = (uint64_t) &irq160_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[160] = temp;

	irqaddr = (uint64_t) &irq161_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[161] = temp;

	irqaddr = (uint64_t) &irq162_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[162] = temp;

	irqaddr = (uint64_t) &irq163_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[163] = temp;

	irqaddr = (uint64_t) &irq164_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[164] = temp;

	irqaddr = (uint64_t) &irq165_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[165] = temp;

	irqaddr = (uint64_t) &irq166_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[166] = temp;

	irqaddr = (uint64_t) &irq167_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[167] = temp;

	irqaddr = (uint64_t) &irq168_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[168] = temp;

	irqaddr = (uint64_t) &irq169_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[169] = temp;

	irqaddr = (uint64_t) &irq170_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[170] = temp;

	irqaddr = (uint64_t) &irq171_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[171] = temp;

	irqaddr = (uint64_t) &irq172_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[172] = temp;

	irqaddr = (uint64_t) &irq173_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[173] = temp;

	irqaddr = (uint64_t) &irq174_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[174] = temp;

	irqaddr = (uint64_t) &irq175_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[175] = temp;

	irqaddr = (uint64_t) &irq176_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[176] = temp;

	irqaddr = (uint64_t) &irq177_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[177] = temp;

	irqaddr = (uint64_t) &irq178_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[178] = temp;

	irqaddr = (uint64_t) &irq179_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[179] = temp;

	irqaddr = (uint64_t) &irq180_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[180] = temp;

	irqaddr = (uint64_t) &irq181_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[181] = temp;

	irqaddr = (uint64_t) &irq182_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[182] = temp;

	irqaddr = (uint64_t) &irq183_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[183] = temp;

	irqaddr = (uint64_t) &irq184_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[184] = temp;

	irqaddr = (uint64_t) &irq185_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[185] = temp;

	irqaddr = (uint64_t) &irq186_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[186] = temp;

	irqaddr = (uint64_t) &irq187_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[187] = temp;

	irqaddr = (uint64_t) &irq188_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[188] = temp;

	irqaddr = (uint64_t) &irq189_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[189] = temp;

	irqaddr = (uint64_t) &irq190_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[190] = temp;

	irqaddr = (uint64_t) &irq191_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[191] = temp;

	irqaddr = (uint64_t) &irq192_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[192] = temp;

	irqaddr = (uint64_t) &irq193_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[193] = temp;

	irqaddr = (uint64_t) &irq194_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[194] = temp;

	irqaddr = (uint64_t) &irq195_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[195] = temp;

	irqaddr = (uint64_t) &irq196_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[196] = temp;

	irqaddr = (uint64_t) &irq197_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[197] = temp;

	irqaddr = (uint64_t) &irq198_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[198] = temp;

	irqaddr = (uint64_t) &irq199_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[199] = temp;

	irqaddr = (uint64_t) &irq200_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[200] = temp;

	irqaddr = (uint64_t) &irq201_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[201] = temp;

	irqaddr = (uint64_t) &irq202_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[202] = temp;

	irqaddr = (uint64_t) &irq203_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[203] = temp;

	irqaddr = (uint64_t) &irq204_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[204] = temp;

	irqaddr = (uint64_t) &irq205_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[205] = temp;

	irqaddr = (uint64_t) &irq206_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[206] = temp;

	irqaddr = (uint64_t) &irq207_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[207] = temp;

	irqaddr = (uint64_t) &irq208_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[208] = temp;

	irqaddr = (uint64_t) &irq209_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[209] = temp;

	irqaddr = (uint64_t) &irq210_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[210] = temp;

	irqaddr = (uint64_t) &irq211_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[211] = temp;

	irqaddr = (uint64_t) &irq212_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[212] = temp;

	irqaddr = (uint64_t) &irq213_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[213] = temp;

	irqaddr = (uint64_t) &irq214_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[214] = temp;

	irqaddr = (uint64_t) &irq215_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[215] = temp;

	irqaddr = (uint64_t) &irq216_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[216] = temp;

	irqaddr = (uint64_t) &irq217_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[217] = temp;

	irqaddr = (uint64_t) &irq218_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[218] = temp;

	irqaddr = (uint64_t) &irq219_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[219] = temp;

	irqaddr = (uint64_t) &irq220_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[220] = temp;

	irqaddr = (uint64_t) &irq221_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[221] = temp;

	irqaddr = (uint64_t) &irq222_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[222] = temp;

	irqaddr = (uint64_t) &irq223_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[223] = temp;

	irqaddr = (uint64_t) &irq224_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[224] = temp;

	irqaddr = (uint64_t) &irq225_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[225] = temp;

	irqaddr = (uint64_t) &irq226_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[226] = temp;

	irqaddr = (uint64_t) &irq227_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[227] = temp;

	irqaddr = (uint64_t) &irq228_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[228] = temp;

	irqaddr = (uint64_t) &irq229_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[229] = temp;

	irqaddr = (uint64_t) &irq230_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[230] = temp;

	irqaddr = (uint64_t) &irq231_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[231] = temp;

	irqaddr = (uint64_t) &irq232_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[232] = temp;

	irqaddr = (uint64_t) &irq233_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[233] = temp;

	irqaddr = (uint64_t) &irq234_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[234] = temp;

	irqaddr = (uint64_t) &irq235_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[235] = temp;

	irqaddr = (uint64_t) &irq236_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[236] = temp;

	irqaddr = (uint64_t) &irq237_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[237] = temp;

	irqaddr = (uint64_t) &irq238_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[238] = temp;

	irqaddr = (uint64_t) &irq239_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[239] = temp;

	irqaddr = (uint64_t) &irq240_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[240] = temp;

	irqaddr = (uint64_t) &irq241_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[241] = temp;

	irqaddr = (uint64_t) &irq242_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[242] = temp;

	irqaddr = (uint64_t) &irq243_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[243] = temp;

	irqaddr = (uint64_t) &irq244_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[244] = temp;

	irqaddr = (uint64_t) &irq245_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[245] = temp;

	irqaddr = (uint64_t) &irq246_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[246] = temp;

	irqaddr = (uint64_t) &irq247_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[247] = temp;

	irqaddr = (uint64_t) &irq248_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[248] = temp;

	irqaddr = (uint64_t) &irq249_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[249] = temp;

	irqaddr = (uint64_t) &irq250_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[250] = temp;

	irqaddr = (uint64_t) &irq251_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[251] = temp;

	irqaddr = (uint64_t) &irq252_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[252] = temp;

	irqaddr = (uint64_t) &irq253_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[253] = temp;

	irqaddr = (uint64_t) &irq254_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[254] = temp;

	irqaddr = (uint64_t) &irq255_handler;
	temp.OFF63_32 = (uint32_t) (irqaddr >> 32);
	temp.OFF31_16 = (uint16_t) (irqaddr >> 16);
	temp.OFF15_0 = (uint16_t) irqaddr;
	IDT[255] = temp;

	//END GEN


	//UPDATE DIFF STACK ENTRIES

}






