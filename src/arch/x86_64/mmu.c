#include "VGA.h"
#include "string.h"
#include "mmu.h"
#include "bio.h"
#include <stdint-gcc.h>


//tag types
#define MEMORY_MAP 6
#define ELF_SYMBOLS 9

//mem region type
#define FREE 1

static uint32_t align8(uint32_t num);

typedef struct availmem{
	void* pool_head;
	void* pool_tail;
	void* page_head;
	void* page_tail;
} free_pages;

typedef struct tag_head{
	uint32_t type;
	uint32_t size; //tag size
} __attribute__((packed)) tag_head;

typedef struct tag_bootname{
	uint32_t type;
	uint32_t size; //tag size
	uint32_t type2;
	uint32_t size2;
	uint64_t idk;
	char name;
} __attribute__((packed)) tag_bootname;


typedef struct meminfo{
	uint64_t start_addr;
	uint64_t length;
	uint32_t type;
	uint32_t reserved;
} __attribute__((packed)) mem_info;

typedef struct memmap{
	uint32_t type;
	uint32_t size;
	uint32_t ent_size;
	uint32_t version;
	mem_info mem_info_arr[0];
} __attribute__((packed)) mem_map;

typedef struct elfsection{
	uint32_t name_offset;
	uint32_t section_type;
	uint64_t flags;
	uint64_t addr;
	uint64_t disk_offset;
	uint64_t size;
	uint32_t table_index;
	uint32_t extra;
	uint64_t alignment;
	uint64_t fixed_entries;
} __attribute__((packed)) elf_section;

typedef struct elfheader{
	uint32_t type;
	uint32_t size;
	uint32_t entry_count;
	uint32_t entry_size;
	uint32_t string_table_index;
	elf_section section_header[0];
} __attribute__((packed)) elf_header;

static free_pages free_data;

void init_mmu(void* ELF)
{
	LOCK;
	void* default_ELF; //backup pointer
	tag_head* cur;
	tag_bootname* bootname;	
	uint32_t total_tag_size;

	default_ELF = ELF;
	free_data.pool_head = NULL;
	free_data.pool_tail = NULL;
	free_data.page_head = NULL;
	free_data.page_tail = NULL;

	//fixed tag
	cur = (tag_head*) ELF;
	total_tag_size = cur->type; //special case, actually total tag size
	printk("TOTAL TAG SIZE: %d \n", total_tag_size);
	printk("TYPE %u SIZE %u ELF %p\n", cur->type, cur->size, ELF);
	ELF += 8;
	//idk
	while(cur->type != 0 && cur->size != 8)
	{
		cur = (tag_head*) ELF;
		printk("TYPE %u SIZE %u ELF %p\n", cur->type, cur->size, ELF);
		ELF += align8(cur->size);

	}
	UNLOCK;
}

static uint32_t align8(uint32_t num)
{
	while(num % 8 != 0)
		num++;
	return num;
}
