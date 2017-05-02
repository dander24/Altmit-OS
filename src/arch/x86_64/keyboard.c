#include "VGA.h"
#include "bio.h"

#define PS2_DATA 0x60
#define PS2_CMD 0x64
#define PS2_STATUS 0x64
#define PS2_STAT_OUT 1
#define PS2_STAT_IN (1<<1)

char LOOKUP[0xFF];

static char pollReadPS2(void)
{
	char stat = inb(PS2_STATUS);
	while(!(stat & PS2_STAT_OUT))
		stat = inb(PS2_STATUS);
	return inb(PS2_DATA);
}

//when this function is done PS2 is ready to be written to
static void writeReadyPS2(void)
{
	char stat = inb(PS2_STATUS);
	while(stat & PS2_STAT_IN)
		stat = inb(PS2_STATUS);	
	return;

}

static void lookupCONF()
{
	for(int i = 0; i < 0xFF; i++)
		LOOKUP[i] = '\0';

	LOOKUP[0x1c] = 'a';
	LOOKUP[0x32] = 'b';
	LOOKUP[0x21] = 'c';
	LOOKUP[0x23] = 'd';
	LOOKUP[0x24] = 'e';
	LOOKUP[0x2b] = 'f';
	LOOKUP[0x34] = 'g';
	LOOKUP[0x33] = 'h';
	LOOKUP[0x43] = 'i';
	LOOKUP[0x3b] = 'j';
	LOOKUP[0x42] = 'k';
	LOOKUP[0x4b] = 'l';
	LOOKUP[0x3a] = 'm';
	LOOKUP[0x31] = 'n';
	LOOKUP[0x44] = 'o';
	LOOKUP[0x4d] = 'p';
	LOOKUP[0x15] = 'q';
	LOOKUP[0x2d] = 'r';
	LOOKUP[0x1b] = 's';
	LOOKUP[0x2c] = 't';
	LOOKUP[0x3c] = 'u';
	LOOKUP[0x2a] = 'v';
	LOOKUP[0x1d] = 'w';
	LOOKUP[0x22] = 'x';
	LOOKUP[0x35] = 'y';
	LOOKUP[0x1a] = 'z';
	LOOKUP[0x45] = '0';
	LOOKUP[0x16] = '1';
	LOOKUP[0x1e] = '2';
	LOOKUP[0x26] = '3';
	LOOKUP[0x25] = '4';
	LOOKUP[0x2e] = '5';
	LOOKUP[0x36] = '6';
	LOOKUP[0x3d] = '7';
	LOOKUP[0x3e] = '8';
	LOOKUP[0x46] = '9';
	LOOKUP[0x0e] = '`';
	LOOKUP[0x4e] = '-';
	LOOKUP[0x55] = '=';
	LOOKUP[0x5c] = '\\';
	LOOKUP[0x29] = ' ';
	LOOKUP[0x5a] = '\n';
	LOOKUP[0xF0] = 0xFF;

}

void initPS2()
{
	uint8_t byte;	
	//check if we can write
	writeReadyPS2();

	//disable port 1
	outb(PS2_CMD, 0xAD);

	//check if we can write
	writeReadyPS2();

	//disable port 2
	outb(PS2_CMD, 0xA7);

	//read byte 0 from RAM
	writeReadyPS2();
	outb(PS2_CMD, 0x20);
	byte = pollReadPS2();

	//update config byte
	//set port 1 intr, sys flag, port 1 clock, zero others
	byte = 0x25;
	//check if we can write
	writeReadyPS2();
	outb(PS2_CMD,0x60);
	byte = pollReadPS2();
	writeReadyPS2();
	outb(PS2_DATA,byte);

	//verify status
	outb(PS2_CMD, 0x20);
	byte = pollReadPS2();

	//test controller
	writeReadyPS2();
	outb(PS2_CMD,0xAA);
	byte = pollReadPS2();

	//test KB port
	writeReadyPS2();
	outb(PS2_CMD,0xAB);

	//tell the mouse to get f
	/*writeReadyPS2();
	outb(PS2_CMD, 0xD4);
	byte = pollReadPS2();
	writeReadyPS2();
	outb(PS2_DATA, 0xF0);
	byte = pollReadPS2();*/

	//reset KB
	writeReadyPS2();
	outb(PS2_DATA,0xFF);
	byte = pollReadPS2();

	//set to scan code 3 (the table's gonna suck anyways)
	writeReadyPS2();
	outb(PS2_DATA,0xF0);
	byte = pollReadPS2();

	writeReadyPS2();
	outb(PS2_DATA,0x03);
	byte = pollReadPS2();

	//turn scan mode on
	writeReadyPS2();
	outb(PS2_DATA,0xF4);
	byte = pollReadPS2();

	//enable KB
	writeReadyPS2();
	outb(PS2_CMD,0xAE);
	byte = pollReadPS2();

	//try status one more time
	//read byte 0 from RAM
	writeReadyPS2();
	outb(PS2_CMD, 0x20);
	byte = pollReadPS2();
	//printk("GET STAT %x \n",byte);

	//update config byte
	//set port 1 intr, sys flag, port 1 clock, zero others
	byte = 0x25;
	//check if we can write
	writeReadyPS2();
	outb(PS2_CMD,0x60);
	writeReadyPS2();
	outb(PS2_DATA,0x25);
	byte = pollReadPS2();
	//byte = pollReadPS2();
	writeReadyPS2();
	outb(PS2_CMD, 0x20);
	byte = pollReadPS2();
	//printk("FINAL STATUS %x \n",byte);

	lookupCONF();
	//printk("LOOKUP SET\n");
		
}

char kbGetChar()
{
	char c = '\0';
	uint8_t next;
	while(c == '\0')
	{
		next = pollReadPS2();
		c = LOOKUP[next];
		if(next == 0xF0)
		{
			next = pollReadPS2();
			c = '\0';
		}
	}	
	return c;

}
