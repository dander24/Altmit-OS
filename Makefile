arch ?= x86_64
kernel := build/kernel.bin
img := build/os.img
loc := src/arch/$(arch)

linker_script := src/arch/$(arch)/linker.ld
grub_cfg := src/arch/$(arch)/grub.cfg

assembly_source_files := $(wildcard src/arch/$(arch)/*.asm)
assembly_object_files := $(patsubst src/arch/$(arch)/%.asm, \
	build/arch/$(arch)/%.o, $(assembly_source_files))

c_source_files := $(wildcard src/arch/$(arch)/*.c)
c_object_files := $(patsubst src/arch/$(arch)/%.c, \
	build/arch/$(arch)/%.o, $(c_source_files))



.PHONY: all clean run img

all: $(kernel)

clean:
	@sudo rm -r build
	@sudo losetup -d /dev/loop0
	@sudo losetup -d /dev/loop1
	@sudo umount /mnt/fatgrub

run: $(img)
	@sudo qemu-system-x86_64 -s -drive format=raw,file=$(img) -serial stdio

img: $(img)

$(img): $(kernel) $(grub_cfg)
	@mkdir -p build/img/boot/grub
	@cp $(kernel) build/img/boot/kernel.bin
	@cp $(grub_cfg) build/img/boot/grub
	@sudo dd if=/dev/zero of=$(img) bs=512 count=32768
	@sudo parted $(img) mklabel msdos
	@sudo parted $(img) mkpart primary fat32 2048s 30720s
	@sudo parted $(img) set 1 boot on
	@sudo losetup /dev/loop0 $(img)
	@sudo losetup /dev/loop1 $(img) -o 1048576
	@sudo mkdosfs -F32 -f 2 /dev/loop1
	@sudo mount /dev/loop1 /mnt/fatgrub
	@sudo grub-install --root-directory=/mnt/fatgrub --no-floppy --modules="normal part_msdos ext2 multiboot" /dev/loop0
	@sudo cp -r build/img/* /mnt/fatgrub
	@sudo umount /mnt/fatgrub
	@sudo losetup -d /dev/loop0
	@sudo losetup -d /dev/loop1


$(kernel): $(assembly_object_files) $(c_object_files) $(linker_script)
	@echo "$(c_object_files)"
	@x86_64-elf-ld -n -T $(linker_script) -o $(kernel) $(assembly_object_files) $(c_object_files) 

build/arch/$(arch)/%.o: src/arch/$(arch)/%.asm
	@mkdir -p $(shell dirname $@)
	@nasm -felf64 $< -o $@
	
build/arch/$(arch)/%.o: src/arch/$(arch)/%.c
	@mkdir -p $(shell dirname $@)
	@x86_64-elf-gcc -g -Wall -mno-red-zone -c $< -o $@
