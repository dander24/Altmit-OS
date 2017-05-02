global start
global gdt64
extern long_mode_start

section .text
bits 32
;kernel startup routine
start:
	;init sp
	mov esp, stack_top
	mov [esp], ebx
	;prep for change to long mode
	call check_multiboot
	call check_cpuid
	call check_long_mode
	call set_up_page_tables
	call enable_paging
	lgdt [gdt64.pointer]
	jmp gdt64.code:long_mode_start
	
	;print OK to screen
	mov dword [0xb8000], 0x2f4b2f4b
	hlt

;multiboot feature verification
check_multiboot:
    cmp eax, 0x36d76289
    jne .no_multiboot
    ret
;multiboot error code '0'
.no_multiboot:
    mov al, "0"
    jmp error

;CPUID detection
;taken from OSDev as per tutorial
check_cpuid:
    ; Check if CPUID is supported by attempting to flip the ID bit (bit 21)
    ; in the FLAGS register. If we can flip it, CPUID is available.

    ; Copy FLAGS in to EAX via stack
    pushfd
    pop eax

    ; Copy to ECX as well for comparing later on
    mov ecx, eax

    ; Flip the ID bit
    xor eax, 1 << 21

    ; Copy EAX to FLAGS via the stack
    push eax
    popfd

    ; Copy FLAGS back to EAX (with the flipped bit if CPUID is supported)
    pushfd
    pop eax

    ; Restore FLAGS from the old version stored in ECX (i.e. flipping the
    ; ID bit back if it was ever flipped).
    push ecx
    popfd

    ; Compare EAX and ECX. If they are equal then that means the bit
    ; wasn't flipped, and CPUID isn't supported.
    cmp eax, ecx
    je .no_cpuid
    ret
;CPUID error code '1'
.no_cpuid:
    mov al, "1"
    jmp error

;long mode verification
;code once again from OSDev
check_long_mode:
    ; test if extended processor info in available
    mov eax, 0x80000000    ; implicit argument for cpuid
    cpuid                  ; get highest supported argument
    cmp eax, 0x80000001    ; it needs to be at least 0x80000001
    jb .no_long_mode       ; if it's less, the CPU is too old for long mode

    ; use extended info to test if long mode is available
    mov eax, 0x80000001    ; argument for extended processor info
    cpuid                  ; returns various feature bits in ecx and edx
    test edx, 1 << 29      ; test if the LM-bit is set in the D-register
    jz .no_long_mode       ; If it's not set, there is no long mode
    ret
;LONGMODE error code '2'
.no_long_mode:
    mov al, "2"
    jmp error

;prepare page tables
set_up_page_tables:
	;map p4 to p3
	mov eax, p3_table
	or eax, 0b11
	mov [p4_table], eax
	;map p3 to p2
	mov eax, p2_table
	or eax, 0b11
	mov [p3_table], eax
	
	;map p2 entries
	mov ecx, 0

.map_p2_table:
	;map entry ecx from P2 to a huge page at address 2MiB*ecx
	mov eax, 0x200000
	mul ecx
	or eax, 0b10000011
	mov [p2_table + ecx * 8], eax
	inc ecx
	cmp ecx, 512 ;whole table is mapped
	jne .map_p2_table
	ret

;load tables, enable paging
enable_paging:
	mov eax, p4_table
	mov cr3, eax
	;PAE mode
	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax
	;LONG MODE BIT
	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr
	;enable paging in cr0
	mov eax, cr0
	or eax, 1 << 31
	mov cr0, eax
	ret

;prints an error code to screen, no vga supp
error:
    mov dword [0xb8000], 0x4f524f45
    mov dword [0xb8004], 0x4f3a4f52
    mov dword [0xb8008], 0x4f204f20
    mov byte  [0xb800a], al
    hlt

;memory config
section .bss
;page table
align 4096
p4_table:
	resb 4096
p3_table:
	resb 4096
p2_table:
	resb 4096
;init stack
stack_bottom:
	resb 128
stack_top:

section .rodata
gdt64:
	dq 0 ;null entry required for 64 bit long mode
.code:  equ $ - gdt64
	dq (1 << 43) | ( 1<< 44) | (1 << 47) | (1 << 53) ;kernel code segment
	resb 64
.pointer:
	dw $ - gdt64 - 1
	dq gdt64
