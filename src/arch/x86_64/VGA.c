#include "VGA.h"
#include "string.h"
#include "bio.h"
#include "serial.h"
#include <stdarg.h>

#define DEC 10
#define HEX 16

#define MAXBUFFSIZE 32 //it's impossible to have a number longer than something like 22 chars, but use 32 for safety

static void scroll(void);
static unsigned int getline(int cursor);
//static void clearline(int line);

static char* itoa(long long int num, int base, char* str);
static char* uitoa(unsigned long long int num, int base, char* str);


static vga_char *vgaBuffer= (vga_char*)VGA_BASE;
static int cursor;
static int cursor_max = (VGA_WIDTH * VGA_HEIGHT)-1;
static char rstr[MAXBUFFSIZE];
char* serout;
void VGA_clear(void)
{
	LOCK;
	kmemset((void*) vgaBuffer, 0, cursor_max * sizeof(vga_char));
	cursor = 0;
	UNLOCK;
}

void VGA_display_char(char c)
{
	LOCK;
	*serout = c;
	SER_write(serout,1);
	if(c == '\n')//newline
	{
		cursor = ((getline(cursor) + 1) * VGA_WIDTH);
		if(cursor >= cursor_max)
			scroll();
	}
	else if(c == '\r')//carrage return
	{
		cursor = getline(cursor) * VGA_WIDTH;
	}
	else
	{
		vga_char chr; //make new character to place on screen
		chr.blink = 0;
		chr.bgc = BLACK;
		chr.fgc = WHITE;
		chr.chr = c;
		vgaBuffer[cursor] = chr; //place char
		if(cursor < cursor_max) //see if we're at the end of the screen
			cursor++;
		else
			scroll();
	}
	UNLOCK;
}

//intr safe
void VGA_display_str(const char *s)
{
	char c = *s;
	int i = 1;
	while(c != '\0')
	{
		VGA_display_char(c);
		c = *(s+i);
		i++;
	}
}

//intr safe
int printk(const char* fmt, ...)
{
	LOCK;
	char c;
	char str[MAXBUFFSIZE];
	int i = 1; //track location in  format

	va_list args;
	va_start(args, fmt);

	c = *fmt;
	
	while(c != '\0')
	{
		if(c != '%')
		{
			VGA_display_char(c);
			c = *(fmt + i);
			i++;
		}
		else //format specifier
		{
			c = *(fmt + i);
			i++;
			switch(c)
			{
				case '%': //another %
				VGA_display_char(c);
				break;

                                case 'd': //signed int
				VGA_display_str(itoa(va_arg(args,int),DEC,str));
                                break;

                                case 'u': //unsigned int
				VGA_display_str(uitoa(va_arg(args,unsigned int),DEC,str));
                                break;

                                case 'x': //unsigned hex
				VGA_display_str(uitoa(va_arg(args,unsigned int),HEX,str));
                                break;

                                case 'c': //char
				VGA_display_char((char)va_arg(args, int));
                                break;

                                case 'p': //unsigned pointer (long long) in hex
				VGA_display_str(uitoa(va_arg(args,unsigned long long int),HEX,str));
                                break;

                                case 's': //string
				VGA_display_str(va_arg(args, char*));
                                break;

				case 'h': //short (half)
				c = *(fmt  + i);
				i++;
				switch(c)
				{
					case 'd': //signed
					VGA_display_str(itoa((short)va_arg(args,int),DEC,str));
					break;
					case 'u': //unsigned
					VGA_display_str(uitoa((unsigned short)va_arg(args,unsigned int),DEC,str));
					break; //hex
					case 'x':
					VGA_display_str(uitoa((unsigned short)va_arg(args,unsigned int),HEX,str));
					break;
				}
				break;

				case 'l': //long
				c = *(fmt  + i);
				i++;
				switch(c)
				{
					case 'd': //signed
					VGA_display_str(itoa((long)va_arg(args,long int),DEC,str));
					break;
					case 'u': //unsigned
					VGA_display_str(uitoa((unsigned long)va_arg(args,unsigned long int),DEC,str));
					break;
					case 'x': //hex
					VGA_display_str(uitoa((unsigned long)va_arg(args,unsigned long int),HEX,str));
					break;
				}
				break;
 
				case 'q': //QUAD
				c = *(fmt  + i);
				i++;
				switch(c)
				{
					case 'd': //signed
					VGA_display_str(itoa(va_arg(args,long long int),DEC,str));
					break;
					case 'u': //unsigned
					VGA_display_str(uitoa(va_arg(args,unsigned long long int),DEC,str));
					break;
					case 'x': //hex
					VGA_display_str(uitoa(va_arg(args,unsigned long long int),HEX,str));
					break;
				}
				break;
			}
			c = *(fmt  + i);
			i++;
		}
	}
	UNLOCK;
	return 0; //for now?
}

static void scroll(void)
{
	LOCK;
	void* start = (void*) vgaBuffer + (VGA_WIDTH * sizeof(vga_char)); 
	size_t len = (VGA_WIDTH * (VGA_HEIGHT - 1)) * sizeof(vga_char);
	kmemcpy((void*) vgaBuffer, start, len);
	cursor = (VGA_HEIGHT - 1) * VGA_WIDTH;

	//clear last line
	//clearline(VGA_HEIGHT-1);
	kmemset((void*) (&(vgaBuffer[cursor])), 0, VGA_WIDTH * sizeof(vga_char));
	UNLOCK; 
}

static unsigned int getline(int cursor)
{
	return (cursor / VGA_WIDTH);
}

/*static void clearline(int line)
{
	void* loc = vgaBuffer + ((VGA_WIDTH * line) * sizeof(vga_char));
	size_t len = VGA_WIDTH * sizeof(vga_char);
	kmemset(loc, 0, len);
}*/

//SIGNED VERSION
static char* itoa(long long int num, int base, char* str) //long long for max compat
{
	LOCK;
	int negative = 0;
	int i = 0, rm;
	char next; //return buffer, should always be long enough
	unsigned long long int unum = 0;	
	
	
	if(num == 0) //saves special cases later
		return "0";			

	if(num < 0) //we'll take care of negatives later
	{
		negative = 1;
		unum = ~num; //undo two's comp
		unum++;
	}
	else
		unum = num;
	
	while(unum != 0)
	{
		rm = unum % base;
		if(rm <= 9)
			next = '0' + rm;
		else //support bases up to 16 (hex)
			next = 'a' + (rm-10);
		rstr[i++] = next;		
		unum = unum / base;
	}
	
	if(negative)
		rstr[i++] = '-'; 
	//reverse str
	for(int j = 0; j < i; j++)
	{
		str[j] = rstr[i-1-j];
	}
	str[i] = '\0';
	UNLOCK;
	return str;
}

//UNSIGNED
static char* uitoa(unsigned long long int num, int base, char* str) //long long for max compat
{
	LOCK;
	int i = 0, rm;
	char next; //return buffer, should always be long enough
		
	
	if(num == 0) //saves special cases later
		return "0";			
	
	while(num != 0)
	{
		rm = num % base;
		if(rm <= 9)
			next = '0' + rm;
		else //support bases up to 16 (hex)
			next = 'a' + (rm-10);
		rstr[i++] = next;		
		num = num / base;
	}
	
	//reverse str
	for(int j = 0; j < i; j++)
	{
		str[j] = rstr[i-1-j];
	}
	str[i] = '\0';
	UNLOCK;
	return str;
}

