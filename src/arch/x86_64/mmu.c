#include "VGA.h"
#include "string.h"
#include "mmu.h"
#include "bio.h"
#include "interrupt.h"
#include <stdint-gcc.h>

//tag types
#define MEMORY_MAP 6
#define ELF_SYMBOLS 9

//mem region type
#define FREE 1

#define MAX_BLOCKS 3
#define FOURK 4096
#define TWOMB FOURK * 500
#define PCOUNT 512

static uint32_t align8(uint32_t num);
static uint64_t align4k(uint64_t num);
void* walk_page_table(uint64_t addr);

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

typedef struct virtualAddr{
uint64_t pys_offset:12;
uint64_t pt1_offset:9;
uint64_t pt2_offset:9;
uint64_t pt3_offset:9;
uint64_t pt4_offset:9;
uint64_t sign_ext:8;
}__attribute__((packed)) virt_addr;

typedef struct memallocinfo{
uint64_t p4;
uint64_t p3;
uint64_t p2;
uint64_t p1;
} mem_alloc_info;

static mem_block avail_mem[MAX_BLOCKS];
static free_pool* free_pages;
static PT4* page_table; //same as value in CR3, so we don't have to asm every time
static void* max_addr;
static mem_alloc_info cur_stack, cur_heap;

void flush_tlb()
{
	uint64_t temp;
	asm volatile ("movq %%cr3, %0": "=r"(temp):);
	asm volatile ("movq %0, %%cr3": : "r"(temp));
}

void page_fault_handler(int irq, int error, void* args)
{
	unsigned long cr2;
	asm volatile ("mov %%cr2, %0" : "=r"(cr2));
	
	virt_addr vadd;
	vadd =  *((virt_addr*) &cr2);
	printk("PF_vaddr %p \n", vadd);
	uint64_t cur = page_table;
	
	PT4  *pt4p;
	PT3  *pt3p;
	PT2  *pt2p;
	PT1  *pt1p;
	pt4p = (PT4*) cur;
	if(pt4p[vadd.pt4_offset].AVL != 1)
	{
		printk("NO PT ENTRY 4");
		asm("hlt");
	}
		
	pt3p = (PT3*) (pt4p[vadd.pt4_offset].BASE << 12);
	if(pt3p[vadd.pt3_offset].AVL != 1)
	{
		printk("NO PT ENTRY 3");
		asm("hlt");
	}
	pt2p = (PT2*) (pt3p[vadd.pt3_offset].BASE << 12);
	if(pt2p[vadd.pt2_offset].AVL != 1)
	{
		printk("NO PT ENTRY 2\n");
		asm("hlt");
	}
	pt1p = (PT1*) (pt2p[vadd.pt2_offset].BASE << 12);
	pt1p[vadd.pt1_offset].BASE = ((uint64_t)alloc_pf() >> 12);
	pt1p[vadd.pt1_offset].AVL = 1;
	pt1p[vadd.pt1_offset].P = 1;
	printk("ALLOC %p \n", pt1p[vadd.pt1_offset].BASE << 12);
	printk("OFFSETS %u %u %u %u\n", vadd.pt4_offset, vadd.pt3_offset, vadd.pt2_offset, vadd.pt1_offset);
	return;
}

void* alloc_heap(int numPages)
{
	LOCK;
	uint64_t cur = page_table;
	PT4  *pt4p;
	PT3  *pt3p;
	PT2  *pt2p;
	PT1  *pt1p;
	
	virt_addr ret;
	void* retval;
	
	ret.pys_offset = 0;
	ret.pt1_offset = cur_heap.p1;
	ret.pt2_offset = cur_heap.p2;
	ret.pt3_offset = cur_heap.p3;
	ret.pt4_offset = cur_heap.p4;
	retval = *((uint64_t*) &ret);

	pt4p = (PT4*) cur;
	for(int i = 0; i < numPages; i++)
	{
		if(pt4p[cur_heap.p4].AVL == 0)
		{
			pt4p[cur_heap.p4].BASE = ((uint64_t)alloc_pf() >> 12);
			pt4p[cur_heap.p4].AVL = 1;
			pt4p[cur_heap.p4].P = 1;
			pt4p[cur_heap.p4].US = 0;
			pt4p[cur_heap.p4].RW = 1;
		}
	
		pt3p = (PT3*) (pt4p[cur_heap.p4].BASE << 12);
		if(pt3p[cur_heap.p3].AVL == 0)
		{
			pt3p[cur_heap.p3].BASE = ((uint64_t)alloc_pf() >> 12);
			pt3p[cur_heap.p3].AVL = 1;
			pt3p[cur_heap.p3].P = 1;
			pt3p[cur_heap.p3].US = 0;
			pt3p[cur_heap.p3].RW = 1;
		}

		pt2p = (PT2*) (pt3p[cur_heap.p3].BASE << 12);
		if(pt2p[cur_heap.p2].AVL == 0)
		{
			pt2p[cur_heap.p2].BASE = ((uint64_t)alloc_pf() >> 12);
			pt2p[cur_heap.p2].AVL = 1;
			pt2p[cur_heap.p2].P = 1;
			pt2p[cur_heap.p2].US = 0;
			pt2p[cur_heap.p2].RW = 1;
		}
		pt1p = (PT1*) (pt2p[cur_heap.p2].BASE << 12);
	
			pt1p[cur_heap.p1].AVL = 0; //needs to be faulted in
			pt1p[cur_heap.p1].P = 0;
			pt1p[cur_heap.p1].US = 0;
			pt1p[cur_heap.p1].RW = 1;
			cur_heap.p1++; //incr stack state
		if(cur_heap.p1 == PCOUNT)
		{
			cur_heap.p2++;
			cur_heap.p1 = 0;
			if(cur_heap.p2 == PCOUNT)
			{
				cur_heap.p3++;
				cur_heap.p2 = 0;
				if(cur_heap.p3 == PCOUNT)
				{
					cur_heap.p4++;
					cur_heap.p3 = 0;
				}
			}
		}
	}
	//return top of stack
	UNLOCK;
	return retval;
}

