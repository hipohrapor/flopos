CC = gcc
LD = ld
CFLAGS = -m32 -ffreestanding -c
LDFLAGS = -m elf_i386 -T link.ld

ISO_DIR = iso
KERNEL = kernel.bin
ISO = flopos.iso

all: $(ISO)

kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

$(KERNEL): kernel.o
	$(LD) $(LDFLAGS) -o $(KERNEL) kernel.o

$(ISO): $(KERNEL)
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.bin
	grub-mkrescue -o $(ISO) $(ISO_DIR)

run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

clean:
	rm -f *.o *.bin *.iso

