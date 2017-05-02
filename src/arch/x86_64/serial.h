#ifndef CEREAL
#define CEREAL

#define buffsize 4096
#define buffmax (buffsize-1)
void SER_init(void);
int SER_write(const char *buff, int len);
void SER_IRQ(int irq, int error, void* arg);

typedef struct SERDATA{
	char buffer[buffsize];        
	int state;
	int r_buffer;
	int w_buffer;
} SERDATA;

#endif