void free_heap(void* free_ptr, int numPages)
{
	LOCK;
	uint64_t cur = page_table;
	uint64_t temp;
	PT4  *pt4p;
	PT3  *pt3p;
	PT2  *pt2p;
	PT1  *pt1p;
	
	mem_alloc_info free_state;
	virt_addr free_int;
	free_int = *((virt_addr*)&free_ptr);
	free_state.p4 = free_int.pt4_offset;
	free_state.p3 = free_int.pt3_offset;
	free_state.p2 = free_int.pt2_offset;
	free_state.p1 = free_int.pt1_offset;

	pt4p = (PT4*) cur;
	for(int i = 0; i < numPages; i++)
	{
		if(pt4p[free_state.p4].AVL == 0)
		{
			printk("FREEING NON PAGE L4");
			asm("hlt");
		}
	
		pt3p = (PT3*) (pt4p[free_state.p4].BASE << 12);
		if(pt3p[free_state.p3].AVL == 0)
		{
			printk("FREEING NON PAGE L3");
			asm("hlt");
		}

		pt2p = (PT2*) (pt3p[free_state.p3].BASE << 12);
		if(pt2p[free_state.p2].AVL == 0)
		{
			printk("FREEING NON PAGE L2");
			asm("hlt");
		}
		pt1p = (PT1*) (pt2p[free_state.p2].BASE << 12);
		if(pt1p[free_state.p1].AVL == 1)
		{
			temp = pt1p[free_state.p1].BASE << 12;
			printk("FREEING %p \n", temp);
			free_pf(temp);
			pt1p[free_state.p1].AVL = 0; //needs to be faulted in
			pt1p[free_state.p1].P = 0;
		}
		free_state.p1++; //incr stack state
		if(free_state.p1 == PCOUNT)
		{
			free_state.p2++;
			free_state.p1 = 0;
			if(free_state.p2 == PCOUNT)
			{
				free_state.p3++;
				free_state.p2= 0;
				if(free_state.p3 == PCOUNT)
				{
					free_state.p4++;
					free_state.p3 = 0;
				}
			}
		}
	}
	UNLOCK;
}

