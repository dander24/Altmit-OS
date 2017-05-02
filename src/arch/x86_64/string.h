#ifndef STRING
#define STRING

extern void *kmemset(void *dst, int c, size_t n);
extern void *kmemcpy(void *dest, const void *src, size_t n);
extern size_t kstrlen(const char *s);
extern char *kstrcpy(char *dest, const char *src);
extern int kstrcmp(const char *s1, const char *s2);
extern const char *kstrchr(const char *s, int c);

#endif
