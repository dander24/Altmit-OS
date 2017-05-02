global long_mode_start
extern kernel_main

section .text
bits 64

;64 bit startup code
long_mode_start:
	;save ELF addr
	mov rdi, [rsp]
	;clear data registers
	mov ax, 0
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	;print 'okay' until I can be assed to change it
	mov rax, 0x2f592f412f4b2f4f
	mov qword [0xb8000], rax
	;rdi should have elf header
	call kernel_main
        mov qword [0xb8020], rax
	hlt
