#include <stdint-gcc.h>
#include "mmu.h"
#include "VGA.h"
#include "string.h"
#include "bio.h"

#define FOURK 4096

typedef struct FreeList {
   struct FreeList *next;
} free_list;

typedef struct mallocPool {
int max_size;
int avail;
free_list* head;
} k_malloc_pool;

typedef struct mallocInfo {
k_malloc_pool* pool;
uint64_t size;
} malloc_info;

static k_malloc_pool s32_pool;
static k_malloc_pool s64_pool;
static k_malloc_pool s256_pool;
static k_malloc_pool s1024_pool;

void init_kmalloc()
{
	s32_pool.max_size = 32 - sizeof(malloc_info);
	s32_pool.avail = 0;
	s32_pool.head = NULL;
	
	s64_pool.max_size = 64 - sizeof(malloc_info);
	s64_pool.avail = 0;
	s64_pool.head = NULL;

	s256_pool.max_size = 256 - sizeof(malloc_info);
	s256_pool.avail = 0;
	s256_pool.head = NULL;

	s1024_pool.max_size = 1024 - sizeof(malloc_info);
	s1024_pool.avail = 0;
	s1024_pool.head = NULL;
}

void kfree(void* ptr)
{
	k_malloc_pool* cur_pool = NULL;
	malloc_info info;
	ptr = (ptr - sizeof(info));
	info = *( (malloc_info*) ptr);
	cur_pool = info.pool;
	printk("%qu %qu\n", info.size, info.pool);
	if(info.size > s1024_pool.max_size)
	{
		int temp = info.size / FOURK;
		temp++;
		printk("FREEING %d @ %p\n", temp, ptr);
		free_heap(ptr, temp);
		return;
	}
	free_list new_head;
	new_head.next = cur_pool->head;
	*((free_list*)ptr) = new_head;
	cur_pool->head = ptr;
	cur_pool->avail++;
	printk("FREEING %d @ %p\n", info.size, ptr);
	printk("REMAINING %d\n", cur_pool->avail);
	return;
}

uint64_t kmalloc(uint64_t size)
{
	k_malloc_pool* cur_pool = NULL;
	void* ret;
	malloc_info info;
	//select pool
	if(size == 0)
		return NULL;

	if(size <= s32_pool.max_size)
		cur_pool = &s32_pool;
	else if(size <= s64_pool.max_size)
		cur_pool = &s64_pool;
	else if(size <= s256_pool.max_size)
		cur_pool = &s256_pool;
	else if(size <= s1024_pool.max_size)
		cur_pool = &s1024_pool;
	
	if(cur_pool == NULL) //alloc whole pages
	{
		int temp = size / FOURK;
		temp++;

		ret = alloc_heap(temp);
		
		info.pool = NULL;
		info.size = size;
		*((malloc_info*) ret) = info;
		ret = ret + sizeof(info);
		printk("%d PAGES @ RET %p\n", temp, ret);
		return ret;
	}
	
	//there already exists a node to allocate
	if(cur_pool->avail == 0)
	{
		
		int remaining_free = 0; 
		void* temp = alloc_heap(1);
		cur_pool->head = temp;
		free_list next_free;
		int size = cur_pool->max_size + sizeof(info);
		while(remaining_free < FOURK)
		{
			next_free.next = temp + remaining_free + size; 
			*((free_list*) (temp + remaining_free)) = next_free;
			remaining_free += size;
			cur_pool->avail++;
		}
		printk("NEW SET ADDED, %d blocks @ %p\n", cur_pool->avail, cur_pool->head);
	}
	
	info.pool = cur_pool;
	info.size = size;
	void* next = ((free_list) *(cur_pool->head)).next;
	*((malloc_info*) cur_pool->head) = info;
	ret = (void*) cur_pool->head + sizeof(info);
	cur_pool->head = next;
	cur_pool->avail--;
	printk("RET %p\, REMAINING, %d\n", ret, cur_pool->avail);
	return ret;
}


