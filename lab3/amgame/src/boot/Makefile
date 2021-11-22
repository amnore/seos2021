SRCS := start.S main.c
bootblock.o: $(SRCS) Makefile
	@echo + CC $(SRCS)
	@$(CROSS_COMPILE)gcc -static -m32 -fno-pic -Os -nostdlib -Ttext 0x7c00 -I$(AM_HOME)/am/src -o bootblock.elf $(SRCS)
	@cp bootblock.elf bootblock.o
	@python3 genboot.py bootblock.o

clean:
	rm -rf *.o
