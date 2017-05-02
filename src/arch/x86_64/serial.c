#include <stdint-gcc.h>
#include "serial.h"
#include "interrupt.h"
#include "bio.h"
#include "VGA.h"
#include "string.h"

#define BUSY 1
#define IDLE 0

#define COM1ITR 0x24
#define IRQLINE 4

//SER DEF
#define COM1 0x3F8
#define C1_DATA (COM1)
#define C1_IER (COM1+1)
#define C1_LSBDIV (COM1)
#define C1_MSBDIV (COM1+1)
#define C1_INTIDENT (COM1+2)
#define C1_LINEC (COM1+3)
#define C1_MODEMC (COM1+4)
#define C1_LINES (COM1+5)
#define C1_MODEMS (COM1+6)

SERDATA ser_state[1];

void SER_init()
{
	LOCK;
	//setup state
	kmemset(ser_state->buffer,0, buffsize);
	ser_state->state = IDLE;
	ser_state->r_buffer = 0;
	ser_state->w_buffer = 0;
	//configure device
	outb(C1_IER, 0x00); //disable interrupts
	outb(C1_LINEC, 0x80); //enable DLAB
	outb(C1_LSBDIV, 0x01); //set divisor 1 (max baud)
	outb(C1_MSBDIV, 0x00); //set divisor (cont)
	outb(C1_INTIDENT, 0xC7); //FIFO, 14 byte
	outb(C1_LINEC, 0x03); //8N1
	outb(C1_MODEMC, 0x0B); //irq enabled
	outb(C1_IER, 0x0F);
	//allow intr
	irq_set_handler(COM1ITR, SER_IRQ, (void*) ser_state);
	IRQ_clear_mask(4);
	UNLOCK;
}


//INTERRUPTS SHOULD BE DISABLED BEFORE CALLING
static void SER_hw_write()
{
	uint8_t temp;
	//double check to make sure state is correct
	if(ser_state->state == BUSY)
	{
		temp = inb(C1_LINES);
		//printk("TEMP %x\n", temp);
		if(temp & 0x20)
			ser_state->state = IDLE;
	}

	//make sure we can write	
	if(ser_state->state == IDLE)
	{
		if(ser_state->r_buffer == ser_state->w_buffer)
			return; //nothing to write, get outta here
	
		ser_state->state = BUSY;
		outb(C1_DATA, ser_state->buffer[ser_state->r_buffer++]);
		ser_state->r_buffer %= buffsize; //loop around if you hit max size
	}
}

//INTR handler, comes locked
void SER_IRQ(int irq, int error, void* arg)
{
	uint8_t status = inb(C1_INTIDENT);
	status &= 0x0F;
	if(status == 0x2) // Transmit holding empty
	{
		((SERDATA*) arg)->state = IDLE;
		SER_hw_write();
		
	}
	else if(status == 0x6) // Line Status Change
	{
		status = inb(C1_LINES);
		//printk("STATUS %x\n", status);
	}
}

int SER_write(const char *buff, int len)
{
	LOCK;
	int ret = 0;
	for(int i = 0; i < len; i++)
	{
		if(ser_state->w_buffer == ser_state->r_buffer-1 ||
		(ser_state->w_buffer == buffmax && ser_state->r_buffer == 0))
			return ret;
		ser_state->buffer[ser_state->w_buffer++] = buff[i];
		ser_state->w_buffer %= buffsize; //loop around if you hit max size	
	}
	SER_hw_write();
	UNLOCK;
	return 0;
}
