#include "constants.h"
#include "string.h"

void *kmemset(void *dst, int c, size_t n)
{
	for(int i = 0; i < n; i++)
	{
		*((char*) dst+i) = (char)c;
	}
	return dst;
}
 
void *kmemcpy(void *dest, const void *src, size_t n)
{
	for(int i = 0; i < n; i++)
	{
		*((unsigned char*)dest+i) = *((unsigned char*)src+i);
	}
	return dest;
}

size_t kstrlen(const char *s)
{
	size_t l = 0;
	char c;

	c = *s;
	while(c != '\0')
	{
		l++;
		c = *(s+l);
	}
	return l;

}

char* kstrcpy(char *dest, const char *src)
{
	int cur = 0;
	char c;
	
	c = *src;
	while(c != '\0')
	{
		*(dest + cur) = c;
		c = *(src + ++cur);
	}
	return dest;
}

int kstrcmp(const char *s1, const char *s2)
{
	int cmp = 0, cur = 0;
	char c1, c2;
	do{
	c1 = *(s1 + cur);
	c2 = *(s2 + cur);
	cmp = s1 - s2;
	cur++;
	}while(cmp == 0 || (c1 != '\0' && c2 != '\0'));

	return cmp;

}

const char *kstrchr(const char *s, int c)
{
	int loc = 0;
	unsigned char search = (unsigned char) c, cur;

	do{
	cur = *(s + loc++);
	}while (cur != '\0' || cur != search);	
	
	if(cur == search)
		return s + loc;

	return NULL;
}
