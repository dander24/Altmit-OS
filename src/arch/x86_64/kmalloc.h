#ifndef KMALLOC
#define KMALLOC

uint64_t kmalloc(uint64_t size);
void init_kmalloc();
void kfree(void* ptr);

#endif
