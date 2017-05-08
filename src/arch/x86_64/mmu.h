#ifndef MMU
#define MMU


void init_mmu(void*);
void* alloc_pf(void);
void free_pf(void*);

typedef struct CR3_T{
uint64_t reserved:3;
uint64_t PWT:1;
uint64_t PCD:1;
uint64_t RES1:7;
uint64_t base:40;
uint64_t RES2:12;
} __attribute__((packed)) cr3_reg;

typedef struct PML4{
uint64_t P:1;
uint64_t RW:1;
uint64_t US:1;
uint64_t PWT:1;
uint64_t PCD:1;
uint64_t A:1;
uint64_t IGN:3;
uint64_t AVL:3;
uint64_t BASE:40;
uint64_t AVL2:11;
uint64_t NX:1;
} __attribute__((packed)) PT4;

typedef struct PML3{
uint64_t P:1;
uint64_t RW:1;
uint64_t US:1;
uint64_t PWT:1;
uint64_t PCD:1;
uint64_t A:1;
uint64_t IGN:1;
uint64_t ZERO:1;
uint64_t MBZ:1;
uint64_t AVL:3;
uint64_t BASE:40;
uint64_t AVL2:11;
uint64_t NX:1;
} __attribute__((packed)) PT3;


typedef struct PML2{
uint64_t P:1;
uint64_t RW:1;
uint64_t US:1;
uint64_t PWT:1;
uint64_t PCD:1;
uint64_t A:1;
uint64_t IGN:1;
uint64_t ZERO:1;
uint64_t IGN2:1;
uint64_t AVL:3;
uint64_t BASE:40;
uint64_t AVL2:11;
uint64_t NX:1;
} __attribute__((packed)) PT2;

typedef struct PML1{
uint64_t P:1;
uint64_t RW:1;
uint64_t US:1;
uint64_t PWT:1;
uint64_t PCD:1;
uint64_t A:1;
uint64_t G:1;
uint64_t ZERO:1;
uint64_t D:1;
uint64_t AVL:3;
uint64_t BASE:40;
uint64_t AVL2:11;
uint64_t NX:1;
} __attribute__((packed)) PT1;



#endif
