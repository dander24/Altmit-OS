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

#define MAX_BLOCKS 3
#define FOURK 4096
#define PCOUNT 512

static uint32_t align8(uint32_t num);
static uint64_t align4k(uint64_t num);

typedef struct free_pool{
	//void* page;
	void* next;
} free_pool;

typedef struct availmem{
	void* head;
	void* tail;
} mem_block;

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

static mem_block avail_mem[MAX_BLOCKS];
static free_pool* free_pages;
static PT4* page_table; //same as value in CR3, so we don't have to asm every time

void init_page_table()
{
	page_table = (PT4*) alloc_pf();
	
	cr3_reg cr3r;
	PT4  *pt4p;
	PT3  *pt3p;
	PT2  *pt2p;
	PT1  *pt1p;

	//address used for identity map
	uint64_t cur_addr = 0;

	
	//set cr3 and pointer to level four of page table
	cr3r.base = page_table;
	pt4p = page_table;

	printk("%p %p\n", cr3r, page_table);
	//make the identity map in slot one of lvl 4
	
	//create pointer to level 3 table
	pt4p[0].BASE = (PT3*) alloc_pf();
	//point to level 3 table (only need one for identity map	
	pt3p = pt4p[0].BASE;

	//configure rest of current entry
	pt4p[0].RW = 1;
	pt4p[0].P = 1;
	pt4p[0].US = 0; 
	
	//loop through every entry of level 3
	for(int i = 0;  i < PCOUNT; i++)
	{
		pt3p[i].RW = 1;
		pt3p[i].P = 1;
		pt3p[i].US = 0; 
		pt3p[i].BASE = (PT2*) ((uint64_t)alloc_pf() >> 12);
		pt2p = pt3p[i].BASE << 12;

		//loop through every entry of level 2 (from cur level 3)
		for(int j = 0; j < PCOUNT; j++)
		{
			pt2p[j].RW = 1;
			pt2p[j].P = 1;
			pt2p[j].US = 0; 
			pt2p[j].BASE = (PT1*) ((uint64_t)alloc_pf() >> 12);
			pt1p = pt2p[j].BASE << 12;
			//loop through every entry of level 1 (from cur level 2)
			for(int k = 0; k < PCOUNT; k++)
			{
				pt1p[k].RW = 1;
				pt1p[k].P = 1;
				pt1p[k].US = 0; 
				pt1p[k].BASE = cur_addr++;
			}

		}
	
	}
	//asm("movq cr3, %0": "=a"(cr3r): );
	printk("%p %p\n", cr3r, page_table);
}

void* alloc_pf(void)
{
	LOCK;
	free_pool* temp;
	uint64_t next_avail;
	if(free_pages != NULL)
	{
		
		temp = free_pages; //get old
		free_pages = free_pages->next;
		UNLOCK;
		return temp;
	}

	for(int i = 0; i < MAX_BLOCKS; i++)
	{
		if(avail_mem[i].head != NULL)
		{
			next_avail = align4k((uint64_t) avail_mem[i].head);
			//verify we actually have a PAGE to return
			if((uint64_t) avail_mem[i].tail - next_avail >= FOURK)
			{
				avail_mem[i].head = next_avail + FOURK;
				UNLOCK;
				return (void*) next_avail;
			}
		}

	}
	printk("ALLOC_PF ERROR @ %p", next_avail);
	asm("hlt");
	return NULL;
}

void free_pf(void* freed)
{
	LOCK;
	free_pool *temp, new;
	if(free_pages == NULL)
	{
		free_pages = freed;
		free_pages->next = NULL;
		UNLOCK;
		return;
	}
	temp = free_pages;
	while(temp->next != NULL)
	{
		temp = temp->next;
	}
	temp->next = freed;
	temp = temp->next;
	temp->next = NULL;
	UNLOCK;
}

static void parse_memtag(void* ELF)
{
	mem_map* map;
	mem_info info;
	uint32_t entry_size, total_size, entry_count, cur_block = 0;

	map = (mem_map*) ELF;
	entry_size = map->ent_size;
	total_size = map->size;
	entry_count = total_size / entry_size;
	printk("ENT SIZE %d TOTAL SIZE %d COUNT %d \n", entry_size, total_size, entry_count);

	for(int i = 0; i < entry_count ; i++)
	{	
		info = map->mem_info_arr[i];
	
		if(info.type == FREE) //usbale
		{
					
			avail_mem[cur_block].head = info.start_addr;
			if(avail_mem[cur_block].head == 0)
				avail_mem[cur_block].head += FOURK;
			avail_mem[cur_block].tail = info.start_addr + info.length;
		printk("%d::ADDR %qx LEN %qx\n", i, avail_mem[cur_block].head, avail_mem[cur_block].tail-avail_mem[cur_block].head);	
			cur_block++;
		}
		
	}
}

static void parse_elfsym(void* ELF)
{
	elf_section section;
	elf_header* header;
	uint32_t total_size, num_entries;
	uint64_t start, end, size;
	
	header = (elf_header*) ELF;
	total_size = header->size;
	num_entries = header->entry_count;

	for(int i = 0; i < num_entries; i++)
	{
		section = header->section_header[i];
		printk("%d:::TYPE %d ADDR %qx LEN %qx\n", i, section.section_type, section.addr, section.size);	
		start = section.addr;
		end = start + section.size;
		size = section.size;
		
		for(int j = 0; j < MAX_BLOCKS; j++)
		{
			if(avail_mem[j].head != NULL) //block exists
			{
				if(end > avail_mem[j].head && end <= avail_mem[j].tail) //section contained
				{
					avail_mem[j].head = end + 1;

				}
				else if(start > avail_mem[j].head && start <= avail_mem[j].tail) //section passes end
				{
					avail_mem[j].tail = start - 1;

				}
			}

		}
	}
	for(int k = 0; k < MAX_BLOCKS; k++)
	{
		if(avail_mem[k].head != NULL)
			printk("START %p END %p \n", avail_mem[k].head, avail_mem[k].tail);
	}


}

void init_mmu(void* ELF)
{
	LOCK;
	void* ELF_HEADER; 
	tag_head* cur;
	uint32_t total_tag_size;

	for(int k = 0; k < 4 ; k++)
	{
		avail_mem[k].head = NULL;
		avail_mem[k].tail = NULL;	
	}

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
		if(cur->type == MEMORY_MAP)
			parse_memtag(cur);
		if(cur->type == ELF_SYMBOLS)
			ELF_HEADER = (void*) cur; //save this to make sure we parse mmap first
		ELF += align8(cur->size);

	}
	parse_elfsym(ELF_HEADER);
	
	//assume we found at least one block of memory
	free_pages = NULL;

	//init page table
	init_page_table();
	UNLOCK;
}

static uint64_t align4k(uint64_t num)
{
	int car = num % FOURK;
	if(car != 0)
		num += FOURK - car;
	return num;	
}

static uint32_t align8(uint32_t num)
{
	while(num % 8 != 0)
		num++;
	return num;
}