void* alloc_stack()
{
	LOCK;
	uint64_t cur = page_table;
	PT4  *pt4p;
	PT3  *pt3p;
	PT2  *pt2p;
	PT1  *pt1p;
	
	virt_addr ret;
	void* retval;
	int q = 1;
	
	pt4p = (PT4*) cur;
	for(int i = 0; i < PCOUNT; i++)
	{
		if(pt4p[cur_stack.p4].AVL == 0)
		{
			pt4p[cur_stack.p4].BASE = ((uint64_t)alloc_pf() >> 12);
			pt4p[cur_stack.p4].AVL = 1;
			pt4p[cur_stack.p4].P = 1;
			pt4p[cur_stack.p4].US = 0;
			pt4p[cur_stack.p4].RW = 1;
		}
	
		pt3p = (PT3*) (pt4p[cur_stack.p4].BASE << 12);
		if(pt3p[cur_stack.p3].AVL == 0)
		{
			pt3p[cur_stack.p3].BASE = ((uint64_t)alloc_pf() >> 12);
			pt3p[cur_stack.p3].AVL = 1;
			pt3p[cur_stack.p3].P = 1;
			pt3p[cur_stack.p3].US = 0;
			pt3p[cur_stack.p3].RW = 1;
		}

		pt2p = (PT2*) (pt3p[cur_stack.p3].BASE << 12);
		if(pt2p[cur_stack.p2].AVL == 0)
		{
			pt2p[cur_stack.p2].BASE = ((uint64_t)alloc_pf() >> 12);
			pt2p[cur_stack.p2].AVL = 1;
			pt2p[cur_stack.p2].P = 1;
			pt2p[cur_stack.p2].US = 0;
			pt2p[cur_stack.p2].RW = 1;
		}
		pt1p = (PT1*) (pt2p[cur_stack.p2].BASE << 12);
		pt1p[cur_stack.p1].A = 0; //needs to be faulted in
		pt1p[cur_stack.p1].P = 0;
		pt1p[cur_stack.p1].US = 0;
		pt1p[cur_stack.p1].RW = 1;
		// TEMP
		ret.pt1_offset = cur_stack.p1;
		ret.pt2_offset = cur_stack.p2;
		ret.pt3_offset = cur_stack.p3;
		ret.pt4_offset = cur_stack.p4;
		uint64_t* tempv = *( (uint64_t*) &ret);
		//printk("VIRT ADDR SET = %p\n", tempv);
		//END TEMP		alloc_heap(511);
		cur_stack.p1++; //incr stack state
		if(cur_stack.p1 == PCOUNT)
		{
			cur_stack.p2++;
			cur_stack.p1 = 0;
			if(cur_stack.p2 == PCOUNT)
			{
				cur_stack.p3++;
				cur_stack.p2 = 0;
				if(cur_stack.p3 == PCOUNT)
				{
					cur_stack.p4++;
					cur_stack.p3 = 0;
				}
			}
		}
	}
	
	
	//return top of stack
	ret.pys_offset = 0;
	ret.pt1_offset = cur_stack.p1;
	ret.pt2_offset = cur_stack.p2;
	ret.pt3_offset = cur_stack.p3;
	ret.pt4_offset = cur_stack.p4;
	retval = *((uint64_t*) &ret);
	UNLOCK;
	return retval;
}

void* walk_page_table(uint64_t addr)
{
	LOCK;
	virt_addr vadd;
	printk("addr %p \n", addr);
	vadd =  *((virt_addr*) &addr);
	printk("vaddr %p \n", vadd);
	uint64_t cur = page_table;
	
	printk("pt4 %p\n", *(int*)addr);
	cur = ((PT4*)cur)[vadd.pt4_offset].BASE << 12;
	cur = ((PT3*)cur)[vadd.pt3_offset].BASE << 12;
	cur = ((PT2*)cur)[vadd.pt2_offset].BASE << 12;
	cur = ((PT1*)cur)[vadd.pt1_offset].BASE << 12;
	cur = cur + vadd.pys_offset;
	printk("res %p \n", cur);
	UNLOCK;
	return cur;
}

void init_page_table()
{
	LOCK;
	page_table = (PT4*) alloc_pf();
	
	cr3_reg cr3r;
	PT4  *pt4p;
	PT3  *pt3p;
	PT2  *pt2p;
	PT1  *pt1p;

	//address used for identity map
	uint64_t cur_addr = 0;
	
	//prep stack and heap 
	cur_stack.p4 = 2;
	cur_stack.p3 = 0;
	cur_stack.p2 = 0;
	cur_stack.p1 = 0;

	cur_heap.p4 = 4;
	cur_heap.p3 = 0;
	cur_heap.p2 = 0;
	cur_heap.p1 = 0;
	

	//set cr3 and pointer to level four of page table
	cr3r.base = (uint64_t)page_table >> 12;
	cr3r.reserved = 0;
	cr3r.RES1 = 0;
	cr3r.PWT = 0;
	cr3r.PCD = 0;
	pt4p = page_table;

	//make the identity map in slot one of lvl 4
	
	//create pointer to level 3 table
	pt4p[0].BASE = (PT3*) ((uint64_t)alloc_pf() >> 12);
	//point to level 3 table (only need one for identity map	
	pt3p = pt4p[0].BASE << 12;
	printk("%p %p\n", pt3p, page_table);

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
				if((cur_addr << 12) > max_addr)
				{
					int q = 1;
					irq_set_handler(14, &page_fault_handler, NULL);
					asm volatile ("movq %0, %%cr3": : "r"(cr3r));
					printk("MAX %p \n", max_addr);
					UNLOCK;
					return;
				}
			}

		}
	
	}
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
		kmemset(temp,0,FOURK);
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
				kmemset(next_avail,0,FOURK);
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
	//track end of memory space
	max_addr = NULL;

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
		//printk("%d::ADDR %qx LEN %qx\n", i, avail_mem[cur_block].head, avail_mem[cur_block].tail-avail_mem[cur_block].head);
			cur_block++;
		}
		if((info.start_addr + info.length)> max_addr)
			 max_addr = info.start_addr + info.length;
	}
	printk("MAX: %p \n", max_addr);
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
